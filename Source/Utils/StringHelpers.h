#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace CZ101 {
namespace Utils {

class StringHelpers
{
public:
    static std::string formatFrequency(float hz)
    {
        std::ostringstream oss;
        if (hz >= 1000.0f)
            oss << std::fixed << std::setprecision(2) << (hz / 1000.0f) << " kHz";
        else
            oss << std::fixed << std::setprecision(1) << hz << " Hz";
        return oss.str();
    }
    
    static std::string formatTime(float seconds)
    {
        std::ostringstream oss;
        if (seconds >= 1.0f)
            oss << std::fixed << std::setprecision(2) << seconds << " s";
        else
            oss << std::fixed << std::setprecision(1) << (seconds * 1000.0f) << " ms";
        return oss.str();
    }
    
    static std::string formatPercentage(float value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (value * 100.0f) << "%";
        return oss.str();
    }
    
    static std::string formatDecibels(float db)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << db << " dB";
        return oss.str();
    }
};

} // namespace Utils
} // namespace CZ101
