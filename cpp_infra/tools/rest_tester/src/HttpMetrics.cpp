
#include <tools/rest_tester/HttpMetrics.h>

#include <fmt/core.h>

namespace nvd {

HttpMetrics::HttpMetrics(std::string target, size_t tmInSec, const std::string& filePath, const std::string& testName) :
    _target(std::move(target)),
    _tmInSec(tmInSec)
{
    _csvPath = std::filesystem::path(filePath) / testName;

    // open new file (wo append mode)
    cmn::CsvWriter writer(_csvPath.string(), true);
}

void HttpMetrics::to_stream(std::ostream& ostr) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_metrics.latencies.empty()) {
        ostr << "No responses received.\n";
        return;
    }

    auto total_latency = std::accumulate(_metrics.latencies.begin(), _metrics.latencies.end(), std::chrono::milliseconds(0));
    auto avg_latency = total_latency / _metrics.latencies.size();
    auto min_latency = *std::min_element(_metrics.latencies.begin(), _metrics.latencies.end());
    auto max_latency = *std::max_element(_metrics.latencies.begin(), _metrics.latencies.end());

    auto metrics_latencies = _metrics.latencies;
    std::sort(metrics_latencies.begin(), metrics_latencies.end());
    
    auto p99_latency = metrics_latencies[metrics_latencies.size() * 99 / 100];

    ostr << "Total Requests: " << _metrics._totalRequests << "\n";
    ostr << "Total Responses: " << _metrics._totalResponses << "\n";
    ostr << "Average Latency: " << avg_latency.count() << " ms\n";
    ostr << "Min Latency: " << min_latency.count() << " ms\n";
    ostr << "Max Latency: " << max_latency.count() << " ms\n";
    ostr << "99th Percentile Latency: " << p99_latency.count() << " ms\n";
}

std::tuple<double, double> 
HttpMetrics::statLatency() const
{
    auto total_latency = std::accumulate(_metrics.latencies.begin(), _metrics.latencies.end(), std::chrono::milliseconds(0));
    auto avg_latency = total_latency / _metrics.latencies.size();
    double seconds = static_cast<double>(avg_latency.count()) / 1000.0;
    LOGINFO("Avg Latency in milli {}, in sec {}", avg_latency.count(), seconds);

    double ReqPS = static_cast<double>(_metrics._totalRequests.load()) / static_cast<double>(_tmInSec);
    LOGINFO("ReqPS  {} :  {} / {} ", ReqPS, _metrics._totalRequests.load(), _tmInSec);
    return {seconds, ReqPS};
}

void HttpMetrics::to_csv(bool isNew)
{
    std::lock_guard<std::mutex> lock(_mutex);

    cmn::CsvWriter writer(_csvPath.string(), isNew);

    std::vector<std::string> metricsCollection;
    
    double avgLatency, ReqPS;
    std::tie(avgLatency, ReqPS) = statLatency();

    metricsCollection.push_back(_target);
    metricsCollection.push_back(fmt::format("{:.3g}", avgLatency));
    metricsCollection.push_back(fmt::format("{:.2g}", ReqPS));

    writer << metricsCollection;
}
size_t HttpMetrics::runtimeInSec() const
{
    return _tmInSec;
}

} // namespace