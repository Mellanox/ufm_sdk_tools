#pragma once

#include <string>

namespace nvd {
namespace utils {

/// @brief encode input string into the Base64 format.
///        It is required for Basic Authentication. It is equivalent to -u user:pass in the curl command.
/// @param in input string to encode
std::string base64Encode(const std::string &in);

}} // namespace nvd::utils