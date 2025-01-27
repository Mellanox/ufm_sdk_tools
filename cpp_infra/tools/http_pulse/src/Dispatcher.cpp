#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <memory>
#include <thread>

#include <tools/http_pulse/Dispatcher.h>
#include <utils/logger/Logger.h>

#include <utils/metrics/CsvWriter.h>
#include <http_client/Request.h>

using namespace std::chrono_literals;

namespace nvd
{

// todo - read auth method from input args 
Dispatcher::Dispatcher(const HttpCommand& command) :
    _sslContext(command.user, command.certPath),
    _work_guard(net::make_work_guard(_ioc)),
    _command(command),
    _metrics(_command.url, command.runtime_seconds, command.metrics_out_path, command.name)
{
    // todo - read from config
    _testConfig.push_back({"basic_auth"});
    _testConfig.push_back({"token_auth"});
}

void Dispatcher::start()
{
    // Run the IO context in a thread pool
    std::vector<std::thread> threads;

    for (int i = 0; i < _command.num_connections; ++i)
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

    auto authMethod = getAuthMethod();

    for (int i = 0; i < _command.num_connections; ++i) 
    {
        auto session = std::make_shared<ClientSession>(_ioc, _sslContext.get(), _command.host, _command.port, authMethod);
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

nvd::AuthMethod Dispatcher::getAuthMethod() const
{
    if (_command.certPath)
    {
        return nvd::AuthMethod::SSL_CERTIFICATE;
    }
    else if (_command.user)
    {
        return nvd::AuthMethod::BASIC;
    }

    LOGERROR("Unknown AuthMethod");
    return nvd::AuthMethod::UNKNOWN;
}

std::unique_ptr<nvd::Request> Dispatcher::createRequest() const
{
    auto req = std::make_unique<nvd::Request>();

    auto auth = getAuthMethod();
    
    req->create(Request::HttpVerb::get, _command.url, _command.host, auth);

    if (_command.user)
    {
        req->setAuthorization(*_command.user);
    }
    return req;
}
    
void Dispatcher::sendRequests(ClientSession& session)
{
    auto req = createRequest();

    auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(_metrics.runtimeInSec());

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
            auto resp = session.sendRequest(*req);

            if (resp)
            {
                //LOGINFO("--- sendRequest resp {}", resp.payload);
                if (resp->latency > 500ms)
                {
                    LOGINFO("Request {} Latency {} ms", _command.url, std::chrono::duration_cast<std::chrono::milliseconds>(resp->latency).count());
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

        if (_command.dry_run) break;
    }
}

} // namespace nvd
