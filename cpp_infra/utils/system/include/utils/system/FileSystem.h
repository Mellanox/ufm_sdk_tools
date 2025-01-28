 
#pragma once

#include <string>

#include <errot_handling/Types.h>

#include <filesystem>

namespace nvd {
namespace utils {

/// @brief Creates a directory if does not exist.
nvd::utils::Result createDirectory(const std::string& dirPath) noexcept;

bool isFileExists(const std::filesystem::path& file_path) noexcept;

bool isFileEmpty(const std::string& filePath) noexcept;

}} // namespace 

