#include "WaveSpectrum.h"

#include <iostream>
#include <vector>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#define PI 3.14159265359f

#define G 9.81f

glm::vec2 Hash22(glm::vec2 p) {
    glm::vec3 p3 = glm::fract(glm::vec3(p.x, p.y, p.x) * glm::vec3(0.1031, 0.1136, 0.1373));
    glm::vec3 p33 = glm::vec3(p3.y, p3.z, p3.x);
    p33.x += + 33.33;
    p33.y += + 33.33;
    p33.z += + 33.33;

    p3 += glm::dot(p3, p33);

    glm::vec2 p3xx = glm::vec2(p3.x, p3.x);
    glm::vec2 p3yz = glm::vec2(p3.y, p3.z);
    glm::vec2 p3zy = glm::vec2(p3.z, p3.y);

    return glm::fract((p3xx + p3yz) * p3zy);
}

glm::vec4 Generate4Uniforms(glm::vec2 uv) {
    glm::vec2 u1 = Hash22(uv);
    glm::vec2 u2 = Hash22(uv + glm::vec2(13.37, 37.13));
    return glm::vec4(u1, u2);
}

glm::vec4 GaussianRandom(glm::vec2 uv) {
    glm::vec4 u = Generate4Uniforms(uv);
    
    // Protect against log(0)
    u = max(u, glm::vec4(1e-7));
    
    float r1 = sqrt(-2.0 * log(u.x));
    float theta1 = 2.0 * PI * u.y;
    
    float r2 = sqrt(-2.0 * log(u.z));
    float theta2 = 2.0 * PI * u.w;
    
    return glm::vec4(
        r1 * cos(theta1),
        r1 * sin(theta1),
        r2 * cos(theta2),
        r2 * sin(theta2)
    );
}

float Phillips(
    glm::vec2 K, 
    glm::vec2 WindDirection, 
    float WindSpeed, 
    float Amplitude, 
    float WaveSuppressionThreshold) {

    float k_length = length(K);
    
    if (k_length < 0.0001) return 0.0;
    
    float k_2 = k_length * k_length;
    float k_4 = k_2 * k_2;
    
    float L = (WindSpeed * WindSpeed) / G;
    float L_2 = L * L;
    
    // small wave suppression
    float s = exp(-k_2 * (WaveSuppressionThreshold * WaveSuppressionThreshold));
    float p = Amplitude * (exp(-1.0 / (k_2 * L_2)) / k_4) * s;      
    
    float d = dot(normalize(K), normalize(WindDirection));
    
    // eliminates waves moving against wind direction
    if (d < 0.0) return 0.0;
    
    return p * (d * d);
}

std::vector<glm::vec4> GenerateWaveSpectrum(
    uint32_t SpectrumDimensions, 
    float PatchSizeMeters,
    float WindSpeed,
    float Amplitude,
    float WaveSuppressionThreshold,
    glm::vec2 WindDirection) {

    std::vector<glm::vec4> Result(SpectrumDimensions * SpectrumDimensions);

    glm::vec2 HalfResolution = glm::vec2(SpectrumDimensions * 0.5f);

    std::vector<glm::vec2> GaussianNoise(SpectrumDimensions * SpectrumDimensions);

    for (size_t Y = 0; Y < SpectrumDimensions; Y++) {
        for (size_t X = 0; X < SpectrumDimensions; X++) {
            glm::vec4 Gauss = GaussianRandom(glm::vec2(X, Y));

            GaussianNoise[Y * SpectrumDimensions + X] = glm::vec2(Gauss.x, Gauss.y);
        }
    }

    for (size_t Y = 0; Y < SpectrumDimensions; Y++) {
        for (size_t X = 0; X < SpectrumDimensions; X++) {
            glm::vec2 FragCoord = glm::vec2(X, Y);

            glm::vec2 N = FragCoord - HalfResolution;
            glm::vec2 K = (2.0f * PI * N) / PatchSizeMeters;

            uint32_t XNeg = (SpectrumDimensions - X) % SpectrumDimensions;
            uint32_t YNeg = (SpectrumDimensions - Y) % SpectrumDimensions;

            glm::vec2 NN = glm::vec2(XNeg, YNeg) - HalfResolution;
            glm::vec2 NK = (2.0f * PI * NN) / PatchSizeMeters;

            float P = Phillips(K, WindDirection, WindSpeed, Amplitude, WaveSuppressionThreshold);
            float NP = Phillips(NK, WindDirection, WindSpeed, Amplitude, WaveSuppressionThreshold);

            glm::vec2 GaussLocal = GaussianNoise[Y * SpectrumDimensions + X];
            glm::vec2 GaussMirrored = GaussianNoise[YNeg * SpectrumDimensions + XNeg];

            float Mag = glm::sqrt(P) * (1.0f / glm::sqrt(2.0f));
            float NMag = glm::sqrt(NP) * (1.0f / glm::sqrt(2.0f));

            glm::vec2 H0 = GaussLocal * Mag;
            glm::vec2 H0Conj = GaussMirrored * NMag;
            H0Conj.y = -H0Conj.y;

            Result[Y * SpectrumDimensions + X] = glm::vec4(H0, H0Conj);
        }
    }

    return Result;
}
