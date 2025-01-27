
#include <utils/configuration/ProgramOptions.h>

namespace po = boost::program_options;

namespace nvd {

ProgramOptions::ProgramOptions(std::string description) : _desc(description) 
{
}

void ProgramOptions::print_help() const
{
    std::cout << _desc << std::endl;
}

bool ProgramOptions::parse(int argc, char* argv[])
{
    try
    {
        store(po::parse_command_line(argc, argv, _desc), _vmap);
        notify(_vmap); // Validates required options
        return true;
    } 
    catch (const po::error& e)
    {
        std::cerr << "Error: " << e.what() << "\n\n" << _desc << std::endl;
        return false;
    }
}

} // namespace