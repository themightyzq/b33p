#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/OversampledDistortion.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <vector>

using B33p::Distortion;
using B33p::OversampledDistortion;
using B33p::kOversamplingFactor;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kFftOrder   = 13;                 // 8192-point FFT
    constexpr int    kFftSize    = 1 << kFftOrder;
    constexpr double kBinHz      = kSampleRate / kFftSize;   // 5.859375 Hz
    constexpr int    kTestBin    = 171;                // 1001.953125 Hz
    // Bin-locked to the analysis window (an exact integer number of
    // cycles fit in kFftSize samples) so a rectangular window produces
    // leakage-free harmonic bins -- with a non-bin-locked frequency (a
    // literal 1000.0 Hz doesn't complete a whole number of cycles in
    // 8192 samples at 48 kHz), window sidelobe leakage from the strong
    // low-order harmonics swamps the much smaller true aliasing energy
    // this test is trying to measure, making 1x and 4x look identical.
    constexpr float  kTestFreqHz = static_cast<float>(kTestBin * kBinHz);

    // Renders a ~1 kHz sine through OversampledDistortion at full drive
    // and returns the highest spectral peak that is NOT within half a
    // bin-and-a-half of an integer multiple of the fundamental, relative
    // to the fundamental's own magnitude, in dB. A well-behaved
    // waveshaper only produces harmonics of its input; energy anywhere
    // else is aliasing (folded harmonics, or -- for the oversampled
    // case -- oversampling-filter artifacts, which should be far
    // smaller).
    float measureNonHarmonicPeakDb(bool oversamplingEnabled)
    {
        OversampledDistortion dist;
        dist.prepare(kSampleRate);
        dist.setOversamplingEnabledForTests(oversamplingEnabled);
        dist.setDrive(100.0f);   // full drive -- hard clipping, rich in harmonics

        // Warm up past the drive smoother's 20 ms ramp (960 samples at
        // 48 kHz) and the oversampling filters' own settling.
        for (int i = 0; i < 4096; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * kTestFreqHz
                                  * static_cast<double>(i) / kSampleRate;
            dist.processSample(static_cast<float>(std::sin(phase)));
        }

        juce::dsp::FFT fft(kFftOrder);
        std::vector<float> fftData(static_cast<size_t>(kFftSize) * 2, 0.0f);

        // No window -- kTestFreqHz is bin-locked (see above), so a plain
        // rectangular capture already gives leakage-free harmonic bins.
        for (int i = 0; i < kFftSize; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * kTestFreqHz
                                  * static_cast<double>(i) / kSampleRate;
            fftData[static_cast<size_t>(i)] = dist.processSample(static_cast<float>(std::sin(phase)));
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data());

        const float fundamentalMag = fftData[static_cast<size_t>(kTestBin)];

        float worstNonHarmonicMag = 0.0f;
        for (int bin = 2; bin < kFftSize / 2; ++bin)
        {
            const double binFreq         = bin * kBinHz;
            const double nearestHarmonic = std::round(binFreq / kTestFreqHz) * kTestFreqHz;
            if (std::abs(binFreq - nearestHarmonic) <= kBinHz * 1.5)
                continue;   // this bin IS (close enough to) a harmonic of 1 kHz

            worstNonHarmonicMag = std::max(worstNonHarmonicMag, fftData[static_cast<size_t>(bin)]);
        }

        return 20.0f * std::log10(worstNonHarmonicMag / fundamentalMag);
    }
}

TEST_CASE("OversampledDistortion: processSample without prepare returns silence",
          "[dsp][distortion][oversampling]")
{
    OversampledDistortion dist;
    dist.setDrive(5.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(dist.processSample(0.5f) == 0.0f);
}

TEST_CASE("OversampledDistortion: at low drive, steady-state output tracks the raw Distortion class",
          "[dsp][distortion][oversampling]")
{
    // Not an exact per-sample match -- the oversampling filters add a
    // few samples of latency and slight passband ripple -- but for a
    // slowly-varying, near-linear (low drive) signal the two should
    // track closely once the filters have settled.
    OversampledDistortion oversampled;
    oversampled.prepare(kSampleRate);
    oversampled.setDrive(1.0f);

    Distortion raw;
    raw.prepare(kSampleRate);
    raw.setDrive(1.0f);

    std::vector<float> rawOut;
    std::vector<float> oversampledOut;
    for (int i = 0; i < 2000; ++i)
    {
        const float x = 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 200.0f
                                          * static_cast<float>(i) / static_cast<float>(kSampleRate));
        rawOut.push_back(raw.processSample(x));
        oversampledOut.push_back(oversampled.processSample(x));
    }

    // Compare RMS energy over the tail (past any warm-up/latency
    // transient) rather than sample-for-sample.
    double rawEnergy = 0.0, oversampledEnergy = 0.0;
    for (size_t i = 1000; i < rawOut.size(); ++i)
    {
        rawEnergy          += rawOut[i] * rawOut[i];
        oversampledEnergy  += oversampledOut[i] * oversampledOut[i];
    }
    REQUIRE(std::sqrt(oversampledEnergy) == Approx(std::sqrt(rawEnergy)).margin(0.05));
}

TEST_CASE("OversampledDistortion: reports positive, deterministic latency once prepared",
          "[dsp][distortion][oversampling][latency]")
{
    OversampledDistortion dist;
    REQUIRE(dist.getLatencySamples() == 0);   // before prepare()

    dist.prepare(kSampleRate);
    const int latency = dist.getLatencySamples();
    REQUIRE(latency > 0);

    // Deterministic: doesn't depend on drive or on how many samples
    // have been processed.
    dist.setDrive(50.0f);
    for (int i = 0; i < 500; ++i)
        dist.processSample(0.3f);
    REQUIRE(dist.getLatencySamples() == latency);

    dist.reset();
    REQUIRE(dist.getLatencySamples() == latency);
}

TEST_CASE("OversampledDistortion: disabling oversampling for tests reports zero latency",
          "[dsp][distortion][oversampling][latency]")
{
    OversampledDistortion dist;
    dist.prepare(kSampleRate);
    dist.setOversamplingEnabledForTests(false);
    REQUIRE(dist.getLatencySamples() == 0);
}

TEST_CASE("OversampledDistortion: 4x oversampling reduces the worst non-harmonic peak by >= 12 dB",
          "[dsp][distortion][oversampling][aliasing]")
{
    const float peak1x = measureNonHarmonicPeakDb(false);
    const float peak4x = measureNonHarmonicPeakDb(true);

    INFO("kOversamplingFactor = " << kOversamplingFactor);
    INFO("1x worst non-harmonic peak: " << peak1x << " dBc");
    INFO("4x worst non-harmonic peak: " << peak4x << " dBc");

    REQUIRE(peak4x <= peak1x - 12.0f);
}
