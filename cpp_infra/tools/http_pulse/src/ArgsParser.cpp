
#include <tools/http_pulse/ArgsParser.h>

namespace nvd {

void build_po(nvd::ProgramOptions& options)
{
    // todo - help should be in different desc / group
    options.add_arg<bool>("help,h", "", "Print Help");

    // http request params
    options.add_arg<std::string>("method", "m", "HTTP method: GET or POST (default: GET)", "GET");
    options.add_arg<std::string>("host", "ho", "destination IP / domain of the HTTP request (required)");
    options.add_arg<std::string>("port", "po", "HTTP port (default: 443)", "443");
    options.add_arg<std::string>("target", "u", "The target (query) of the HTTP request (required)");
    options.add_arg<std::string>("user", "u", "Server user and password (Optional)");
    options.add_arg<std::string>("cert", "c", "Client Certificates and public key files path (Optional)");
    options.add_arg<std::string>("token", "T", "Set token authrization.");
    options.add_arg<std::string>("body", "B", "HTTP body for POST requests");
    options.add_arg<int>("http_version", "hv", "HTTP version (default: 11)", 11);

    // bench params
    options.add_arg<size_t>("seconds", "sec", "Tets runtime duration in seconds");
    options.add_arg<int>("connections", "con", "Num connections (default: 1)", 1);
    options.add_arg<bool>("dryrun,dr", "", "dryrun. Run single test request (default: false)");
    options.add_arg<std::string>("connection-mode", "cm", "Set connection mode: 'new' or 'keep-alive' (default). to establish a new connection for each request, or keep-alive to reuse the connection.", "keep-alive");

    // output files
    options.add_arg<std::string>("metrics-path", "met", "The metrics output path as csv file (default: '/tmp/benchmark/')", "/tmp/benchmark/");
    options.add_arg<std::string>("name", "n", "Test name to store output files. Example: ${csv_path}/${name}.csv (default: 'pulse')", "pulse");
}

std::optional<nvd::HttpCommand> parseHttpParams(int argc, char** argv)
{
    nvd::ProgramOptions options("HttpPulse");
    
    build_po(options);

    // parse arguments 
    if (argc == 1 || options.get_value_or<bool>("help",false))
    {
        options.print_help();
        return std::nullopt;
    }

    if (!options.parse(argc, argv))
    {
        return std::nullopt; // Parsing failed
    }

    // retrieve values
    auto host = options.get_value<std::string>("host", true);
    auto target = options.get_value<std::string>("target", true);
    auto runtime_seconds = options.get_value<size_t>("seconds");

    auto num_connections = options.get_value<int>("connections");
    auto port = options.get_value<std::string>("port");
    auto version = options.get_value<int>("http_version");
    auto metrics_out_path = options.get_value<std::string>("metrics-path");
    auto name = options.get_value<std::string>("name");
    
    auto user = options.get_value<std::string>("user");
    auto cert_path = options.get_value<std::string>("cert");
    auto dryrun = options.get_value<bool>("dryrun");
    
    auto token = options.get_value<std::string>("token");
    auto body = options.get_value<std::string>("body");
    auto connection_mode = options.get_value<std::string>("connection-mode");

    if (!host || !target || !num_connections || !connection_mode || !port || !version || !metrics_out_path || !name || !dryrun) return std::nullopt;
    return nvd::HttpCommand{*host, *port, *target, *num_connections, *version, *metrics_out_path, *name, *connection_mode, *dryrun, runtime_seconds, user, token, cert_path, body};
}

nvd::AuthMethod getAuthMethod(const nvd::HttpCommand& args)
{
    if (args.user) return AuthMethod::BASIC;
    else if (args.token) return AuthMethod::TOKEN;
    else if (args.certPath) return AuthMethod::SSL_CERTIFICATE;

    return AuthMethod::UNKNOWN;
}

} // namespace