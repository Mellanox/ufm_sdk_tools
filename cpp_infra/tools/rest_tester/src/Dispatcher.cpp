#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <memory>
#include <thread>

#include <tools/rest_tester/Dispatcher.h>
#include <utils/logger/Logger.h>

#include <utils/metrics/CsvWriter.h>
#include <http_client/Request.h>

using namespace std::chrono_literals;

namespace nvd
{

// todo - read auth method from input args 
Dispatcher::Dispatcher(std::string host, std::string port, std::string target, size_t runtimeSeconds, int version, int num_connections, AuthMethod authMethod) :
    _sslContext(authMethod),
    _work_guard(net::make_work_guard(_ioc)),
    _authMethod(authMethod),
    _host(std::move(host)), 
    _port(std::move(port)),
    _target(std::move(target)),
    _version(version),
    _numConnections(num_connections),
    _metrics(_target, runtimeSeconds, "/tmp/benchmark/auth/nvd/basic_auth_bm.csv")
{
    _testConfig.push_back({"basic_auth"});
    _testConfig.push_back({"token_auth"});
}

void Dispatcher::start()
{
    // Run the IO context in a thread pool
    std::vector<std::thread> threads;

    for (int i = 0; i < _numConnections; ++i)
    {
        threads.emplace_back([&] { _ioc.run(); });
    }

    for (auto& test : _testConfig)
    {
        runTest(test.name);
        _metrics.clear();
    }

    // Stop the IO context
    _ioc.stop();

    // Join all threads
    for (auto& t : threads) 
    {
        t.join();
    }
}

void Dispatcher::runTest(std::string testName)
{
    LOGINFO("start running : {}", testName);

    std::vector<std::shared_ptr<ClientSession>> sessions;

    for (int i = 0; i < _numConnections; ++i) 
    {
        auto session = std::make_shared<ClientSession>(_ioc, _sslContext.get(), _host, _port, _authMethod);
        sessions.push_back(session);        
    }

    for (auto& session : sessions) 
    {
        session->connect();
        sendRequests(*session);
    }

    // run blocking for 'runtime_seconds'
    _metrics.to_stream(std::cout);

    // add metrics to csv - todo format file from args/config
    _metrics.to_csv();

    // destroy the dummy work guard
    _work_guard.reset();
}

void Dispatcher::sendRequests(ClientSession& session)
{
    auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(_metrics.runtimeInSec());

    nvd::Request req;
    req.create(Request::HttpVerb::get, _target, _host, nvd::AuthMethod::BASIC);

    // Loop to send requests while runtime has not expired
    while (std::chrono::steady_clock::now() < endTime)
    {
        if (!session.isConnected())
        {
            LOGINFO("Session Connection is close. Reconnecting..");
            session.reconnect();
        }

        try
        {
            // Serialize the headers and log them
            // std::ostringstream header_stream;
            // header_stream << req.get().base(); 
            // LOGINFO("--- sendRequest {}", header_stream.str());

            _metrics.record_request();
            auto resp = session.sendRequest(req);

            if (resp)
            {
                //LOGINFO("--- sendRequest resp {}", resp.payload);
                if (resp->latency > 500ms)
                {
                    LOGINFO("Request {} Latency {} ms", _target, std::chrono::duration_cast<std::chrono::milliseconds>(resp->latency).count());
                }
                _metrics.record_response(std::chrono::duration_cast<std::chrono::milliseconds>(resp->latency), resp->statusCode);
            }
            else
            {
                _metrics.record_fail();
                LOGINFO("sendRequest Failed");
            }
        } 
        catch (const std::exception& e)
        {
            std::cerr << "Request error: " << e.what() << "\n";
        }
    }
}

} // namespace nvd
