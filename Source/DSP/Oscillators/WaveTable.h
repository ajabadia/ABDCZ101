#pragma once

#include <array>
#include <cmath>

namespace CZ101 {
namespace DSP {

/**
 * @brief WaveTable generator for Phase Distortion synthesis
 * 
 * Generates lookup tables for basic waveforms used in CZ-101 emulation.
 * Tables are 256 samples for efficient memory usage and fast lookup.
 * 
 * Note: Sawtooth and Square will use PolyBLEP at render time,
 * so these tables are "naive" versions.
 */
class WaveTable
{
public:
    static constexpr int TABLE_SIZE = 256;
    
    WaveTable();
    
    /**
     * @brief Get sine wave value at normalized phase
     * @param phase Normalized phase [0.0, 1.0]
     * @return Sample value [-1.0, 1.0]
     */
    float getSine(float phase) const noexcept;
    
    /**
     * @brief Get sawtooth wave value at normalized phase
     * @param phase Normalized phase [0.0, 1.0]
     * @return Sample value [-1.0, 1.0]
     * @note This is a naive sawtooth. Apply PolyBLEP at render time!
     */
    float getSawtooth(float phase) const noexcept;
    
    /**
     * @brief Get square wave value at normalized phase
     * @param phase Normalized phase [0.0, 1.0]
     * @return Sample value [-1.0, 1.0]
     * @note This is a naive square. Apply PolyBLEP at render time!
     */
    float getSquare(float phase) const noexcept;
    
    /**
     * @brief Get triangle wave value at normalized phase
     * @param phase Normalized phase [0.0, 1.0]
     * @return Sample value [-1.0, 1.0]
     */
    float getTriangle(float phase) const noexcept;
    
    // Advanced waveforms (CZ-101 specific)
    float getPulse(float phase, float width = 0.5f) const noexcept;
    float getDoubleSine(float phase) const noexcept;
    float getHalfSine(float phase) const noexcept;
    float getResonantSaw(float phase) const noexcept;
    float getResonantTriangle(float phase) const noexcept;
    float getTrapezoid(float phase) const noexcept;
    
private:
    std::array<float, TABLE_SIZE> sineTable;
    std::array<float, TABLE_SIZE> sawtoothTable;
    std::array<float, TABLE_SIZE> squareTable;
    std::array<float, TABLE_SIZE> triangleTable;
    
    void generateTables();
    
    // Helper: Linear interpolation between table samples
    float interpolate(const std::array<float, TABLE_SIZE>& table, float phase) const noexcept;
};

} // namespace DSP
} // namespace CZ101
