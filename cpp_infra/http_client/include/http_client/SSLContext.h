
#pragma once 

#include <optional>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/ssl.hpp>

#include <http_client/Types.h>

namespace ssl = boost::asio::ssl;          // From <boost/asio/ssl.hpp>

namespace nvd {

/// @brief encapsualtes boost ssl::context  
///        enable SSL/TLS communication (secure HTTP over https)
class SSLContext
{
public:
    explicit SSLContext(AuthMethod authMethod,
                        std::optional<std::string> user, 
                        std::optional<std::string> certPath = std::nullopt);
    
    ssl::context& get() const;
    
private:
    mutable ssl::context _sslContext;
};

} // namespace