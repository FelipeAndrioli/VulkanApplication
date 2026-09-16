#include <iostream>
#include <array>
#include <math.h>

#define SAMPLES 8
#define PI 3.14159265359f

struct sample {
    float X;
    float Y;
};

struct complex_number {
    float Real;
    float Imaginary;
    float Magnitude;
};

std::array<complex_number, SAMPLES> DFT(const std::array<sample, SAMPLES>& TimeDomainSamples) {

    std::array<complex_number, SAMPLES> FrequencyDomainSamples = {};

    float TotalSamplesFloat = static_cast<float>(TimeDomainSamples.size());

    for (size_t FrequencyDomainSampleIndex = 0; FrequencyDomainSampleIndex < SAMPLES; FrequencyDomainSampleIndex++) {

        float Real = 0.0f;
        float Imaginary = 0.0f;

        for (size_t TimeDomainSampleIndex = 0; TimeDomainSampleIndex < SAMPLES; TimeDomainSampleIndex++) {

            float Angle = (2.0f * PI * FrequencyDomainSampleIndex * TimeDomainSampleIndex) / TotalSamplesFloat; 

            Real += TimeDomainSamples[TimeDomainSampleIndex].Y * std::cos(Angle);
            Imaginary -= TimeDomainSamples[TimeDomainSampleIndex].Y * std::sin(Angle);
        }

        float ComplexNumberMagnitude = std::sqrt(Real * Real + Imaginary * Imaginary);

        FrequencyDomainSamples[FrequencyDomainSampleIndex] = { Real, Imaginary, ComplexNumberMagnitude };
    }

    return FrequencyDomainSamples;
}

int Test_main() {

    float TotalDurationSeconds = 1.0f;
    float TargetSignalFrequencyHz = 1.0f;
    float TimeStep = TotalDurationSeconds / static_cast<float>(SAMPLES);
    float SamplingRateHz = 1.0f / TimeStep;

    std::array<sample, SAMPLES> TimeDomainSamples = {};

    std::cout << "Samples (N): " << SAMPLES << '\n'
        << "Sampling rate: " << SamplingRateHz << "Hz\n"
        << "Total duration (seconds): " << TotalDurationSeconds << "s\n"
        << "Sample interval (time step): " << TimeStep << "s\n";

    std::cout << "\nTime domain samples: \n";

    float CurrentTime = 0.0f;

    for (size_t SampleIndex = 0; SampleIndex < SAMPLES; SampleIndex++) {

        TimeDomainSamples[SampleIndex].X = CurrentTime;
        TimeDomainSamples[SampleIndex].Y = std::sin(2.0f * PI * TargetSignalFrequencyHz * CurrentTime);

        std::cout << "Sample: [ " << TimeDomainSamples[SampleIndex].X << ", " << TimeDomainSamples[SampleIndex].Y << " ]\n";
        CurrentTime += TimeStep;
    }

    std::array<complex_number, SAMPLES> FrequencyDomainSamples = DFT(TimeDomainSamples);

    std::cout << "\nFrequency domain samples: \n";

    for (size_t SampleIndex = 0; SampleIndex < SAMPLES; SampleIndex++) {
        std::cout << "Real: " << FrequencyDomainSamples[SampleIndex].Real << ", ";
        std::cout << "Imaginary: " << FrequencyDomainSamples[SampleIndex].Imaginary << ", ";
        std::cout << "Magnitude: " << FrequencyDomainSamples[SampleIndex].Magnitude << "\n";
    }

    return 0;
}
