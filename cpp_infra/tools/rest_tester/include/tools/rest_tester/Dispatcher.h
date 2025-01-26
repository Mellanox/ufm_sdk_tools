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

/// @brief Manages the overall benchmarking logic. 
///        Owns and reuses ClientSession instances for sending requests in a loop.
///        Keeps track of active connections and sends the next request once a response is received
//         It sends requests in a loop until the runtime expires.
class Dispatcher
{
public:

    Dispatcher(std::string host, std::string port, std::string target, size_t runtimeSeconds, int version, int num_connections, AuthMethod authMethod = AuthMethod::BASIC);
    
    // start run
    void start();

private:

    void sendRequests();

private:

    net::io_context _ioc;
    SSLContext _sslContext;
    std::optional<net::executor_work_guard<net::io_context::executor_type>> _work_guard;

    AuthMethod _authMethod;
    std::string _host;
    std::string _port;
    std::string _target;

    int _version;
    int _numConnections;
    int _duration;

    std::vector<std::shared_ptr<ClientSession>> _sessions;
    HttpMetrics _metrics;
};

} // namespace