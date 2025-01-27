#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <memory>
#include <thread>

#include <http_client/ClientSession.h>
#include <http_client/SSLContext.h>

#include "HttpMetrics.h"

namespace nvd
{

struct HttpCommand
{
    std::string host;
    std::string port;
    std::string url;
    int num_connections;
    size_t runtime_seconds;
    int version;

    std::string metrics_out_path;
    std::string name;

    bool dry_run;

    // Optional parameteres
    std::optional<std::string> user;
    std::optional<std::string> certPath;
};

/// @brief Manages the overall benchmarking logic.
///        Owns and reuses ClientSession instances for sending requests in a loop.
///        Keeps track of active connections and sends the next request once a response is received
//         It sends requests in a loop until the runtime expires.
class Dispatcher
{
public:

    Dispatcher(const HttpCommand& command);
    
    /// start runnning test
    void start();

private:

    /// @brief run a test case
    void runTest(std::string testName);

    void sendRequests(ClientSession& session);

    std::unique_ptr<nvd::Request> createRequest() const;

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
};

} // namespace