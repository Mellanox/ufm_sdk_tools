
#pragma once 

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/ssl.hpp>

#include <http_client/Types.h>

namespace ssl = boost::asio::ssl;          // From <boost/asio/ssl.hpp>

namespace nvd {

/// encapsualtes the boost asio SSL context 
class SSLContext
{
public:
    explicit SSLContext(AuthMethod authMethood);
    
    ssl::context& get() const;
    
private:
    mutable ssl::context _sslContext;
};

} // namespace