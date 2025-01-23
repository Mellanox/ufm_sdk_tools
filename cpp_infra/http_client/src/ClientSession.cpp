
#include "http_client/ClientSession.h"

#include <boost/asio/strand.hpp>

#include "http_client/Utils.h"

namespace nvd {

//------------------------------------------------------------------------------
std::mutex _mu;

// Objects are constructed with a strand to
// ensure that handlers do not execute concurrently.
ClientSession::ClientSession(net::io_context& ioc, 
                             ssl::context& ctx,
                             const std::string& host, const std::string& port,
                             AuthMethod authMethod) :
    _resolver(net::make_strand(ioc)),
    _stream(net::make_strand(ioc), ctx),
    _sslCcontext(ctx),
    _host(host),
    _port(port),
    _authMethod(authMethod),
    _streamState(StreamState::Closed)
{
}

void ClientSession::connect()
{
    // Resolve the host and port
    auto const results = _resolver.resolve(_host, _port);

    // Connect to the server
    beast::get_lowest_layer(_stream).connect(results);

    // Perform SSL handshake
    _stream.handshake(ssl::stream_base::client);

    _streamState = StreamState::Connected;

    std::cout << "Exit connect" << std::endl;
}

void ClientSession::reconnect()
{
    recreate_stream();
    connect();
}

void ClientSession::recreate_stream() 
{
    // Reset the underlying TCP stream
    beast::get_lowest_layer(_stream).close();

    // Recreate the SSL stream
    _stream = beast::ssl_stream<beast::tcp_stream>(
        net::make_strand(beast::get_lowest_layer(_stream).get_executor()), _sslCcontext);
}

bool ClientSession::isConnected() const
{
    return beast::get_lowest_layer(_stream).socket().is_open() && _streamState == StreamState::Connected;
}

// start the asynchronous operation
std::future<void> ClientSession::connectAsync(const std::string& host, const std::string& port)
{
    _connectPromise.emplace();

    auto future = _connectPromise->get_future();

    // Look up the domain name
    _resolver.async_resolve(host, port,
                            beast::bind_front_handler(
                                &ClientSession::onResolve,
                                shared_from_this()));

    return future;
}

void ClientSession::onResolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if(ec) return fail(ec, "resolve");

    // Set a timeout on the next operation on the socket (connect)
    // If the operation doesn't complete within this time, it will fail with a timeout error.
    beast::get_lowest_layer(_stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(_stream).async_connect(
        results,
        beast::bind_front_handler(
            &ClientSession::on_connect,
            shared_from_this()));
}

void ClientSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
    if(ec) return fail(ec, "connect");

    // Set a timeout for the SSL handshake operation
    beast::get_lowest_layer(_stream).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    _stream.async_handshake(ssl::stream_base::client,
                            beast::bind_front_handler(
                            &ClientSession::on_handshake,
                            shared_from_this()));
}

void ClientSession::on_handshake(beast::error_code ec)
{
    if (ec) return fail(ec, "handshake");

    // Set a timeout on the operation
    beast::get_lowest_layer(_stream).expires_after(std::chrono::seconds(30));
}

void ClientSession::create_request(const std::string& target, int version)
{
    _req.version(version);
    _req.method(http::verb::get);
    _req.target(target);
    _req.set(http::field::connection, "keep-alive");
    _req.set(http::field::host, _host);

    if (_authMethod == AuthMethod::BASIC)
    {
        setReqBasicAuth();
    }
    else if (_authMethod == AuthMethod::SSL_CERTIFICATE)
    {
        _req.set(http::field::content_type, "application/json"); // Add Content-Type header
    }

    _req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
}

Result ClientSession::sendRequest(const std::string& target, int version)
{
    // Record the start time
    auto startTime = std::chrono::steady_clock::now();

    // reset the response
    auto response = http::response<http::string_body>();
    
    create_request(target, version);

    http::write(_stream, _req);

    http::read(_stream, _buffer, response);

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);

    return {static_cast<uint32_t>(response.result()), response.body(), latency};
}

std::future<std::chrono::milliseconds> ClientSession::sendRequestAsync(const std::string& target, int version)
{
    _promise.emplace();

    // reset the response
    _response = http::response<http::string_body>();

    // Record the start time
    _startTime = std::chrono::steady_clock::now();

    // todo pass the request in API
    // Set up an HTTP GET request message
    create_request(target, version);

    auto future = _promise->get_future();

    // Send the HTTP request to the remote host
    http::async_write(
        _stream, _req,
        beast::bind_front_handler(&ClientSession::on_write, shared_from_this()));
    
    return future;
}

void ClientSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    //std::cout << " on_write : bytes_transferred " << bytes_transferred << std::endl;

    boost::ignore_unused(bytes_transferred);
    if (ec) return fail(ec, "write");
    
    // Receive the HTTP response
    http::async_read(_stream, _buffer, _response,
        beast::bind_front_handler(
            &ClientSession::on_read,
            shared_from_this()));
}

void ClientSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec == beast::http::error::end_of_stream) 
    {
        _streamState = StreamState::EndOfStream;
        // Gracefully close the SSL stream todo - consider async_shutdown 
        _stream.shutdown();
    }
    else if (ec)
    {
        return fail(ec, "read");
    }

    //std::cout << " on_read : bytes_transferred " << bytes_transferred << ", _buffer.size:  " << _buffer.size() << std::endl;
    //std::cout << _response.body().c_str() << std::endl;
    //std::cout << "Response headers:\n" << _response.base() << "\n";
    //_response.clear();

    // Check for Connection: close
    if (_response[http::field::connection] == "close") 
    {
        std::cerr << "Server closed the connection. Reconnecting...\n";
        _streamState = StreamState::Closed;
    }    
    
    // // Gracefully close the SSL stream
    // _stream.async_shutdown(
    //     beast::bind_front_handler(&ClientSession::on_shutdown, shared_from_this()));

    // Write the message to standard out
    //std::cout << _response << std::endl;

    // todo - use profiler

    // If we get here then the connection is closed gracefully
    // Measure latency

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - _startTime);

    //std::cout << "Latency: " << latency.count() << "ms, Response: " << _response.result_int() << std::endl;
    
    // todo - check this
    _buffer.consume(_buffer.size());

    // Fulfill the promise with the latency
    if (_promise)
    {
        _promise->set_value(latency);
        _promise.reset(); // Reset the promise for the next request
    }

    // // Gracefully close the socket
    // beast::get_lowest_layer(_stream).socket().shutdown(tcp::socket::shutdown_both, ec);

    // // not_connected happens sometimes so don't bother reporting it.
    // if(ec && ec != beast::errc::not_connected)
    //     return fail(ec, "shutdown");
}

void ClientSession::disconnect()
{
    beast::error_code ec;
    
    // Gracefully close the socket
    beast::get_lowest_layer(_stream).socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes so don't bother reporting it.
    if(ec && ec != beast::errc::not_connected)
        return fail(ec, "shutdown");
}

void ClientSession::setReqBasicAuth()
{
    // Credentials
    const std::string username = "admin";
    const std::string password = "123456";
    const std::string credentials = username + ":" + password;
    const std::string base64Credentials = utils::base64Encode(credentials);

    _req.set(http::field::authorization, "Basic " + base64Credentials);
}

// Report a failure
void ClientSession::fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";

    _streamState = StreamState::UnknownError;

    // Fulfill the promise with an error
    if (_promise) 
    {
        _promise->set_exception(std::make_exception_ptr(std::runtime_error(ec.message())));
        _promise.reset();
    }    
}

void ClientSession::closeConnectionAsync()
{
    // Gracefully close the SSL stream
    _stream.async_shutdown(
        beast::bind_front_handler(&ClientSession::on_shutdown, shared_from_this()));
}

void ClientSession::on_shutdown(beast::error_code ec) 
{
    if (ec && ec != boost::asio::error::eof) {
        // Log the shutdown error
        std::cerr << "Shutdown error: " << ec.message() << "\n";
    }

    // The connection is now fully closed; reconnect
    std::cout << "Connection closed. Reconnecting...\n";
    
    //reestablish_connection();
    connect();

    // Fulfill the promise with an error
    if (_promise) 
    {
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _startTime);

        _promise->set_value(latency);
        _promise.reset(); // Reset the promise for the next request
    }        
}

void ClientSession::reestablish_connection()
{
    // Start the connection process again
    _resolver.async_resolve(
        _req[http::field::host].to_string(), "443",
        beast::bind_front_handler(&ClientSession::onResolve, shared_from_this()));
}

} // namespace nvd
