#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/Oscillator.h"
#include "Pattern/Pattern.h"
#include "Pattern/PatternRenderer.h"

#include <cmath>

using B33p::Event;
using B33p::Oscillator;
using B33p::Pattern;
using B33p::PatternRenderer;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;

    PatternRenderer::Config defaultConfig()
    {
        PatternRenderer::Config c;
        c.sampleRate     = kSampleRate;
        c.maxTailSeconds = 1.0;
        c.waveform       = Oscillator::Waveform::Sine;
        c.basePitchHz    = 440.0f;
        c.ampAttack      = 0.0f;
        c.ampDecay       = 0.0f;
        c.ampSustain     = 1.0f;
        c.ampRelease     = 0.05f;
        c.gain           = 0.5f;
        return c;
    }

    float peakAbs(const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                peak = std::max(peak, std::fabs(b.getSample(ch, i)));
        return peak;
    }
}

TEST_CASE("PatternRenderer: empty pattern produces silent buffer of pattern length",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.5);

    const auto buf = PatternRenderer::render(pattern, defaultConfig());

    REQUIRE(buf.getNumChannels() == 1);
    // No events fire, so the voice never goes active and the tail
    // loop exits immediately. Length is exactly the pattern.
    REQUIRE(buf.getNumSamples() == static_cast<int>(0.5 * kSampleRate));
    REQUIRE(peakAbs(buf) == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("PatternRenderer: single event produces audible output",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.5);
    pattern.addEvent(0, { 0.1, 0.2, 0.0f });

    const auto buf = PatternRenderer::render(pattern, defaultConfig());

    // Pattern is 0.5 s; the release tail extends slightly past that.
    REQUIRE(buf.getNumSamples() >= static_cast<int>(0.5 * kSampleRate));
    REQUIRE(buf.getNumSamples() <= static_cast<int>(1.5 * kSampleRate));
    REQUIRE(peakAbs(buf) > 0.05f);
}

TEST_CASE("PatternRenderer: deterministic given identical inputs (sine)",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.3);
    pattern.addEvent(0, { 0.0,  0.1, 0.0f });
    pattern.addEvent(1, { 0.15, 0.1, 7.0f });

    const auto cfg = defaultConfig();
    const auto buf1 = PatternRenderer::render(pattern, cfg);
    const auto buf2 = PatternRenderer::render(pattern, cfg);

    REQUIRE(buf1.getNumSamples() == buf2.getNumSamples());
    for (int i = 0; i < buf1.getNumSamples(); ++i)
        REQUIRE(buf1.getSample(0, i) == buf2.getSample(0, i));
}

TEST_CASE("PatternRenderer: tail extends past pattern length for late events",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.2);

    // Event starts very near pattern end so its release tail must
    // extend past the pattern boundary into the tail region.
    pattern.addEvent(0, { 0.18, 0.05, 0.0f });

    auto cfg = defaultConfig();
    cfg.ampRelease     = 0.1f;
    cfg.maxTailSeconds = 0.5;

    const auto buf = PatternRenderer::render(pattern, cfg);
    REQUIRE(buf.getNumSamples() > static_cast<int>(0.2 * kSampleRate));
}

TEST_CASE("PatternRenderer: maxTailSeconds caps the tail length",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.1);
    // Long sustained note with a long release — the cap should win.
    pattern.addEvent(0, { 0.0, 0.05, 0.0f });

    auto cfg = defaultConfig();
    cfg.ampSustain     = 1.0f;
    cfg.ampRelease     = 5.0f;     // would naturally produce a 5 s tail
    cfg.maxTailSeconds = 0.2;

    const auto buf = PatternRenderer::render(pattern, cfg);
    const int patternSamples = static_cast<int>(0.1 * kSampleRate);
    const int maxAllowed     = patternSamples + static_cast<int>(0.2 * kSampleRate);
    REQUIRE(buf.getNumSamples() <= maxAllowed);
}

TEST_CASE("PatternRenderer: rendered audio respects gain setting",
          "[pattern][render]")
{
    Pattern pattern;
    pattern.setLengthSeconds(0.2);
    pattern.addEvent(0, { 0.0, 0.1, 0.0f });

    auto cfgFull = defaultConfig();
    cfgFull.gain = 1.0f;
    auto cfgHalf = defaultConfig();
    cfgHalf.gain = 0.5f;

    const auto bufFull = PatternRenderer::render(pattern, cfgFull);
    const auto bufHalf = PatternRenderer::render(pattern, cfgHalf);

    const float peakFull = peakAbs(bufFull);
    const float peakHalf = peakAbs(bufHalf);

    REQUIRE(peakFull > 0.05f);
    REQUIRE(peakHalf == Approx(peakFull * 0.5f).margin(0.02f));
}
