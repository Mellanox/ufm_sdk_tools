#pragma once     

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <http_client/Types.h>

namespace nvd {

// todo - add api to set 
// - user_agent
// - 
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

    /// @brief create request with body
    void create(HttpVerb method,
                const std::string& target,
                const std::string& host,
                const std::string& body,
                AuthMethod authMethod = AuthMethod::BASIC,
                int version = 11);


    const StringBodyRequestType& get() const {return _req;}
    const StringBodyRequestType& operator*() const {return _req;}

protected:

    void setReqBasicAuth();

private:

    // todo 
    // std::optional<RequestType> _request;
    StringBodyRequestType _req;
};

} // namespace