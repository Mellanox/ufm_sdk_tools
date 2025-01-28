#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include <numeric>
#include <atomic>

#include "Types.h"

namespace nvd {
namespace http {

struct Metrics
{
    std::vector<std::chrono::milliseconds> latencies;
    
    std::atomic<uint64_t> _totalRequests = 0U;
    std::atomic<uint64_t> _totalResponses = 0U;
    std::atomic<uint64_t> _totalSuccessOK = 0U;
    std::atomic<uint64_t> _totalClientError = 0U;
    std::atomic<uint64_t> _totalServerError = 0U;
    std::atomic<uint64_t> _totalOtherError = 0U;

    void clear()
    {
        _totalRequests.store(0U);
        _totalResponses.store(0U);
        _totalSuccessOK.store(0U);
        _totalClientError.store(0U);
        _totalServerError.store(0U);
        _totalOtherError.store(0U);
    }
};

class MetricsCollector
{
public:

    void record_request()
    {
        ++_metrics._totalRequests;
    }

    void record_fail(ErrorCode statusCode)
    {
        if (statusCode == ErrorCode::ClientError)
        {   
            ++_metrics._totalClientError;
        }
        else if (statusCode == ErrorCode::ServerError)
        {   
            ++_metrics._totalServerError;
        }
        else
        {
            ++_metrics._totalOtherError;
        }
    }

    void record_response(std::chrono::milliseconds latency, ErrorCode statusCode)
    {
        _metrics.latencies.push_back(latency);
        ++_metrics._totalResponses;

        if (statusCode == ErrorCode::Success)
        {
            ++_metrics._totalSuccessOK;
        }
        else
        {
            record_fail(statusCode);
        }
    }
    
    void clear() {_metrics.clear();}
    
    const Metrics& metrics() const {return _metrics;}

protected:

    Metrics _metrics;
};

}} // namespace