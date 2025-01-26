#include <http_client/Request.h>
#include <http_client/Utils.h>

#include <boost/beast/version.hpp>

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

    if (authMethod == AuthMethod::BASIC)
    {
        setReqBasicAuth();
    }
    else if (authMethod == AuthMethod::SSL_CERTIFICATE)
    {
        _req.set(http::field::content_type, "application/json"); // Add Content-Type header
    }

    _req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}


void Request::create(http::verb method,
                     const std::string& target,
                     const std::string& host,
                     const std::string& body,
                     AuthMethod authMethod,
                     int version)
{
    // todo - create with body
}

void Request::setReqBasicAuth()
{
    // Credentials - todo get from config or API
    const std::string username = "admin";
    const std::string password = "123456";
    const std::string credentials = username + ":" + password;
    const std::string base64Credentials = utils::base64Encode(credentials);

    _req.set(http::field::authorization, "Basic " + base64Credentials);
}

} // namespace
