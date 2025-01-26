
#pragma once

#include <http_client/Metrics.h>

#include <utils/metrics/CsvWriter.h>

#include <mutex>
#include <string>

namespace nvd {

/// @brief  extend the http::MetricsCollector for the http perf-tool statistics
class HttpMetrics : public http::MetricsCollector
{
public:
    /// construct HttpMetrics given the target url
    HttpMetrics(std::string target, size_t tmInSec);

    /// @brief export metrics to input csv file
    void to_csv(const std::string& filePath);

    // @brief  todo export to csv
    void to_stream(std::ostream& ostr) const;

    size_t runtimeInSec() const;
protected: 

    std::tuple<double,double> statLatency() const;

private:

    mutable std::mutex _mutex;
    std::string _target;
    size_t _tmInSec;
};

} // namespace