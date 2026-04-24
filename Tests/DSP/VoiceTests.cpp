#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Voice.h"

#include <cmath>
#include <vector>

using B33p::Oscillator;
using B33p::Voice;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;

    // Configure a voice with near-transparent effects and a long-hold
    // amp envelope so tests can focus on the oscillator/pitch-env path.
    void configureCleanBeep(Voice& voice)
    {
        voice.setWaveform(Oscillator::Waveform::Sine);
        voice.setBasePitchHz(440.0f);

        voice.setAmpAttack(0.0f);
        voice.setAmpDecay(0.0f);
        voice.setAmpSustain(1.0f);
        voice.setAmpRelease(0.0f);

        voice.setFilterCutoff(20000.0f);
        voice.setFilterResonance(0.707f);

        voice.setBitcrushBitDepth(16.0f);
        voice.setBitcrushSampleRate(static_cast<float>(kSampleRate));

        voice.setDistortionDrive(0.1f);  // near-linear for small signals
        voice.setGain(1.0f);
    }

    std::vector<float> render(Voice& voice, int numSamples)
    {
        std::vector<float> out(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            out[static_cast<size_t>(i)] = voice.processSample();
        return out;
    }

    int countZeroCrossings(const std::vector<float>& samples, int lo, int hi)
    {
        int crossings = 0;
        for (int i = lo + 1; i < hi; ++i)
        {
            const bool prevPositive = samples[static_cast<size_t>(i - 1)] > 0.0f;
            const bool currPositive = samples[static_cast<size_t>(i)]     > 0.0f;
            if (prevPositive != currPositive)
                ++crossings;
        }
        return crossings;
    }

    float peakAbs(const std::vector<float>& samples)
    {
        float peak = 0.0f;
        for (float s : samples)
            peak = std::max(peak, std::fabs(s));
        return peak;
    }
}

TEST_CASE("Voice: processSample without prepare returns silence", "[dsp][voice]")
{
    Voice voice;
    configureCleanBeep(voice);
    voice.trigger(0.1f, 0.0f);

    REQUIRE_FALSE(voice.isActive());
    for (int i = 0; i < 64; ++i)
        REQUIRE(voice.processSample() == 0.0f);
}

TEST_CASE("Voice: prepared but not triggered is idle and silent", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);

    REQUIRE_FALSE(voice.isActive());
    for (int i = 0; i < 64; ++i)
        REQUIRE(voice.processSample() == 0.0f);
}

TEST_CASE("Voice: triggered voice oscillates at base pitch", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);

    voice.trigger(1.0f, 0.0f);
    const std::vector<float> samples = render(voice, static_cast<int>(kSampleRate));

    // A 440 Hz sine produces ~880 sign changes per second.
    const int crossings = countZeroCrossings(samples, 0, static_cast<int>(samples.size()));
    REQUIRE(crossings >= 870);
    REQUIRE(crossings <= 890);

    REQUIRE(peakAbs(samples) > 0.05f);
}

TEST_CASE("Voice: per-event pitch offset shifts frequency by semitones", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);

    // +12 semitones doubles the frequency -> ~1760 sign changes per sec.
    voice.trigger(1.0f, 12.0f);
    const std::vector<float> samples = render(voice, static_cast<int>(kSampleRate));

    const int crossings = countZeroCrossings(samples, 0, static_cast<int>(samples.size()));
    REQUIRE(crossings >= 1740);
    REQUIRE(crossings <= 1780);
}

TEST_CASE("Voice: pitch envelope modulates frequency across the event", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);

    // Ramp 0 -> +12 semitones over the event duration.
    voice.setPitchCurve({ { 0.0f, 0.0f }, { 1.0f, 12.0f } });
    voice.trigger(1.0f, 0.0f);

    const std::vector<float> samples = render(voice, static_cast<int>(kSampleRate));

    const int half  = static_cast<int>(samples.size()) / 2;
    const int early = countZeroCrossings(samples, 0,    half);
    const int late  = countZeroCrossings(samples, half, static_cast<int>(samples.size()));

    // Analytical integral puts early ~= 526, late ~= 744 (ratio ~1.41).
    // Require at least 1.3x, well within the analytical margin.
    REQUIRE(static_cast<double>(late) > static_cast<double>(early) * 1.3);
}

TEST_CASE("Voice: gain scales the output linearly", "[dsp][voice]")
{
    auto peakAtGain = [](float gainValue)
    {
        Voice voice;
        voice.prepare(kSampleRate);
        configureCleanBeep(voice);
        voice.setGain(gainValue);

        voice.trigger(1.0f, 0.0f);
        return peakAbs(render(voice, 5000));
    };

    const float peakUnity = peakAtGain(1.0f);
    const float peakHalf  = peakAtGain(0.5f);

    REQUIRE(peakUnity > 0.05f);
    REQUIRE(peakHalf == Approx(peakUnity * 0.5f).margin(0.02f));
}

TEST_CASE("Voice: noteOff and release tail drive isActive to false", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);
    voice.setAmpRelease(0.01f);   // 480 samples

    voice.trigger(1.0f, 0.0f);
    for (int i = 0; i < 100; ++i) (void) voice.processSample();
    REQUIRE(voice.isActive());

    voice.noteOff();
    for (int i = 0; i < 600; ++i) (void) voice.processSample();
    REQUIRE_FALSE(voice.isActive());
}

TEST_CASE("Voice: reset returns the voice to idle and silent", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);

    voice.trigger(1.0f, 0.0f);
    for (int i = 0; i < 200; ++i) (void) voice.processSample();
    REQUIRE(voice.isActive());

    voice.reset();
    REQUIRE_FALSE(voice.isActive());

    for (int i = 0; i < 256; ++i)
        REQUIRE(voice.processSample() == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("Voice: retrigger while ringing restarts the amp envelope cleanly", "[dsp][voice]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureCleanBeep(voice);
    voice.setAmpAttack(0.01f);
    voice.setAmpRelease(0.1f);

    voice.trigger(1.0f, 0.0f);
    for (int i = 0; i < 100; ++i) (void) voice.processSample();

    voice.noteOff();
    for (int i = 0; i < 100; ++i) (void) voice.processSample();
    REQUIRE(voice.isActive());  // still in release

    // Retrigger mid-release: must remain active and produce sound.
    voice.trigger(1.0f, 0.0f);
    REQUIRE(voice.isActive());

    const std::vector<float> samples = render(voice, 4800);
    REQUIRE(peakAbs(samples) > 0.05f);
}
