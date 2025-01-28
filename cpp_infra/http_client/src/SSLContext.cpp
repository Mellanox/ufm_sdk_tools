


#include <http_client/SSLContext.h>

#include <utils/logger/Logger.h>

#include <filesystem>

namespace nvd {

SSLContext::SSLContext(AuthMethod authMethod, std::optional<std::string> user, std::optional<std::string> certPath) :
    _sslContext(ssl::context::tlsv12_client)
{
    if (authMethod == AuthMethod::BASIC)
    {
        if (user)
        {
            _sslContext.set_verify_mode(ssl::verify_none);
            LOGINFO("Construct SSL Context using BASIC Authentication Method");
        }
        else
        {
            LOGERROR("Missing user credential, for using SSL BASIC Authentication");
        }
    }
    else if (authMethod == AuthMethod::SSL_CERTIFICATE)
    {
        if (certPath)
        {
            // files must exists in path : certificate.crt , private-key.pem
            auto certificate = std::filesystem::path(*certPath) / "certificate.crt";
            auto privateKey = std::filesystem::path(*certPath) / "private-key.pem";

            // Load client certificate and private key
            _sslContext.use_certificate_file(certificate.string(), ssl::context::pem);
            _sslContext.use_private_key_file(privateKey.string(), ssl::context::pem);
            
            LOGINFO("Construct SSL Context using 'SSL_CERTIFICATE' Authentication Method. {} ", certificate.string(), privateKey.string());
        }
        else
        {
            LOGERROR("Missing certificates path, for using SSL Client Certificate Authentication");
        }

    }
    else if (authMethod != AuthMethod::TOKEN)
    {
        LOGWARN("Unwown Authentication Method.");
    }
}

ssl::context& SSLContext::get() const
{
    return _sslContext;
}

} // namespace