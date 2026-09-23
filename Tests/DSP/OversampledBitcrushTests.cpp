#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Bitcrush.h"
#include "DSP/OversampledBitcrush.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <vector>

using B33p::Bitcrush;
using B33p::OversampledBitcrush;
using B33p::kOversamplingFactor;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kFftOrder   = 13;                 // 8192-point FFT
    constexpr int    kFftSize    = 1 << kFftOrder;
    constexpr double kBinHz      = kSampleRate / kFftSize;   // 5.859375 Hz
    constexpr int    kTestBin    = 171;                // 1001.953125 Hz
    // Bin-locked to the analysis window -- see OversampledDistortionTests.cpp's
    // kTestFreqHz comment for why a non-bin-locked frequency's window
    // leakage would swamp the (smaller) true aliasing energy this test
    // measures.
    constexpr float  kTestFreqHz = static_cast<float>(kTestBin * kBinHz);

    // Renders a ~1 kHz sine through OversampledBitcrush at a low bit
    // depth and a target rate chosen to fold at 1x (kCrushTargetHz is
    // deliberately NOT an integer multiple of kTestFreqHz, so the
    // sample-and-hold's images/aliases land off the fundamental's own
    // harmonic series and aren't mistaken for legitimate harmonic
    // content by the classifier below).
    constexpr float kBitDepth      = 3.0f;
    constexpr float kCrushTargetHz = 8800.0f;

    // Measurement band: the top 4 kHz below the ORIGINAL (1x) Nyquist.
    // A sample-and-hold's spectrum is a whole series of copies of the
    // input at n*targetHz +/- f0 for every n -- most of that series is
    // legitimate, INTENTIONAL crush character (in-band copies well
    // below Nyquist that oversampling correctly leaves alone), and it
    // is loud: for most (bitDepth, targetHz) combinations, one of those
    // low-order copies is louder than anything true aliasing produces
    // and would swamp a whole-spectrum "worst peak" search, making 1x
    // and 4x look identical regardless of how much aliasing is actually
    // fixed (verified empirically while tuning this test). Content that
    // specifically folded down from just above the 1x Nyquist -- the
    // thing oversampling exists to remove -- lands closest to Nyquist,
    // so restricting the search to just below it isolates that
    // component from the legitimate low-order copies. kBitDepth/
    // kCrushTargetHz above were picked (by sweeping target rates) so
    // that folded energy in this band is actually the dominant feature
    // there at 1x.
    constexpr double kMeasureBandLoHz = 20000.0;
    constexpr double kMeasureBandHiHz = 24000.0;

    float measureNonHarmonicPeakDb(bool oversamplingEnabled)
    {
        OversampledBitcrush crush;
        crush.prepare(kSampleRate);
        crush.setOversamplingEnabledForTests(oversamplingEnabled);
        crush.setBitDepth(kBitDepth);
        crush.setTargetSampleRate(kCrushTargetHz);

        // Warm up past the parameter smoothers' 30 ms ramps (1440
        // samples at 48 kHz) and the oversampling filters' settling.
        for (int i = 0; i < 4096; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * kTestFreqHz
                                  * static_cast<double>(i) / kSampleRate;
            crush.processSample(static_cast<float>(std::sin(phase)));
        }

        juce::dsp::FFT fft(kFftOrder);
        std::vector<float> fftData(static_cast<size_t>(kFftSize) * 2, 0.0f);

        // No window -- kTestFreqHz is bin-locked (see above), so a plain
        // rectangular capture already gives leakage-free harmonic bins.
        for (int i = 0; i < kFftSize; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * kTestFreqHz
                                  * static_cast<double>(i) / kSampleRate;
            fftData[static_cast<size_t>(i)] = crush.processSample(static_cast<float>(std::sin(phase)));
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data());

        const float fundamentalMag = fftData[static_cast<size_t>(kTestBin)];

        float worstNonHarmonicMag = 0.0f;
        const int loBin = static_cast<int>(kMeasureBandLoHz / kBinHz);
        const int hiBin = std::min(static_cast<int>(kMeasureBandHiHz / kBinHz), kFftSize / 2);
        for (int bin = loBin; bin < hiBin; ++bin)
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

TEST_CASE("OversampledBitcrush: processSample without prepare returns silence",
          "[dsp][bitcrush][oversampling]")
{
    OversampledBitcrush crush;
    crush.setBitDepth(4.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(crush.processSample(0.5f) == 0.0f);
}

TEST_CASE("OversampledBitcrush: reports positive, deterministic latency once prepared",
          "[dsp][bitcrush][oversampling][latency]")
{
    OversampledBitcrush crush;
    REQUIRE(crush.getLatencySamples() == 0);   // before prepare()

    crush.prepare(kSampleRate);
    const int latency = crush.getLatencySamples();
    REQUIRE(latency > 0);

    crush.setBitDepth(2.0f);
    crush.setTargetSampleRate(3000.0f);
    for (int i = 0; i < 500; ++i)
        crush.processSample(0.3f);
    REQUIRE(crush.getLatencySamples() == latency);

    crush.reset();
    REQUIRE(crush.getLatencySamples() == latency);
}

TEST_CASE("OversampledBitcrush: disabling oversampling for tests reports zero latency",
          "[dsp][bitcrush][oversampling][latency]")
{
    OversampledBitcrush crush;
    crush.prepare(kSampleRate);
    crush.setOversamplingEnabledForTests(false);
    REQUIRE(crush.getLatencySamples() == 0);
}

TEST_CASE("Bitcrush: the oversampled-tick hold counter captures at the same real-time rate as 1x",
          "[dsp][bitcrush][oversampling]")
{
    // Direct test of the "hold counter must be scaled by the
    // oversampling factor" requirement, using Bitcrush's own
    // beginOversampledTick()/step() primitives (what OversampledBitcrush
    // drives internally) rather than going through the oversampling
    // filters -- isolates the hold-rate arithmetic from the FFT-level
    // aliasing measurement below.
    constexpr double sampleRate = 48000.0;
    constexpr float  targetHz   = 4800.0f;   // sampleRate / 10
    constexpr int    numSamples = 4800;      // 0.1 s -> ~targetHz * 0.1 = 480 captures

    auto countCaptures = [](auto&& renderOneHostSample) -> int
    {
        int   captures = 0;
        float last     = 0.0f;
        bool  first     = true;
        for (int i = 0; i < numSamples; ++i)
        {
            const float y = renderOneHostSample(i);
            if (first) { last = y; first = false; }
            else if (y != last) { ++captures; last = y; }
        }
        return captures;
    };

    Bitcrush crush1x;
    crush1x.prepare(sampleRate);
    crush1x.setBitDepth(16.0f);
    crush1x.setTargetSampleRate(targetHz);
    const int captures1x = countCaptures([&](int i)
    {
        return crush1x.processSample(static_cast<float>(i) / numSamples);
    });

    Bitcrush crushOversampled;
    crushOversampled.prepare(sampleRate);
    crushOversampled.setBitDepth(16.0f);
    crushOversampled.setTargetSampleRate(targetHz);
    const int capturesOversampled = countCaptures([&](int i)
    {
        crushOversampled.beginOversampledTick(sampleRate * static_cast<double>(kOversamplingFactor));
        float y = 0.0f;
        for (int k = 0; k < kOversamplingFactor; ++k)
        {
            const float ramp = (static_cast<float>(i) + static_cast<float>(k) / kOversamplingFactor)
                                / numSamples;
            y = crushOversampled.step(ramp);
        }
        return y;
    });

    INFO("1x captures: " << captures1x << ", oversampled-tick captures: " << capturesOversampled);
    REQUIRE(capturesOversampled == Approx(captures1x).margin(2));
}

TEST_CASE("OversampledBitcrush: 4x oversampling reduces the worst non-harmonic (folding) peak by >= 12 dB",
          "[dsp][bitcrush][oversampling][aliasing]")
{
    const float peak1x = measureNonHarmonicPeakDb(false);
    const float peak4x = measureNonHarmonicPeakDb(true);

    INFO("kOversamplingFactor = " << kOversamplingFactor);
    INFO("1x worst non-harmonic peak: " << peak1x << " dBc");
    INFO("4x worst non-harmonic peak: " << peak4x << " dBc");

    REQUIRE(peak4x <= peak1x - 12.0f);
}

