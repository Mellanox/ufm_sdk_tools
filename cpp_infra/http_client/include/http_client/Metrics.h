#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include <numeric>
#include <atomic>

namespace nvd {
namespace http {

struct Metrics
{
    std::vector<std::chrono::milliseconds> latencies;
    
    std::atomic<uint64_t> _totalRequests = 0U;
    std::atomic<uint64_t> _totalResponses = 0U;
    std::atomic<uint64_t> _totalSuccessOK = 0U;
    std::atomic<uint64_t> _totalSuccessOther = 0U;
    std::atomic<uint64_t> _totalFailed = 0U;

    void clear()
    {
        _totalRequests.store(0U);
        _totalResponses.store(0U);
        _totalSuccessOK.store(0U);
        _totalSuccessOther.store(0U);
        _totalFailed.store(0U);
    }
};

class MetricsCollector
{
public:

    void record_request()
    {
        ++_metrics._totalRequests;
    }

    void record_fail()
    {
        ++_metrics._totalFailed;
    }

    void record_response(std::chrono::milliseconds latency, uint32_t statusCode)
    {
        _metrics.latencies.push_back(latency);
        ++_metrics._totalResponses;

        if (statusCode == 200U)
        {
            ++_metrics._totalSuccessOK;
        }
        else
        {   
            // todo  - split to 3xx / 4xx
            ++_metrics._totalSuccessOther;
        }
    }
    
    void clear() {_metrics.clear();}
    const Metrics& metrics() const {return _metrics;}

protected:

    Metrics _metrics;
};

}} // namespace