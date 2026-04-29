#include "FilterResponseVisualizer.h"

#include "Core/ParameterIDs.h"
#include "DSP/Filter.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float kOutlineInset   = 1.0f;
        constexpr float kCornerRadius   = 3.0f;
        constexpr float kInnerInset     = 6.0f;
        constexpr float kStrokeWidth    = 2.0f;

        // Plot range constants. dB range chosen so the resonance peak
        // (up to ~26 dB at Q=20) is mostly visible without dwarfing
        // the passband — anything above the top is just clipped.
        constexpr float kMinHz       = 20.0f;
        constexpr float kMaxHz       = 20000.0f;
        constexpr float kMaxDb       =  18.0f;
        constexpr float kMinDb       = -36.0f;
        constexpr int   kCurvePoints = 192;
        constexpr int   kNumFormants = 3;

        // 5x3 vowel formant table. Mirrors Filter.cpp's table so the
        // visualizer plots the same response the audio engine
        // produces. Kept in this TU so visualizer and engine can
        // diverge without forcing a shared header — the mismatch
        // would cost a misleading curve, not a crash.
        constexpr float kVowelFormants[Filter::kNumVowels][kNumFormants] = {
            { 700.0f, 1200.0f, 2500.0f },
            { 400.0f, 2000.0f, 2700.0f },
            { 270.0f, 2300.0f, 3000.0f },
            { 400.0f,  800.0f, 2400.0f },
            { 300.0f,  600.0f, 2300.0f },
        };
        constexpr float kFormantQ = 12.0f;

        // 2nd-order analog prototype magnitude. r = freq / fc.
        // For LP: |H|² = 1 / D, where D = (1-r²)² + (r/Q)².
        // For HP: numerator becomes r⁴.
        // For BP: numerator becomes (r/Q)².
        // We fall back to kMaxDb when D collapses to zero (a
        // resonance peak right at fc with absurd Q).
        float biquadDb(float r, float q, Filter::Type type)
        {
            const float r2 = r * r;
            const float real = 1.0f - r2;
            const float imag = r / std::max(q, 1e-3f);
            const float denom = real * real + imag * imag;
            if (denom <= 0.0f)
                return kMaxDb;

            float num = 1.0f;
            switch (type)
            {
                case Filter::Type::Lowpass:  num = 1.0f;             break;
                case Filter::Type::Highpass: num = r2 * r2;          break;
                case Filter::Type::Bandpass: num = imag * imag;      break;
                case Filter::Type::Comb:                              // unused — caller branches on type
                case Filter::Type::Formant:  num = 1.0f;             break;
            }
            return 10.0f * std::log10(std::max(num, 1e-12f) / denom);
        }

        // Feedback-comb magnitude in dB.
        // |H(f)|² = 1 / (1 - 2g·cos(ωD) + g²) where ω = 2π·f /
        // sampleRate and D in samples. Equivalently for a comb
        // peak fundamental fc, ωD = 2π·f / fc, so we don't need
        // the sample rate.
        float combDb(float freqHz, float cutoffHz, float feedback)
        {
            const float omegaD = 2.0f * 3.14159265358979323846f * (freqHz / cutoffHz);
            const float c      = std::cos(omegaD);
            const float denom  = 1.0f - 2.0f * feedback * c + feedback * feedback;
            if (denom <= 0.0f)
                return kMaxDb;
            return 10.0f * std::log10(1.0f / denom);
        }

        // Sum of three formant bandpasses' linear magnitude.
        // Vowel position is interpolated between the table entries
        // exactly as Filter.cpp does so the visualizer matches the
        // audio engine.
        float formantDb(float freqHz, float vowel01)
        {
            const float clamped = std::clamp(vowel01, 0.0f, 1.0f);
            const float pos     = clamped * static_cast<float>(Filter::kNumVowels - 1);
            const int   lo      = std::clamp(static_cast<int>(std::floor(pos)),
                                             0, Filter::kNumVowels - 1);
            const int   hi      = std::min(lo + 1, Filter::kNumVowels - 1);
            const float frac    = pos - static_cast<float>(lo);

            std::array<float, kNumFormants> formants;
            for (int i = 0; i < kNumFormants; ++i)
                formants[static_cast<size_t>(i)] =
                    kVowelFormants[lo][i] + frac * (kVowelFormants[hi][i] - kVowelFormants[lo][i]);

            // Sum the three bandpasses' magnitudes (linear, then
            // averaged like the engine does), then convert to dB.
            float sumLinear = 0.0f;
            for (int i = 0; i < kNumFormants; ++i)
            {
                const float r     = freqHz / formants[static_cast<size_t>(i)];
                const float r2    = r * r;
                const float real  = 1.0f - r2;
                const float imag  = r / kFormantQ;
                const float denom = real * real + imag * imag;
                if (denom > 0.0f)
                    sumLinear += std::sqrt((imag * imag) / denom);
            }
            sumLinear /= static_cast<float>(kNumFormants);
            if (sumLinear <= 0.0f)
                return kMinDb;
            return 20.0f * std::log10(sumLinear);
        }
    }

    FilterResponseVisualizer::FilterResponseVisualizer(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
    {
        attachListeners(currentLane);
    }

    FilterResponseVisualizer::~FilterResponseVisualizer()
    {
        detachListeners(currentLane);
    }

    void FilterResponseVisualizer::retargetLane(int lane)
    {
        if (lane == currentLane) return;
        detachListeners(currentLane);
        currentLane = lane;
        attachListeners(currentLane);
        repaint();
    }

    void FilterResponseVisualizer::attachListeners(int lane)
    {
        apvts.addParameterListener(ParameterIDs::filterType(lane),       this);
        apvts.addParameterListener(ParameterIDs::filterCutoffHz(lane),   this);
        apvts.addParameterListener(ParameterIDs::filterResonanceQ(lane), this);
        apvts.addParameterListener(ParameterIDs::filterVowel(lane),      this);
    }

    void FilterResponseVisualizer::detachListeners(int lane)
    {
        apvts.removeParameterListener(ParameterIDs::filterType(lane),       this);
        apvts.removeParameterListener(ParameterIDs::filterCutoffHz(lane),   this);
        apvts.removeParameterListener(ParameterIDs::filterResonanceQ(lane), this);
        apvts.removeParameterListener(ParameterIDs::filterVowel(lane),      this);
    }

    void FilterResponseVisualizer::parameterChanged(const juce::String&, float)
    {
        // juce::Component::repaint is thread-safe; no need to hop to
        // the message thread manually.
        repaint();
    }

    void FilterResponseVisualizer::paint(juce::Graphics& g)
    {
        const auto type = static_cast<Filter::Type>(juce::jlimit(0,
            static_cast<int>(Filter::Type::Formant),
            static_cast<int>(apvts.getRawParameterValue(ParameterIDs::filterType(currentLane))->load())));
        const float cutoff = apvts.getRawParameterValue(ParameterIDs::filterCutoffHz  (currentLane))->load();
        const float q      = apvts.getRawParameterValue(ParameterIDs::filterResonanceQ(currentLane))->load();
        const float vowel  = apvts.getRawParameterValue(ParameterIDs::filterVowel     (currentLane))->load();

        auto frame = getLocalBounds().toFloat().reduced(kOutlineInset);

        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(frame, kCornerRadius);

        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(frame, kCornerRadius, 1.0f);

        const auto plotArea = frame.reduced(kInnerInset);
        if (plotArea.getWidth() <= 0.0f || plotArea.getHeight() <= 0.0f)
            return;

        const float logMin = std::log10(kMinHz);
        const float logMax = std::log10(kMaxHz);
        const float logSpan = logMax - logMin;
        const float dbSpan  = kMaxDb - kMinDb;

        auto xForFreq = [&](float hz)
        {
            const float t = (std::log10(hz) - logMin) / logSpan;
            return plotArea.getX() + t * plotArea.getWidth();
        };
        auto yForDb = [&](float db)
        {
            const float t = juce::jlimit(0.0f, 1.0f, (db - kMinDb) / dbSpan);
            return plotArea.getBottom() - t * plotArea.getHeight();
        };

        // 0 dB reference line — anchors the eye so the user can tell
        // passband from boost/cut at a glance.
        const float zeroDbY = yForDb(0.0f);
        g.setColour(juce::Colour::fromRGB(50, 50, 50));
        g.drawHorizontalLine((int) zeroDbY,
                             plotArea.getX(), plotArea.getRight());

        // Vertical guide at the cutoff frequency for the modes that
        // actually use it. Formant ignores cutoff (vowel-driven), so
        // we skip the guide there to avoid suggesting otherwise.
        if (type != Filter::Type::Formant)
        {
            const float cutoffX = xForFreq(juce::jlimit(kMinHz, kMaxHz, cutoff));
            g.setColour(juce::Colour::fromRGB(120, 200, 255).withAlpha(0.35f));
            g.drawVerticalLine((int) cutoffX,
                               plotArea.getY(), plotArea.getBottom());
        }

        // Map Q to the same comb feedback the engine uses, so the
        // visualizer's peak heights track the engine's output.
        auto combFeedback = [&](float qVal)
        {
            const float t = juce::jlimit(0.0f, 1.0f,
                                          (qVal - 0.1f) / (20.0f - 0.1f));
            return t * 0.95f;
        };

        // Magnitude curve — sample at log-spaced points so the
        // resonance peak (which sits around fc) gets enough
        // resolution wherever the user has placed it.
        juce::Path response;
        for (int i = 0; i < kCurvePoints; ++i)
        {
            const float t  = (float) i / (float) (kCurvePoints - 1);
            const float hz = std::pow(10.0f, logMin + t * logSpan);

            float db = kMinDb;
            switch (type)
            {
                case Filter::Type::Lowpass:
                case Filter::Type::Highpass:
                case Filter::Type::Bandpass:
                    db = biquadDb(hz / cutoff, q, type);
                    break;
                case Filter::Type::Comb:
                    db = combDb(hz, cutoff, combFeedback(q));
                    break;
                case Filter::Type::Formant:
                    db = formantDb(hz, vowel);
                    break;
            }
            const float x = plotArea.getX() + t * plotArea.getWidth();
            const float y = yForDb(db);

            if (i == 0) response.startNewSubPath(x, y);
            else        response.lineTo(x, y);
        }

        // Filled "passband" beneath the curve for visual weight,
        // matching the amp-envelope visualizer's style.
        juce::Path filled = response;
        filled.lineTo(plotArea.getRight(), plotArea.getBottom());
        filled.lineTo(plotArea.getX(),     plotArea.getBottom());
        filled.closeSubPath();

        g.setColour(juce::Colour::fromRGB(90, 180, 255).withAlpha(0.18f));
        g.fillPath(filled);

        g.setColour(juce::Colour::fromRGB(120, 200, 255));
        g.strokePath(response, juce::PathStrokeType(kStrokeWidth));
    }
}
