
#pragma once

#include <http_client/Metrics.h>

#include <utils/metrics/CsvWriter.h>

#include <mutex>
#include <string>
#include <filesystem>

namespace nvd {

/// @brief  extend the http::MetricsCollector for the http perf-tool statistics
class HttpMetrics : public http::MetricsCollector
{
public:
    /// construct HttpMetrics given the target url
    HttpMetrics(std::string target, size_t tmInSec, const std::string& filePath, const std::string& testName);

    /// @brief export metrics to input csv file
    /// @param isNew flag to determine if to open new file
    void to_csv(bool isNew = false);

    // @brief  todo export to csv
    void to_stream(std::ostream& ostr) const;

    size_t runtimeInSec() const;

protected: 

    std::tuple<std::chrono::milliseconds,double> statLatency() const;

private:

    mutable std::mutex _mutex;
    std::string _target;
    size_t _tmInSec; 

    size_t _numThreads;
    size_t _numConnections;
    std::filesystem::path _csvPath;
};

} // namespace