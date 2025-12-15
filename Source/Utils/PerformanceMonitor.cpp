#include "PerformanceMonitor.h"
#include <algorithm>

namespace CZ101 {
namespace Utils {

PerformanceMonitor::PerformanceMonitor()
{
}

void PerformanceMonitor::startMeasurement()
{
    startTime = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::stopMeasurement()
{
    endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double timeMs = duration.count() / 1000.0;
    
    totalTime += timeMs;
    peakTime = std::max(peakTime, timeMs);
    measurementCount++;
}

double PerformanceMonitor::getAverageCpuUsage() const
{
    if (measurementCount == 0)
        return 0.0;
    
    return totalTime / measurementCount;
}

double PerformanceMonitor::getPeakCpuUsage() const
{
    return peakTime;
}

int PerformanceMonitor::getVoiceCount() const
{
    return currentVoiceCount;
}

void PerformanceMonitor::setVoiceCount(int count)
{
    currentVoiceCount = count;
}

void PerformanceMonitor::reset()
{
    totalTime = 0.0;
    peakTime = 0.0;
    measurementCount = 0;
}

} // namespace Utils
} // namespace CZ101
