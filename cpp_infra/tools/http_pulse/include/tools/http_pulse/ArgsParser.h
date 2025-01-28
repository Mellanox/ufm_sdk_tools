#pragma once

#include <string>
#include <optional>

#include "Types.h"

#include <http_client/Types.h>

#include "utils/configuration/ProgramOptions.h"

namespace nvd {

void build_po(nvd::ProgramOptions& options);

std::optional<nvd::HttpCommand> parseHttpParams(int argc, char** argv);

/// @brief get Auth methid given the input Args
/// @return auth method
nvd::AuthMethod getAuthMethod(const nvd::HttpCommand& args);

} // namespace