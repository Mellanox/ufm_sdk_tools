
#include "tools/rest_tester/Dispatcher.h"

#include "utils/logger/Logger.h"

// ./benchmark_tool ufm.azurehpc.core.azure-test.net /ufmRest/app/ufm_version 4 60
int main(int argc, char** argv)
{
    std::string logDir = "/tmp/log";
    nvd::Log::initializeLogger(spdlog::level::info, logDir, 10U, 1U);

    LOGINFO ("main start {}" , argv[0]);

    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <target> <num_connections> <runtime_seconds>\n";
        return 1;
    }

    std::string host = argv[1];
    std::string target = argv[2];
    std::string port = "443";

    int num_connections = std::stoi(argv[3]);
    size_t runtime_seconds = std::stoul(argv[4]);

    int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    nvd::Dispatcher dispatcher(host, port, target, version, num_connections);

    dispatcher.run(runtime_seconds);
    return 0;
}

