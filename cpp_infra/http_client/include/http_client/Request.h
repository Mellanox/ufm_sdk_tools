#pragma once     

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <http_client/Types.h>

namespace nvd {

namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class Request
{
public:

    using StringBody = http::string_body;
    using EmptyBody = http::empty_body;

    using StringBodyRequestType = http::request<StringBody>;
    using EmptyBodyRequestType = http::request<EmptyBody>;

    using RequestType = std::variant<EmptyBodyRequestType, StringBodyRequestType>;

    /// @brief create empty request (wo body)
    void create(http::verb method,
                const std::string& target,
                const std::string& host,
                AuthMethod authMethod = AuthMethod::BASIC,
                int version = 11);

    /// @brief create request with body
    void create(http::verb method,
                const std::string& target,
                const std::string& host,
                const std::string& body,
                AuthMethod authMethod = AuthMethod::BASIC,
                int version = 11);


    const http::request<http::string_body>& get() const {return _req;}
    const http::request<http::string_body>& operator*() const {return _req;}

protected:

    void setReqBasicAuth();

private:

    // todo 
    // std::optional<RequestType> _request;
    http::request<http::string_body> _req;
};

} // namespace