#pragma once

#include <chrono>
#include <string>

namespace CZ101 {
namespace Utils {

class PerformanceMonitor
{
public:
    PerformanceMonitor();
    
    void startMeasurement();
    void stopMeasurement();
    
    double getAverageCpuUsage() const;
    double getPeakCpuUsage() const;
    int getVoiceCount() const;
    
    void setVoiceCount(int count);
    void reset();
    
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    
    double totalTime = 0.0;
    double peakTime = 0.0;
    int measurementCount = 0;
    int currentVoiceCount = 0;
};

} // namespace Utils
} // namespace CZ101
