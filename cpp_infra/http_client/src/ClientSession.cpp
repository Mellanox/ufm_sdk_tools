
#include "http_client/ClientSession.h"

#include <boost/asio/strand.hpp>

#include <http_client/Utils.h>
#include <utils/logger/Logger.h>

namespace nvd {

std::atomic<uint64_t> ClientSession::_nextId{0};

namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

//------------------------------------------------------------------------------

// Objects are constructed with a strand to
// ensure that handlers do not execute concurrently.
ClientSession::ClientSession(net::io_context& ioc, 
                             ssl::context& sslCtx,
                             const std::string& host, const std::string& port,
                             AuthMethod authMethod) :
    _id{++_nextId},
    _sslCcontext(sslCtx),
    _resolver(net::make_strand(ioc)),
    _stream(net::make_strand(ioc), sslCtx),
    _host(host),
    _port(port),
    _authMethod(authMethod),
    _streamState(StreamState::Closed)
{
}

ClientSession::~ClientSession()
{
    if (_streamState == StreamState::Connected)
    {
        disconnect();
    }
}

void ClientSession::connect()
{
    // todo - add error checks 

    // Resolve the host and port
    auto const results = _resolver.resolve(_host, _port);

    // Connect to the server
    beast::get_lowest_layer(_stream).connect(results);

    // Perform SSL handshake
    _stream.handshake(ssl::stream_base::client);

    _streamState = StreamState::Connected;

    LOGINFO("ClientSession connect Succeeded");
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
    return _streamState == StreamState::Connected && beast::get_lowest_layer(_stream).socket().is_open();
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

std::optional<Response> ClientSession::sendRequest(const Request& req)
{
    http::response_parser<http::string_body> response_parser;
    response_parser.body_limit(boost::none); // unlimit body size

    if (!isConnected())
    {
        reconnect();
    }
    
    try
    {
        // record the start time
        auto startTime = std::chrono::steady_clock::now();

        boost::system::error_code ec;

        http::write(_stream, *req, ec);

        if(ec)
        {
            LOGERROR("Write failed. ec : {}", ec.what());
            std::cerr << "Error :" << ": " << ec.what() << "\n";
            return std::nullopt;
        }

        // Use the custom parser with no limit for reading
        http::read(_stream, _buffer, response_parser, ec);

        if(ec) 
        {
            LOGERROR("Read response failed. ec : {}", ec.what());
            std::cerr << "Error :" << ": " << ec.what() << "\n";
            return std::nullopt;
        }

        // get ownership of the message 
        auto response = response_parser.release();

        // Check for Connection: close
        if (response[http::field::connection] == "close") 
        {
            std::cerr << "Server closed the connection. Reconnecting...\n";
            _streamState = StreamState::Closed;
        }

        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime);

        return Response{responseResultToErrorCode(response), response.body(), latency};
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error :" << ": " << e.what() << "\n";
    }
    return std::nullopt;
}

std::future<Response> ClientSession::sendRequestAsync(const Request& req)
{
    _promise.emplace();

    // reset the response
    _response = http::response<http::string_body>();

    // Record the start time
    _startTime = std::chrono::steady_clock::now();

    auto future = _promise->get_future();

    // Send the HTTP request to the remote host
    http::async_write(
        _stream, *req,
        beast::bind_front_handler(&ClientSession::on_write, shared_from_this()));
    
    return future;
}

void ClientSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    //std::cout << " on_write : bytes_transferred " << bytes_transferred << std::endl;

    // todo alon - check this
    http::response_parser<http::string_body> response_parser;
    response_parser.body_limit(boost::none); // Set unlimit the responser body

    
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
        _promise->set_value({responseResultToErrorCode(_response), std::move(_response.body()), latency});
        _promise.reset(); // Reset the promise for the next request
    }

    // // Gracefully close the socket
    // beast::get_lowest_layer(_stream).socket().shutdown(tcp::socket::shutdown_both, ec);

    // // not_connected happens sometimes so don't bother reporting it.
    // if(ec && ec != beast::errc::not_connected)
    //     return fail(ec, "shutdown");
}

ErrorCode ClientSession::responseResultToErrorCode(const StringBodyResponseType& response) const
{
    auto status = response.result(); // Get the HTTP status code from the response

    // Map HTTP status codes to the ErrorCode enum
    if (status == http::status::ok || status == http::status::created) 
    {
        return ErrorCode::Success; // Success (200 or 201)
    } 
    else if (status == http::status::bad_request) 
    {
        return ErrorCode::BadRequest; // 400
    } 
    else if (status == http::status::unauthorized) 
    {
        return ErrorCode::Unauthorized; // 401
    } 
    else if (status == http::status::forbidden) 
    {
        return ErrorCode::Forbidden; // 403
    } 
    else if (status == http::status::not_found) 
    {
        return ErrorCode::NotFound; // 404
    } 
    else if (status == http::status::request_timeout) 
    {
        return ErrorCode::Timeout; // 408
    } 
    else if (status == http::status::too_many_requests) 
    {
        return ErrorCode::TooManyRequests; // 429
    } 
    else if (status == http::status::found) 
    {
        return ErrorCode::FoundTempRedirect; // 302
    } 
    else if (status >= http::status::bad_request && status < http::status::internal_server_error) 
    {
        return ErrorCode::ClientError; // 4xx other than above
    } 
    else if (status >= http::status::internal_server_error) 
    {
        return ErrorCode::ServerError; // 5xx
    }
    
    LOGINFO("response result UnknownError : {}", static_cast<int>(response.result()));
    return ErrorCode::UnknownError; // Any other status
}

void ClientSession::disconnect()
{
    beast::error_code ec;
    
    // Gracefully close the socket
    beast::get_lowest_layer(_stream).socket().shutdown(tcp::socket::shutdown_both, ec);

    _streamState = StreamState::Closed;

    // not_connected happens sometimes so don't bother reporting it.
    if(ec && ec != beast::errc::not_connected)
        return fail(ec, "shutdown");
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

    // todo - check this flow if need to reconnect
    // The connection is now fully closed; reconnect
    LOGINFO("on_shutdown. Reconnecting..");
    
    //reestablish_connection();
    //connect();

    // Fulfill the promise with an error
    if (_promise) 
    {
        LOGERROR("on_shutdown. Response Promise was expected to be nullopt.");
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _startTime);
        _promise->set_value({ErrorCode::UnknownError, {}, latency});
        _promise.reset(); // Reset the promise for the next request
    }        
}

void ClientSession::reestablish_connection()
{
    // Start the connection process again
    _resolver.async_resolve(
        _host, "443",
        beast::bind_front_handler(&ClientSession::onResolve, shared_from_this()));
}

} // namespace nvd
