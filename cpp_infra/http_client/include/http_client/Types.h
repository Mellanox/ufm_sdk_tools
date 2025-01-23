#pragma once

namespace nvd {

enum class AuthMethod
{
    BASIC,
    SSL_CERTIFICATE
};


using rep_t = std::chrono::system_clock::rep;

using period_t = std::nano;
        
using duration_t = std::chrono::duration<rep_t, period_t>;

struct Result
{
    uint32_t statusCode;
    std::string payload;
    duration_t latency;
};

} // namespace nvd