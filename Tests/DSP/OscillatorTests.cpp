#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Oscillator.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

using B33p::Oscillator;
using Waveform = B33p::Oscillator::Waveform;
using Catch::Approx;

namespace
{
    constexpr double kTwoPi      = 6.283185307179586;
    constexpr double kSampleRate = 48000.0;

    std::vector<float> render(Oscillator& osc, int numSamples)
    {
        std::vector<float> out(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            out[static_cast<size_t>(i)] = osc.processSample();
        return out;
    }

    int countZeroCrossings(const std::vector<float>& samples)
    {
        int count = 0;
        for (size_t i = 1; i < samples.size(); ++i)
        {
            const bool prevNeg = samples[i - 1] < 0.0f;
            const bool currNeg = samples[i] < 0.0f;
            if (prevNeg != currNeg)
                ++count;
        }
        return count;
    }
}

TEST_CASE("Oscillator: processSample without prepare returns silence", "[dsp][oscillator]")
{
    Oscillator osc;
    for (int i = 0; i < 64; ++i)
        REQUIRE(osc.processSample() == 0.0f);
}

TEST_CASE("Oscillator: prepare enables non-silent output", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(1000.0f);

    float sumSq = 0.0f;
    for (int i = 0; i < 48; ++i)
    {
        const float s = osc.processSample();
        sumSq += s * s;
    }
    REQUIRE(sumSq > 0.1f);
}

TEST_CASE("Oscillator: sine matches analytic reference within tolerance", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(1000.0f);

    for (int n = 0; n < 512; ++n)
    {
        const double expected = std::sin(kTwoPi * 1000.0 * static_cast<double>(n) / kSampleRate);
        const float  actual   = osc.processSample();
        REQUIRE(static_cast<double>(actual) == Approx(expected).margin(1e-5));
    }
}

TEST_CASE("Oscillator: square outputs exactly plus or minus one with 50% duty cycle", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Square);
    osc.setFrequency(1000.0f); // 48 samples per period

    const std::vector<float> samples = render(osc, 480); // 10 periods

    for (float s : samples)
        REQUIRE((s == 1.0f || s == -1.0f));

    const int highCount = static_cast<int>(std::count(samples.begin(), samples.end(), 1.0f));
    const int lowCount  = static_cast<int>(samples.size()) - highCount;
    REQUIRE(std::abs(highCount - lowCount) <= 4); // tolerate minor phase-accumulator drift
}

TEST_CASE("Oscillator: triangle is piecewise linear with peaks at quarter points", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Triangle);
    osc.setFrequency(100.0f); // 480 samples per period

    const std::vector<float> samples = render(osc, 480);

    for (float s : samples)
    {
        REQUIRE(s >= -1.0f);
        REQUIRE(s <=  1.0f);
    }

    // Zero-crossings at the start and halfway through the cycle.
    REQUIRE(samples[0]   == Approx(0.0f).margin(0.01f));
    REQUIRE(samples[240] == Approx(0.0f).margin(0.01f));

    // Peaks at the quarter and three-quarter points.
    REQUIRE(samples[120] == Approx( 1.0f).margin(0.01f));
    REQUIRE(samples[360] == Approx(-1.0f).margin(0.01f));
}

TEST_CASE("Oscillator: saw rises monotonically across a cycle", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Saw);
    osc.setFrequency(100.0f); // period ~ 480 samples

    // Render a safe interior chunk of one period, staying clear of the exact
    // wrap boundary where phase-accumulator drift can push the jump ±1 sample.
    const std::vector<float> samples = render(osc, 470);

    REQUIRE(samples.front() == Approx(-1.0f).margin(0.01f));
    REQUIRE(samples.back()  >  0.9f);

    for (size_t i = 1; i < samples.size(); ++i)
    {
        REQUIRE(samples[i]     >  samples[i - 1]);
        REQUIRE(samples[i]     >= -1.0f);
        REQUIRE(samples[i]     <=  1.0f);
    }
}

TEST_CASE("Oscillator: saw has exactly one discontinuity per period", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Saw);
    osc.setFrequency(100.0f); // period ~ 480 samples

    // A downward jump of ~2.0 marks the period boundary. Its exact sample
    // index drifts by ±1 because (1.0 / 480.0) isn't exactly representable in
    // double; what matters is that a single jump lives in the expected window.
    const std::vector<float> samples = render(osc, 500);

    int jumpCount = 0;
    for (size_t i = 470; i + 1 < samples.size(); ++i)
        if (samples[i + 1] - samples[i] < -1.5f)
            ++jumpCount;

    REQUIRE(jumpCount == 1);
}

TEST_CASE("Oscillator: noise stays in [-1, 1] with mean near zero", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Noise);
    osc.setFrequency(440.0f); // ignored for noise, but valid

    const std::vector<float> samples = render(osc, 48000); // 1 second

    for (float s : samples)
    {
        REQUIRE(s >= -1.0f);
        REQUIRE(s <=  1.0f);
    }

    const double sum  = std::accumulate(samples.begin(), samples.end(), 0.0);
    const double mean = sum / static_cast<double>(samples.size());
    REQUIRE(std::abs(mean) < 0.01);
}

TEST_CASE("Oscillator: noise RNG advances (consecutive samples are distinct)", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Noise);

    const std::vector<float> samples = render(osc, 100);

    std::vector<float> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const auto distinct = static_cast<size_t>(std::distance(sorted.begin(),
                                                            std::unique(sorted.begin(), sorted.end())));
    // With a continuous uniform distribution, 100 samples being ≥95% distinct
    // is essentially certain; anything less means the RNG isn't advancing.
    REQUIRE(distinct >= 95);
}

TEST_CASE("Oscillator: 440 Hz sine produces ~880 zero crossings per second", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(440.0f);

    const std::vector<float> samples = render(osc, static_cast<int>(kSampleRate));

    const int crossings = countZeroCrossings(samples);
    REQUIRE(crossings >= 879);
    REQUIRE(crossings <= 881);
}

TEST_CASE("Oscillator: reset returns phase to zero for deterministic waveforms", "[dsp][oscillator]")
{
    Oscillator osc;
    osc.prepare(kSampleRate);
    osc.setWaveform(Waveform::Sine);
    osc.setFrequency(1000.0f);

    const std::vector<float> first = render(osc, 256);
    osc.reset();
    const std::vector<float> second = render(osc, 256);

    for (size_t i = 0; i < first.size(); ++i)
        REQUIRE(first[i] == second[i]);
}

TEST_CASE("Oscillator: reset yields the same output as a fresh instance", "[dsp][oscillator]")
{
    Oscillator fresh;
    fresh.prepare(kSampleRate);
    fresh.setWaveform(Waveform::Saw);
    fresh.setFrequency(250.0f);

    Oscillator used;
    used.prepare(kSampleRate);
    used.setWaveform(Waveform::Saw);
    used.setFrequency(250.0f);

    for (int i = 0; i < 999; ++i)
        (void) used.processSample();

    used.reset();

    for (int i = 0; i < 128; ++i)
        REQUIRE(fresh.processSample() == used.processSample());
}
