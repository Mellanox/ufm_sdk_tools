


#include <http_client/SSLContext.h>

#include <utils/logger/Logger.h>

namespace nvd {

SSLContext::SSLContext(AuthMethod authMethood) :
    _sslContext(ssl::context::tlsv12_client)
{
    if (authMethood == AuthMethod::BASIC)
    {
        _sslContext.set_verify_mode(ssl::verify_none);
        LOGINFO("Construct SSL Context using BASIC Authentication Method");
    }
    else if (authMethood == AuthMethod::SSL_CERTIFICATE)
    {
        // Load client certificate and private key
        _sslContext.use_certificate_file("/tmp/certificate.crt", ssl::context::pem);
        _sslContext.use_private_key_file("/tmp/private-key.pem", ssl::context::pem);
        LOGINFO("Construct SSL Context using 'SSL_CERTIFICATE' Authentication Method");
    }
    else
    {
        LOGWARN("Unwown Authentication Method.");
    }
}

ssl::context& SSLContext::get() const
{
    return _sslContext;
}

} // namespace