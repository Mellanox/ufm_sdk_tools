#include <iostream>
#include <mutex>
#include <vector>
#include <chrono>
#include <algorithm>

#include <numeric>
#include <atomic>

class MetricsCollector
{
    std::mutex _mutex;
    std::vector<std::chrono::milliseconds> _latencies;
    
    std::atomic<uint64_t> _totalRequests = 0U;
    std::atomic<uint64_t> _totalResponses = 0U;
    std::atomic<uint64_t> _totalSuccessOK = 0U;
    std::atomic<uint64_t> _totalSuccessOther = 0U;
    std::atomic<uint64_t> _totalFailed = 0U;

     

public:

    void record_request()
    {
        ++_totalRequests;
    }

    void record_fail()
    {
        ++_totalFailed;
    }

    void record_response(std::chrono::milliseconds latency, uint32_t statusCode)
    {
        _latencies.push_back(latency);
        ++_totalResponses;

        if (statusCode == 200U)
        {
            ++_totalSuccessOK;
        }
        else
        {   
            // todo  - split to 3xx / 4xx
            ++_totalSuccessOther;
        }
    }

    // todo export to csv
    void print_metrics() 
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_latencies.empty()) {
            std::cout << "No responses received.\n";
            return;
        }

        auto total_latency = std::accumulate(_latencies.begin(), _latencies.end(), std::chrono::milliseconds(0));
        auto avg_latency = total_latency / _latencies.size();
        auto min_latency = *std::min_element(_latencies.begin(), _latencies.end());
        auto max_latency = *std::max_element(_latencies.begin(), _latencies.end());

        std::sort(_latencies.begin(), _latencies.end());
        auto p99_latency = _latencies[_latencies.size() * 99 / 100];

        std::cout << "Total Requests: " << _totalRequests << "\n";
        std::cout << "Total Responses: " << _totalResponses << "\n";
        std::cout << "Average Latency: " << avg_latency.count() << " ms\n";
        std::cout << "Min Latency: " << min_latency.count() << " ms\n";
        std::cout << "Max Latency: " << max_latency.count() << " ms\n";
        std::cout << "99th Percentile Latency: " << p99_latency.count() << " ms\n";
    }
};
