
#include "tools/http_pulse/Dispatcher.h"

#include "utils/logger/Logger.h"

#include <tools/http_pulse/ArgsParser.h>

// Usage Examples:
// -- Basic auth w credentia:l
// ./pulse --host 10.237.169.185 --target /ufmRest/app/ufm_version --user admin:123456 [--seconds 10]
// 
// -- Client Cert auth :
// ./pulse --host ufm.azurehpc.core.azure-test.net --target /ufmRest/app/ufm_version --cert /tmp/ [--seconds 10]
//
// -- POST Token request (get the token):
// ./pulse --method POST --host 10.237.169.185 --target /ufmRest/app/tokens --user admin:123456
// ./pulse --host 10.237.169.185 --target /ufmRest/app/users --name bm_token_auth --token ZfNpj7lU1G1KUmf88KHZXFGLpi8F26 --connection-mode keep-alive --seconds 60
int main(int argc, char** argv)
{
    std::string logDir = "/tmp/log";
    nvd::Log::initializeLogger(spdlog::level::info, logDir, 10U, 1U);

    auto httpParams = nvd::parseHttpParams(argc, argv);

    if (!httpParams)
    {
        return 1;
    }

    auto runtime_seconds = httpParams->runtime_seconds ? *httpParams->runtime_seconds : 0;
    LOGINFO("start {} : host '{}', port '{}', target '{}', seconds '{}', connection '{}', version '{}'", argv[0], httpParams->host, httpParams->port, httpParams->target, runtime_seconds, httpParams->num_connections, httpParams->version);

    nvd::Dispatcher dispatcher(nvd::getAuthMethod(*httpParams), *httpParams);
    dispatcher.start();
    return 0;
}
