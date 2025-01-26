 
#pragma once

#include <string>

#include <errot_handling/Types.h>

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
nvd::utils::Result createDirectory(const std::string& dirPath) noexcept;

}} // namespace 

