#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <future>
#include <optional>

#include "http_client/Types.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // From <boost/asio/ssl.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace nvd {

/// @brief  Manage a client session on top the boost ASIO and beast library.
class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:

    /// @brief construct ClientSession object given the io_context and ssl context
    explicit ClientSession(net::io_context& ioc, 
                           ssl::context& sslCtx,
                           const std::string& host, 
                           const std::string& port,
                           AuthMethod authMethod = AuthMethod::BASIC);

    /// @brief establish connection sync
    void connect();

    void reconnect();
    
    /// @brief disconnect stream 
    void disconnect();

    /// @brief check if the stream is open
    bool isConnected() const;

    /// @brief establish connection async
    std::future<void> connectAsync(const std::string& host, const std::string& port);

    /// @brief send a sync request
    Result sendRequest(const std::string& target, int version);

    /// @brief send an async request
    std::future<std::chrono::milliseconds> sendRequestAsync(const std::string& target, int version);

private:

    void onResolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    // async callback when stream is end 
    void closeConnectionAsync();
    void on_shutdown(beast::error_code ec);
    void reestablish_connection();

    // sync
    void recreate_stream();

    void create_request(const std::string& target, int version);

    void setReqBasicAuth();

    void fail(beast::error_code ec, const char* what);

    enum class StreamState
    {
        Closed,
        Connected,
        EndOfStream,
        UnknownError
    };

private:

    tcp::resolver _resolver;
    beast::ssl_stream<beast::tcp_stream> _stream; // Use SSL stream instead of basic TCP stream
    ssl::context& _sslCcontext;
    beast::flat_buffer _buffer; // (Must persist between reads)
    http::request<http::empty_body> _req;
    http::response<http::string_body> _response;

    std::string _host;
    std::string _port;

    AuthMethod _authMethod;
    StreamState _streamState;

    std::optional<std::promise<std::chrono::milliseconds>> _promise; // Holds the promise for the current request
    std::optional<std::promise<void>> _connectPromise; // Holds the promise for the current request

    // todo - move to profiler
    std::chrono::steady_clock::time_point _startTime;
};

} // namespace nvd
