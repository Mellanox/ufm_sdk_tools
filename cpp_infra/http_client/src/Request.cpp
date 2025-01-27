#include <http_client/Request.h>
#include <http_client/Utils.h>

#include <boost/beast/version.hpp>

namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

namespace nvd {

void Request::create(http::verb method,
                     const std::string& target,
                     const std::string& host,
                     AuthMethod authMethod,
                     int version)
{

    // http::field::host
    // req[http::field::host].to_string()

    _req.version(version);
    _req.method(method);
    _req.target(target);
    _req.set(http::field::connection, "keep-alive");
    _req.set(http::field::host, host);

    if (authMethod == AuthMethod::SSL_CERTIFICATE)
    {
        _req.set(http::field::content_type, "application/json"); // Add Content-Type header
    }

    _req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}

void Request::setUserAgent(const std::string& userAgent)
{
    _req.set(http::field::user_agent, userAgent);
}

void Request::setBody(const std::string& body)
{
    _req.set(http::field::body, body);
}

void Request::setAuthorization(const std::string& credentials, bool base64encode)
{
    const std::string base64Credentials = utils::base64Encode(credentials);
    _req.set(http::field::authorization, "Basic " + base64Credentials);
}


} // namespace
