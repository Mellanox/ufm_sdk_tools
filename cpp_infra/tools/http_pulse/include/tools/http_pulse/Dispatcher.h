#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <memory>
#include <thread>

#include <http_client/ClientSession.h>
#include <http_client/SSLContext.h>

#include "Types.h"
#include "HttpMetrics.h"

namespace nvd
{

/// @brief Manages the overall benchmarking logic.
///        Owns and reuses ClientSession instances for sending requests in a loop.
///        Keeps track of active connections and sends the next request once a response is received
//         It sends requests in a loop until the runtime expires.
class Dispatcher
{
public:

    Dispatcher(AuthMethod method, const HttpCommand& command);
    
    /// start runnning test
    void start();

private:

    /// @brief run a test case
    void runTest(std::string testName);

    void sendRequests(ClientSession& session);
    void sendRequestsAsync(ClientSession& session);

    std::unique_ptr<nvd::Request> createRequest() const;

    /// @brief handle response received from http client session
    void handleResponse(ClientSession& session, const Response& resp);

    nvd::AuthMethod getAuthMethod() const;

private:

    net::io_context _ioc;
    SSLContext _sslContext;
    std::optional<net::executor_work_guard<net::io_context::executor_type>> _work_guard;

    HttpCommand _command;
    HttpMetrics _metrics;

    struct TestConfig
    {
        std::string name;
    };

    std::vector<TestConfig> _testConfig;

    // define maximum message print size
    static constexpr size_t ConsoleMaxPrintSize = 1024;
};

} // namespace