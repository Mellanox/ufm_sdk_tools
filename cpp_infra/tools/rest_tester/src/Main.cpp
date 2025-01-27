
#include "tools/rest_tester/Dispatcher.h"

#include "utils/configuration/ProgramOptions.h"

#include "utils/logger/Logger.h"

// temp - should be build from config
void build_po(nvd::ProgramOptions& options)
{
    // Define command-line arguments
    options.add_arg<std::string>("host", "ho", "destination IP / domain of the HTTP request (required)");
    options.add_arg<std::string>("url", "u", "The URL of the HTTP request (required)");
    options.add_arg<size_t>("seconds", "sec", "Tets runtime duration in seconds (required)");

    options.add_arg<int>("connections", "con", "Num connections (default: 1)", 1);
    options.add_arg<std::string>("method", "m", "HTTP method: GET or POST (default: GET)", "GET");
    options.add_arg<std::string>("body", "bo", "HTTP body for POST requests", "");
    options.add_arg<int>("http_version", "hv", "HTTP version (default: 11)", 11);
    options.add_arg<std::string>("port", "po", "HTTP port (default: 443)", "443");
}

struct HttpInput
{
    std::string host;
    std::string port;
    std::string url;
    int num_connections;
    size_t runtime_seconds;
    int version;
    nvd::AuthMethod authMethod;
};

std::optional<HttpInput> parseHttpParams(int argc, char** argv)
{
    nvd::ProgramOptions options("HttpClientBM");    
    build_po(options);

    // parse arguments
    if (!options.parse(argc, argv))
    {
        return std::nullopt; // Parsing failed
    }

    // retrieve values
    auto host = options.get_value<std::string>("host");
    auto url = options.get_value<std::string>("url");
    auto num_connections = options.get_value<int>("connections");
    auto runtime_seconds = options.get_value<size_t>("seconds");
    auto port = options.get_value<std::string>("port");
    auto version = options.get_value<int>("http_version");

    //std::string method = options.get_value("method").empty() ? "GET" : options.get_value("method");
    //std::string body = options.get_value("body");

    if (!host || !url || !num_connections || !runtime_seconds || !port || !version) return std::nullopt;
    
    return HttpInput{*host, *port, *url, *num_connections, *runtime_seconds, *version, nvd::AuthMethod::BASIC};
}


// ./benchmark_tool ufm.azurehpc.core.azure-test.net /ufmRest/app/ufm_version 4 60
int main(int argc, char** argv)
{
    std::string logDir = "/tmp/log";
    nvd::Log::initializeLogger(spdlog::level::info, logDir, 10U, 1U);

    auto httpParams = parseHttpParams(argc, argv);

    if (!httpParams)
    {
        return 1;
    }

    LOGINFO("start {} : host '{}', port '{}', url '{}', seconds '{}', connection '{}', version '{}'", argv[0], httpParams->host, httpParams->port, httpParams->url, httpParams->runtime_seconds, httpParams->version, httpParams->num_connections);
    nvd::Dispatcher dispatcher(httpParams->host, httpParams->port, httpParams->url, httpParams->runtime_seconds, httpParams->version, httpParams->num_connections, httpParams->authMethod);
    dispatcher.start();
    return 0;
}

