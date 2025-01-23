#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <memory>
#include <thread>

#include "tools/rest_tester/Dispatcher.h"

namespace nvd
{

// todo - read auth method from input args 
Dispatcher::Dispatcher(std::string host, std::string port, std::string target, int version, int num_connections) :
    _sslContext(AuthMethod::SSL_CERTIFICATE),
    _work_guard(net::make_work_guard(_ioc)),
    _host(std::move(host)), _port(std::move(port)),
    _target(std::move(target)), _version(version), _numConnections(num_connections)
{
}

void Dispatcher::run(size_t runtime_seconds)
{
    for (int i = 0; i < _numConnections; ++i) 
    {
        auto session = std::make_shared<ClientSession>(_ioc, _sslContext.get(), _host, _port, nvd::AuthMethod::SSL_CERTIFICATE);
        
        // todo - build the request here 

        _sessions.push_back(session);        
    }

    // Run the IO context in a thread pool
    std::vector<std::thread> threads;

    for (int i = 0; i < _numConnections; ++i)
    {
        threads.emplace_back([&] { _ioc.run(); });
    }

    for (auto& session : _sessions) 
    {
        session->connect();
    }

    // run blocking for 'runtime_seconds'
    sendRequests(runtime_seconds);

    _metrics.print_metrics();

    // destroy the dummy work guard
    _work_guard.reset();

    // Stop the IO context
    _ioc.stop();

    // Join all threads
    for (auto& t : threads) 
    {
        t.join();
    }
}


void Dispatcher::sendRequests(size_t runtime_seconds)
{
    auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(runtime_seconds);

    // Loop to send requests while runtime has not expired
    while (std::chrono::steady_clock::now() < endTime)
    {
        for (auto& session : _sessions) 
        {
            if (!session->isConnected())
            {
                std::cout << "Session Connection is close. Reconnecting.."  << std::endl;
                session->reconnect();
            }
            
            try
            {
                _metrics.record_request();
                auto resp = session->sendRequest(_target, _version);
                _metrics.record_response(std::chrono::duration_cast<std::chrono::milliseconds>(resp.latency), resp.statusCode);
            } 
            catch (const std::exception& e)
            {
                std::cerr << "Request error: " << e.what() << "\n";
            }

        }
    }
}

} // namespace nvd
