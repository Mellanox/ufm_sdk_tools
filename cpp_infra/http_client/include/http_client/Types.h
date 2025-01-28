#pragma once

#include <utils/types/Time.h>

namespace nvd {

enum class AuthMethod
{
    BASIC,
    TOKEN,
    SSL_CERTIFICATE,
    UNKNOWN
};

/// @brief define Token Auth Scheme
enum class TokenAuthScheme
{
    BASIC,
    BEARER
};

enum class ErrorCode : uint32_t
{
    Success = 0,            // Request was successful
    ClientError = 1,        // 4xx errors (Client-side issue)
    ServerError = 2,        // 5xx errors (Server-side issue)
    NotFound = 3,           // 404 Not Found
    Unauthorized = 4,       // 401 Unauthorized
    Forbidden = 5,          // 403 Forbidden
    BadRequest = 6,         // 400 Bad Request
    Timeout = 7,            // 408 Request Timeout
    TooManyRequests = 8,    // 429 Too Many Requests
    FoundTempRedirect = 9,  // 302 found, ok
    UnknownError            // Other unexpected errors
};


struct Response
{
    ErrorCode statusCode;
    std::string payload;
    time::DurationT latency;
};


} // namespace nvd