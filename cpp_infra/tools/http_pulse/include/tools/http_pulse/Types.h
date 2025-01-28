#pragma once 

#include <string>
#include <optional>

namespace nvd {

struct HttpCommand
{
    std::string host;
    std::string port;
    std::string target;
    int num_connections;    
    int version;

    std::string metrics_out_path;
    std::string name;

    mutable bool dry_run;

    // Optional parameteres
    std::optional<size_t> runtime_seconds;
    std::optional<std::string> user;
    std::optional<std::string> token;
    std::optional<std::string> certPath;
    std::optional<std::string> body;
};

} // namespace nvd