#include <iostream>
#include <vector>
#include <math.h>
#include <cassert>

#define PI 3.14159265359f

// TODO: Non recursive FFT
//  - Complex number operations as functions (I know we could do it as operator overloading but we don't have it in glsl).
//      - Sum 
//      - Multiplication 
//      - Subtraction

struct complex_number {
    float Real = 0.0f;
    float Imaginary = 0.0f;
};

std::vector<complex_number> IFFT_Internal_Recursive(std::vector<complex_number> Samples) {

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
    FEven = IFFT_Internal_Recursive(XEven);

    std::vector<complex_number> FOdd(M);
    FOdd = IFFT_Internal_Recursive(XOdd);

    std::vector<complex_number> TimeBins(N);

    for (size_t SampleIndex = 0; SampleIndex < N / 2; SampleIndex++) {
        const complex_number &EvenSample = FEven[SampleIndex];
        const complex_number &OddSample = FOdd[SampleIndex];

        const float Angle = 2.0f * PI * SampleIndex / N;
        const float SinA = std::sin(Angle);
        const float CosA = std::cos(Angle);

        const complex_number RawComplexExponential = { CosA, SinA };
        complex_number ComplexExponential = {};

        const float A = RawComplexExponential.Real;
        const float B = RawComplexExponential.Imaginary;
        const float C = OddSample.Real;
        const float D = OddSample.Imaginary;

        ComplexExponential.Real = (A * C - B * D);
        ComplexExponential.Imaginary = (A * D + B * C);

        const complex_number FirstHalfSample = { 
            EvenSample.Real + ComplexExponential.Real,
            EvenSample.Imaginary + ComplexExponential.Imaginary
        };

        const complex_number SecondHalfSample = { 
            EvenSample.Real - ComplexExponential.Real,
            EvenSample.Imaginary - ComplexExponential.Imaginary
        };

        TimeBins[SampleIndex] = FirstHalfSample;
        TimeBins[SampleIndex + N / 2] = SecondHalfSample;
    }

    return TimeBins;
}

std::vector<complex_number> IFFT_Recursive(std::vector<complex_number> Samples) {
    const size_t N = Samples.size();

    std::vector<complex_number> TimeDomainSamples = IFFT_Internal_Recursive(Samples);

    for (size_t SampleIndex = 0; SampleIndex < N; SampleIndex++) {
        TimeDomainSamples[SampleIndex].Real /= static_cast<float>(N);
        TimeDomainSamples[SampleIndex].Imaginary /= static_cast<float>(N);
    }

    return TimeDomainSamples;
}

complex_number ComplexAdd(complex_number A, complex_number B) {

    complex_number Result = {};

    const float AReal       = A.Real;
    const float AImaginary  = A.Imaginary;
    const float BReal       = B.Real;
    const float BImaginary  = B.Imaginary;

    Result.Real         = (AReal + BReal);
    Result.Imaginary    = (AImaginary + BImaginary);

    return Result;
}

complex_number ComplexSubtract(complex_number A, complex_number B) {

    complex_number Result = {};

    const float AReal       = A.Real;
    const float AImaginary  = A.Imaginary;
    const float BReal       = B.Real;
    const float BImaginary  = B.Imaginary;

    Result.Real         = (AReal - BReal);
    Result.Imaginary    = (AImaginary - BImaginary);

    return Result;
}

complex_number ComplexMulttiply(complex_number A, complex_number B) {

    complex_number Result = {};

    const float AReal       = A.Real;
    const float AImaginary  = A.Imaginary;
    const float BReal       = B.Real;
    const float BImaginary  = B.Imaginary;

    Result.Real         = (AReal * BReal - AImaginary * BImaginary);
    Result.Imaginary    = (AReal * BImaginary + AImaginary * BReal);

    return Result;
}

void BitReversalPermutation(std::vector<complex_number>& Samples) {
    const size_t N = Samples.size();

    size_t j = 0;

    for (size_t i = 0; i < N; i++) {

        // Avoid swapping the same elements pair twice.
        if (i < j) {
            complex_number Temp = Samples[i];
            Samples[i] = Samples[j];
            Samples[j] = Temp;
        }

        // Advances 'j' counter in a reversed way (bit-reversed counter)
        // Starts with the most significative bit (for N = 8 it would be 4)
        size_t bit =  N >> 1;

        // while 'j' bit is 1
        // zeroes the bit using XOR 
        // moves bit to the next bit to the right
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }

        // sets the first bit that was 0 to 1
        j |= bit;
    }
}

std::vector<complex_number> IFFT_Iterative(std::vector<complex_number> Samples) {
    const size_t N = Samples.size();

    if (N <= 1) {
        return Samples;
    }

    BitReversalPermutation(Samples);

    for (size_t Len = 2; Len <= N; Len <<= 1) {
        float Angle = 2.0f * PI / Len;

        complex_number WLen = { std::cos(Angle), std::sin(Angle) };

        for (size_t i = 0; i < N; i += Len) {
            complex_number W = { 1.0f, 0.0f }; // initial twiddle factor (W^0)
            size_t HalfLength = Len / 2;

            for (size_t j = 0; j < HalfLength; j++) {
                complex_number U = Samples[i + j];
                complex_number V = ComplexMulttiply(Samples[i + j + HalfLength], W);

                Samples[i + j] = ComplexAdd(U, V);
                Samples[i + j + HalfLength] = ComplexSubtract(U, V);

                W = ComplexMulttiply(W, WLen);
            }
        }
    }

    const float Scale = 1.0f / N;

    for (size_t SampleIndex = 0; SampleIndex < N; SampleIndex++) {
        Samples[SampleIndex].Real *= Scale;
        Samples[SampleIndex].Imaginary *= Scale;
    }

    return Samples;
}



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

        const complex_number RawComplexExponential = { CosA, SinA };
        complex_number ComplexExponential = {};

        const float A = RawComplexExponential.Real;
        const float B = RawComplexExponential.Imaginary;
        const float C = OddSample.Real;
        const float D = OddSample.Imaginary;

        ComplexExponential.Real = (A * C - B * D);
        ComplexExponential.Imaginary = (A * D + B * C);

        const complex_number FirstHalfFrequencyBin = { 
            EvenSample.Real + ComplexExponential.Real,
            EvenSample.Imaginary + ComplexExponential.Imaginary
        };

        const complex_number SecondHalfFrequencyBin = { 
            EvenSample.Real - ComplexExponential.Real,
            EvenSample.Imaginary - ComplexExponential.Imaginary
        };

        FrequencyBins[SampleIndex] = FirstHalfFrequencyBin;
        FrequencyBins[SampleIndex + N / 2] = SecondHalfFrequencyBin;
    }

    return FrequencyBins;
}

int Test_main() {

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

    std::cout << "Frequency domain samples: \n";

    for (const auto &Sample : FrequencyDomainSamples) {
        std::cout << "\tSamples Real: " << Sample.Real << ", Imaginary: " << Sample.Imaginary << '\n';
    }

    std::cout << '\n';

    std::vector<complex_number> TimeDomainSamplesFromIFFTRecursive = IFFT_Recursive(FrequencyDomainSamples);

    std::cout << "Time domain samples from IFFT (Recursive): \n";

    for (const auto &Sample : TimeDomainSamplesFromIFFTRecursive) {
        std::cout << "\tSamples Real: " << Sample.Real << ", Imaginary: " << Sample.Imaginary << '\n';
    }

    std::cout << '\n';

    std::vector<complex_number> TimeDomainSamplesFromIFFTIterative = IFFT_Iterative(FrequencyDomainSamples);

    std::cout << "Time domain samples from IFFT (Iterative): \n";

    for (const auto &Sample : TimeDomainSamplesFromIFFTIterative) {
        std::cout << "\tSamples Real: " << Sample.Real << ", Imaginary: " << Sample.Imaginary << '\n';
    }
}
