#include <istream>
#include <fstream>

#include <utils/os/FileSystem.h>

#include <boost/filesystem.hpp>

namespace nvd {
namespace utils {

/**
 * Creates a directory if does not exist.
 *
 * @param dirPath               IN  the directory path.
 *
 * @return Success if the directory exists or if it was created successfully,
 * Error if the path point to file or if directory creation has failed.
 */
nvd::utils::Result createDirectory(const std::string& dirPath) noexcept
{
    try
    {
        boost::filesystem::path dir(dirPath);

        if (boost::filesystem::exists(dir))
        {
            if (!boost::filesystem::is_directory(dir))
            {
                return ENOTDIR;
            }
        }
        else
        {
            boost::filesystem::create_directories(dir);
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        return ex.code().value();
    }
    
    return nvd::utils::Success;
}


}} // namespace 

