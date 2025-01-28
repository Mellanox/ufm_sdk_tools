#include <istream>
#include <fstream>

#include <utils/system/FileSystem.h>

namespace fs = std::filesystem;

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
        fs::path dir(dirPath);

        if (fs::exists(dir))
        {
            if (!fs::is_directory(dir))
            {
                return ENOTDIR;
            }
        }
        else
        {
            fs::create_directories(dir);
        }
    }
    catch (const fs::filesystem_error& ex)
    {
        return ex.code().value();
    }
    
    return nvd::utils::Success;
}


bool isFileExists(const fs::path& file_path) noexcept
{
    try 
    {
        return fs::exists(file_path) && fs::is_regular_file(file_path);
    } 
    catch (...) 
    {
        return false; // Return false on any exception
    }
}
bool isFileEmpty(const std::string& filePath) noexcept
{
    fs::path file_path(filePath);
    try 
    {
        if (fs::exists(file_path) && fs::is_regular_file(file_path)) 
        {
            return fs::file_size(file_path) == 0;
        } 
        else 
        {
            std::cerr << "File does not exist or is not a regular file.\n";
            return false;
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return false;
    }
}

}} // namespace 

