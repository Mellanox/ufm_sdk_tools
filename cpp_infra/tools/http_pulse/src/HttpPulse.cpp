
#include "tools/http_pulse/Dispatcher.h"

#include "utils/configuration/ProgramOptions.h"

#include "utils/logger/Logger.h"

void build_po(nvd::ProgramOptions& options)
{
    options.add_arg<bool>("help", "h", "Print Help", false);

    // http request params
    options.add_arg<std::string>("host", "ho", "destination IP / domain of the HTTP request (required)");
    options.add_arg<std::string>("url", "u", "The URL of the HTTP request (required)");
    options.add_arg<std::string>("user", "u", "Server user and password (Optional)");
    options.add_arg<std::string>("cert", "c", "Client Certificates and public key files path (Optional)");
    options.add_arg<std::string>("method", "m", "HTTP method: GET or POST (default: GET)", "GET");
    options.add_arg<std::string>("body", "bo", "HTTP body for POST requests", "");
    options.add_arg<std::string>("port", "po", "HTTP port (default: 443)", "443");
    options.add_arg<int>("http_version", "hv", "HTTP version (default: 11)", 11);

    // bench params
    options.add_arg<size_t>("seconds", "sec", "Tets runtime duration in seconds (required)");
    options.add_arg<int>("connections", "con", "Num connections (default: 1)", 1);
    options.add_arg<bool>("dry_run", "dr", "dry_run. Run single test request (default: false)", false);

    // output files
    options.add_arg<std::string>("metrics_out_path", "meto", "The metrics output path as csv file (default: '/tmp/benchmark/')", "/tmp/benchmark/");
    options.add_arg<std::string>("name", "n", "Test name to store output files. Example: ${csv_path}/${name}.csv (default: 'pulse')", "pulse");
}

std::optional<nvd::HttpCommand> parseHttpParams(int argc, char** argv)
{
    nvd::ProgramOptions options("HttpPulse");
    build_po(options);

    // parse arguments
    if (argc == 1 || options.get_value<bool>("help") || !options.parse(argc, argv))
    {
        return std::nullopt; // Parsing failed
    }

    // retrieve values
    auto host = options.get_value<std::string>("host", true);
    auto url = options.get_value<std::string>("url", true);
    auto runtime_seconds = options.get_value<size_t>("seconds", true);

    auto num_connections = options.get_value<int>("connections");
    auto port = options.get_value<std::string>("port");
    auto version = options.get_value<int>("http_version");
    auto metrics_out_path = options.get_value<std::string>("metrics_out_path");
    auto name = options.get_value<std::string>("name");
    auto user = options.get_value<std::string>("user");
    auto cert_path = options.get_value<std::string>("cert");
    auto dry_run = options.get_value<bool>("dry_run");

    // todo
    //std::string method = options.get_value("method").empty() ? "GET" : options.get_value("method");
    //std::string body = options.get_value("body");

    if (!host || !url || !num_connections || !runtime_seconds || !port || !version || !metrics_out_path || !name || !dry_run) return std::nullopt;
    return nvd::HttpCommand{*host, *port, *url, *num_connections, *runtime_seconds, *version, *metrics_out_path, *name, *dry_run, user, cert_path};
}

// Examples:
// -- Basic auth w credentia:l
// ./pulse --host 10.237.169.185 --url /ufmRest/app/ufm_version --seconds 10 --user admin:123456
// 
// -- Client Cert auth :
// ./pulse --host ufm.azurehpc.core.azure-test.net --url /ufmRest/app/ufm_version --seconds 10 --cert /tmp/

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

    nvd::Dispatcher dispatcher(*httpParams);
    dispatcher.start();
    return 0;
}
