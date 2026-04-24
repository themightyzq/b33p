#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Core/ParameterIDs.h"
#include "State/B33pProcessor.h"
#include "State/ParameterLayout.h"

#include <juce_audio_processors/juce_audio_processors.h>

using Catch::Approx;

namespace
{
    struct FloatExpectation
    {
        const char* id;
        float       min;
        float       max;
        float       defaultValue;
    };

    juce::AudioParameterFloat* asFloat(juce::RangedAudioParameter* p)
    {
        return dynamic_cast<juce::AudioParameterFloat*>(p);
    }

    juce::AudioParameterChoice* asChoice(juce::RangedAudioParameter* p)
    {
        return dynamic_cast<juce::AudioParameterChoice*>(p);
    }
}

TEST_CASE("ParameterLayout: B33pProcessor constructs and exposes APVTS", "[state][layout]")
{
    REQUIRE_NOTHROW(B33p::B33pProcessor{});
}

TEST_CASE("ParameterLayout: every expected parameter ID is present", "[state][layout]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    const char* const expectedIds[] = {
        B33p::ParameterIDs::oscWaveform,
        B33p::ParameterIDs::basePitchHz,
        B33p::ParameterIDs::ampAttack,
        B33p::ParameterIDs::ampDecay,
        B33p::ParameterIDs::ampSustain,
        B33p::ParameterIDs::ampRelease,
        B33p::ParameterIDs::filterCutoffHz,
        B33p::ParameterIDs::filterResonanceQ,
        B33p::ParameterIDs::bitcrushBitDepth,
        B33p::ParameterIDs::bitcrushSampleRateHz,
        B33p::ParameterIDs::distortionDrive,
        B33p::ParameterIDs::voiceGain,
    };

    for (const char* id : expectedIds)
        REQUIRE(apvts.getParameter(id) != nullptr);
}

TEST_CASE("ParameterLayout: continuous parameters have correct ranges and defaults", "[state][layout]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    const FloatExpectation expectations[] = {
        { B33p::ParameterIDs::basePitchHz,          20.0f,    20000.0f, 440.0f  },
        { B33p::ParameterIDs::ampAttack,            0.0f,     5.0f,     0.005f  },
        { B33p::ParameterIDs::ampDecay,             0.0f,     5.0f,     0.05f   },
        { B33p::ParameterIDs::ampSustain,           0.0f,     1.0f,     1.0f    },
        { B33p::ParameterIDs::ampRelease,           0.0f,     5.0f,     0.1f    },
        { B33p::ParameterIDs::filterCutoffHz,       20.0f,    20000.0f, 20000.0f },
        { B33p::ParameterIDs::filterResonanceQ,     0.1f,     20.0f,    0.707f  },
        { B33p::ParameterIDs::bitcrushBitDepth,     1.0f,     16.0f,    16.0f   },
        { B33p::ParameterIDs::bitcrushSampleRateHz, 200.0f,   48000.0f, 48000.0f },
        { B33p::ParameterIDs::distortionDrive,      0.1f,     100.0f,   1.0f    },
        { B33p::ParameterIDs::voiceGain,            0.0f,     10.0f,    1.0f    },
    };

    for (const FloatExpectation& e : expectations)
    {
        auto* floatParam = asFloat(apvts.getParameter(e.id));
        INFO("parameter id: " << e.id);
        REQUIRE(floatParam != nullptr);

        const auto& range = floatParam->getNormalisableRange();
        REQUIRE(range.start == Approx(e.min).margin(1e-6f));
        REQUIRE(range.end   == Approx(e.max).margin(1e-6f));
        REQUIRE(floatParam->get() == Approx(e.defaultValue).margin(1e-5f));
    }
}

TEST_CASE("ParameterLayout: waveform choice lists all five waveforms with Sine default", "[state][layout]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    auto* choice = asChoice(apvts.getParameter(B33p::ParameterIDs::oscWaveform));
    REQUIRE(choice != nullptr);

    const juce::StringArray expected { "Sine", "Square", "Triangle", "Saw", "Noise" };
    REQUIRE(choice->choices == expected);
    REQUIRE(choice->getIndex() == 0);  // Sine
}

TEST_CASE("ParameterLayout: Hz parameters are log-skewed around their centre", "[state][layout]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    // 0.5 normalized must map near the declared skew centre, not the
    // linear midpoint of the range. This confirms a skew was applied
    // rather than left linear.
    struct SkewCheck { const char* id; float centre; };
    const SkewCheck checks[] = {
        { B33p::ParameterIDs::basePitchHz,          440.0f  },
        { B33p::ParameterIDs::filterCutoffHz,       1000.0f },
        { B33p::ParameterIDs::bitcrushSampleRateHz, 8000.0f },
    };

    for (const SkewCheck& c : checks)
    {
        auto* param = asFloat(apvts.getParameter(c.id));
        INFO("parameter id: " << c.id);
        REQUIRE(param != nullptr);

        const float mapped = param->getNormalisableRange().convertFrom0to1(0.5f);
        // Tolerance 1 Hz: setSkewForCentre is exact in theory, but
        // float round-trip through the normalised range adds noise.
        REQUIRE(mapped == Approx(c.centre).margin(1.0f));
    }
}

TEST_CASE("ParameterLayout: sustain and Q stay linear (no skew applied)", "[state][layout]")
{
    B33p::B33pProcessor processor;
    auto& apvts = processor.getApvts();

    auto* sustain = asFloat(apvts.getParameter(B33p::ParameterIDs::ampSustain));
    REQUIRE(sustain != nullptr);
    REQUIRE(sustain->getNormalisableRange().convertFrom0to1(0.5f) == Approx(0.5f).margin(1e-5f));

    auto* q = asFloat(apvts.getParameter(B33p::ParameterIDs::filterResonanceQ));
    REQUIRE(q != nullptr);
    // Linear midpoint of [0.1, 20] is 10.05.
    REQUIRE(q->getNormalisableRange().convertFrom0to1(0.5f) == Approx(10.05f).margin(1e-4f));
}
