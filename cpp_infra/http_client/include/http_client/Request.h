#pragma once     

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <http_client/Types.h>

namespace nvd {

/// @brief encapsulate http request, 
///        provide API to build request with required fields
class Request
{
public:
    using HttpVerb = boost::beast::http::verb;
    using StringBody = boost::beast::http::string_body;
    using EmptyBody = boost::beast::http::empty_body;

    using StringBodyRequestType = boost::beast::http::request<StringBody>;
    using EmptyBodyRequestType = boost::beast::http::request<EmptyBody>;

    using RequestType = std::variant<EmptyBodyRequestType, StringBodyRequestType>;

    /// @brief create empty request (wo body)
    void create(HttpVerb method,
                const std::string& target,
                const std::string& host,
                AuthMethod authMethod = AuthMethod::BASIC,
                int version = 11);

    /// @brief set request body field
    void setBody(const std::string& body);

    /// @brief set request 'user_agent' field
    void setUserAgent(const std::string& userAgent);

    /// @brief set request credentials
    /// @param credentials string formated as 'user:pass'
    void setAuthorization(const std::string& credentials, bool base64encode = true);

    const StringBodyRequestType& get() const {return _req;}
    const StringBodyRequestType& operator*() const {return _req;}

private:

    // todo  use optional or std::variant to decide on the real type on constructions (w/empty body)
    // std::optional<RequestType> / std::variant<StringBodyRequestType, StringBodyRequestType>
    StringBodyRequestType _req;
};

} // namespace