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

Dispatcher::Dispatcher(AuthMethod method, const HttpCommand& command) :
    _sslContext(method, command.user, command.certPath),
    _work_guard(net::make_work_guard(_ioc)),
    _command(command),
    _metrics(_command.target, command.runtime_seconds ? *command.runtime_seconds : 1U, command.metrics_out_path, command.name)
{
    // todo - read from config tests (or other class e.g. TestVector)
    _testConfig.push_back({command.name});

    // case 'runtime_seconds' nullopt set dry_run
    _command.dry_run = !command.runtime_seconds ? true : command.dry_run;
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
        sendRequests(*session);
    }

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
    else if (_command.token)
    {
        return nvd::AuthMethod::TOKEN;
    }

    LOGERROR("Unknown AuthMethod");
    return nvd::AuthMethod::UNKNOWN;
}

std::unique_ptr<nvd::Request> Dispatcher::createRequest() const
{
    auto req = std::make_unique<nvd::Request>();

    auto auth = getAuthMethod();
    
    req->create(Request::HttpVerb::get, _command.target, _command.host, auth);

    if (_command.user)
    {
        req->setAuthorization(*_command.user);
    }
    if (_command.token)
    {
        req->setTokenAuthorization(*_command.token);
    }
    return req;
}

// todo - sendRequestsAsync & sendRequests should use sendRequests_i for the common code
void Dispatcher::sendRequests(ClientSession& session)
{
    auto req = createRequest();

    auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(_metrics.runtimeInSec());

    // Loop to send requests while runtime has not expired
    while (std::chrono::steady_clock::now() < endTime)
    {
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
                handleResponse(*resp);
            }
            else
            {
                _metrics.record_fail(ErrorCode::UnknownError);
                LOGERROR("sendRequest Failed. Exit the test");
                break;
            }
            if (_command.dry_run) break;
        } 
        catch (const std::exception& e)
        {
            std::cerr << "Request error: " << e.what() << "\n";
            LOGERROR("sendRequest exception Err {}. Exit the test", e.what());
            break;
        }
    }
}

// todo - sendRequestsAsync & sendRequests should use sendRequests_i for the common code
void Dispatcher::sendRequestsAsync(ClientSession& session)
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
            _metrics.record_request();
            auto fuResp = session.sendRequestAsync(*req);
            auto resp = fuResp.get();
            handleResponse(resp);
        } 
        catch (const std::exception& e)
        {
            std::cerr << "Request error: " << e.what() << "\n";
        }

        if (_command.dry_run) break;
    }
}


void Dispatcher::handleResponse(const Response& resp)
{
    _metrics.record_response(std::chrono::duration_cast<std::chrono::milliseconds>(resp.latency), resp.statusCode);

    if (resp.latency > 500ms)
    {
        LOGINFO("Request {} Latency {} ms", _command.target, std::chrono::duration_cast<std::chrono::milliseconds>(resp.latency).count());
    }

    if (resp.statusCode == ErrorCode::Success)
    {
        if (_command.dry_run)
        {
            if (resp.payload.size() <= ConsoleMaxPrintSize)
            {
                std::cout << resp.payload << std::endl;
            }
            else
            {
                std::cout << resp.payload.substr(0, ConsoleMaxPrintSize) << "..." << std::endl; // Add ellipsis to indicate truncation
            }
        }
    }
    else if (resp.statusCode == ErrorCode::FoundTempRedirect)
    {
        std::cout << "Received 302 Redirect\n";
        // auto location = response[http::field::location];
        // std::cout << "Redirect to: " << location << "\n";
    }
    else
    {
        std::cout << "Responde Received Status Code : " << static_cast<uint32_t>(resp.statusCode) << "\n";
    }
}

} // namespace nvd
