#pragma once 

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <vector>

std::vector<glm::vec4> GenerateWaveSpectrum(
    uint32_t SpectrumDimensions, 
    float PatchSizeMeters,
    float WindSpeed,
    float Amplitude,
    float WaveSuppressionThreshold,
    glm::vec2 WindDirection);
