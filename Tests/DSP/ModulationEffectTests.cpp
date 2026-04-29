#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "DSP/ModulationEffect.h"

#include <cmath>
#include <vector>

using B33p::ModulationEffect;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kTwoPi      = 6.28318530717958647692;

    // Renders a unit sine through the effect at full mix and
    // returns the RMS of the wet output. Long warmup so any
    // delay-line / reverb tail stabilises.
    double rmsAtFullMix(ModulationEffect::Type type, float p1, float p2,
                        double frequencyHz, int warmup, int measure)
    {
        ModulationEffect fx;
        fx.prepare(kSampleRate);
        fx.setType(type);
        fx.setParam1(p1);
        fx.setParam2(p2);
        fx.setMix   (1.0f);

        double phase = 0.0;
        const double phaseInc = kTwoPi * frequencyHz / kSampleRate;

        for (int i = 0; i < warmup; ++i)
        {
            (void) fx.processSample(static_cast<float>(std::sin(phase)));
            phase += phaseInc;
        }

        double sumSq = 0.0;
        for (int i = 0; i < measure; ++i)
        {
            const float y = fx.processSample(static_cast<float>(std::sin(phase)));
            phase += phaseInc;
            sumSq += static_cast<double>(y) * static_cast<double>(y);
        }
        return std::sqrt(sumSq / static_cast<double>(measure));
    }
}

TEST_CASE("ModulationEffect: None type passes input through unchanged", "[dsp][modfx]")
{
    ModulationEffect fx;
    fx.prepare(kSampleRate);
    fx.setType(ModulationEffect::Type::None);
    fx.setParam1(0.7f);   // values shouldn't matter in None mode
    fx.setParam2(0.3f);
    fx.setMix   (1.0f);

    for (int i = 0; i < 256; ++i)
    {
        const float x = 0.5f * std::sin(0.01f * static_cast<float>(i));
        REQUIRE(fx.processSample(x) == Approx(x).margin(1e-6f));
    }
}

TEST_CASE("ModulationEffect: processSample without prepare returns input unchanged", "[dsp][modfx]")
{
    ModulationEffect fx;
    fx.setType(ModulationEffect::Type::Chorus);
    fx.setMix(1.0f);

    for (int i = 0; i < 64; ++i)
        REQUIRE(fx.processSample(0.42f) == Approx(0.42f).margin(1e-6f));
}

TEST_CASE("ModulationEffect: every active type produces non-silent output for a sine in",
          "[dsp][modfx]")
{
    // Smoke test — for every non-None type, a unit sine through
    // the effect at full mix should produce non-trivial output.
    // Catches any wiring issue where a mode silently bypasses.
    using T = ModulationEffect::Type;
    const T types[] = { T::Chorus, T::Reverb, T::Delay, T::Flanger, T::Phaser };

    for (auto t : types)
    {
        const double rms = rmsAtFullMix(t, 0.5f, 0.5f, 440.0, 4000, 4000);
        REQUIRE(rms > 0.05);
    }
}

TEST_CASE("ModulationEffect: Delay at high feedback echoes a single impulse for many samples",
          "[dsp][modfx]")
{
    ModulationEffect fx;
    fx.prepare(kSampleRate);
    fx.setType (ModulationEffect::Type::Delay);
    fx.setParam1(0.5f);   // ~45 ms delay (log-skewed midpoint)
    fx.setParam2(0.95f);  // near-max feedback
    fx.setMix   (1.0f);

    // One impulse, then zeros. A high-feedback delay must keep
    // producing observable echoes for many delay taps. We sample
    // a window starting after the first echo lands but well before
    // the feedback decay collapses to noise.
    fx.processSample(1.0f);
    float maxAbsTail = 0.0f;
    for (int i = 1; i < 12000; ++i)
    {
        const float y = fx.processSample(0.0f);
        // Window from sample 4000 to 8000: ~1.5 to 3 delay taps in
        // at 45 ms delay. Even with feedback decay, levels stay
        // well above any kind of numerical floor here.
        if (i > 4000 && i < 8000)
            maxAbsTail = std::max(maxAbsTail, std::abs(y));
    }
    REQUIRE(maxAbsTail > 0.1f);
}

TEST_CASE("ModulationEffect: type switch resets prior delay-line memory", "[dsp][modfx]")
{
    ModulationEffect fx;
    fx.prepare(kSampleRate);

    // Build up tail energy in Delay mode.
    fx.setType (ModulationEffect::Type::Delay);
    fx.setParam1(0.3f);
    fx.setParam2(0.9f);
    fx.setMix   (1.0f);
    for (int i = 0; i < 4000; ++i)
        (void) fx.processSample(static_cast<float>(std::sin(0.05 * i)));

    // Switch to None — output should be a clean passthrough of zero
    // immediately, with no tail leaking through.
    fx.setType(ModulationEffect::Type::None);
    for (int i = 0; i < 256; ++i)
        REQUIRE(fx.processSample(0.0f) == Approx(0.0f).margin(1e-6f));
}
