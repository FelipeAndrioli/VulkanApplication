#include <iostream>
#include <vector>
#include <math.h>
#include <cassert>

#define PI 3.14159265359f

// TODO: IFFT

struct complex_number {
    float Real = 0.0f;
    float Imaginary = 0.0f;
};

std::vector<complex_number> FFT(std::vector<complex_number> Samples) {

    const int N = Samples.size();

    if (N == 1) {
        return Samples;
    }

    const int M = N / 2;

    std::vector<complex_number> XEven(M);
    std::vector<complex_number> XOdd(M);

    for (size_t SampleIndex = 0; SampleIndex < M; SampleIndex++) {
        XEven[SampleIndex] = Samples[SampleIndex * 2];
        XOdd[SampleIndex] = Samples[SampleIndex * 2 + 1];
    }

    std::vector<complex_number> FEven(M);
    FEven = FFT(XEven);

    std::vector<complex_number> FOdd(M);
    FOdd = FFT(XOdd);

    std::vector<complex_number> FrequencyBins(N);
    
    for (size_t SampleIndex = 0; SampleIndex < N / 2; SampleIndex++) {

        const complex_number &EvenSample = FEven[SampleIndex];
        const complex_number &OddSample = FOdd[SampleIndex];

        const float Angle = -2.0f * PI * SampleIndex / N;
        const float SinA = std::sin(Angle);
        const float CosA = std::cos(Angle);

//        std::complex<float> ReferenceComplexExponential = std::polar(1.0f, -2.0f * PI * SampleIndex / N) * FOdd[SampleIndex];

        complex_number RawComplexExponential = { CosA, SinA };
        complex_number ComplexExponential = {};

        const float A = RawComplexExponential.Real;
        const float B = RawComplexExponential.Imaginary;
        const float C = OddSample.Real;
        const float D = OddSample.Imaginary;

        ComplexExponential.Real = (A * C - B * D);
        ComplexExponential.Imaginary = (A * D + B * C);

        complex_number FirstHalfFrequencyBin = { 
            EvenSample.Real + ComplexExponential.Real,
            EvenSample.Imaginary + ComplexExponential.Imaginary
        };

        complex_number SecondHalfFrequencyBin = { 
            EvenSample.Real - ComplexExponential.Real,
            EvenSample.Imaginary - ComplexExponential.Imaginary
        };

        FrequencyBins[SampleIndex] = FirstHalfFrequencyBin;
        FrequencyBins[SampleIndex + N / 2] = SecondHalfFrequencyBin;
    }

    return FrequencyBins;
}

int main() {

    const int SampleCount = 4;

    std::vector<complex_number> Samples(SampleCount);

    Samples[0] = { 0.0f, 0.0f };
    Samples[1] = { 1.0f, 0.0f };
    Samples[2] = { 0.0f, 0.0f };
    Samples[3] = { -1.0f, 0.0f };

    std::vector<complex_number> FrequencyDomainSamples = FFT(Samples);

    std::cout << "Time domain samples: \n" ;

    for (const auto &Sample : Samples) {
        std::cout << "\tSamples Real: " << Sample.Real << ", Imaginary: " << Sample.Imaginary << '\n';
    }

    std::cout << '\n';

    std::cout << "Frequency domain samples: \n" ;

    for (const auto &Sample : FrequencyDomainSamples) {
        std::cout << "\tSamples Real: " << Sample.Real << ", Imaginary: " << Sample.Imaginary << '\n';
    }

    std::cout << '\n';
}
