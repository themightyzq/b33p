#include "FilterResponseVisualizer.h"

#include "Core/ParameterIDs.h"

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

        float magnitudeDb(float freqHz, float cutoffHz, float q)
        {
            const float r  = freqHz / cutoffHz;
            const float r2 = r * r;
            const float real = 1.0f - r2;
            const float imag = r / q;
            const float denom = real * real + imag * imag;
            if (denom <= 0.0f)
                return kMaxDb;
            return -10.0f * std::log10(denom);
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
        apvts.addParameterListener(ParameterIDs::filterCutoffHz(lane),   this);
        apvts.addParameterListener(ParameterIDs::filterResonanceQ(lane), this);
    }

    void FilterResponseVisualizer::detachListeners(int lane)
    {
        apvts.removeParameterListener(ParameterIDs::filterCutoffHz(lane),   this);
        apvts.removeParameterListener(ParameterIDs::filterResonanceQ(lane), this);
    }

    void FilterResponseVisualizer::parameterChanged(const juce::String&, float)
    {
        // juce::Component::repaint is thread-safe; no need to hop to
        // the message thread manually.
        repaint();
    }

    void FilterResponseVisualizer::paint(juce::Graphics& g)
    {
        const float cutoff = apvts.getRawParameterValue(ParameterIDs::filterCutoffHz  (currentLane))->load();
        const float q      = apvts.getRawParameterValue(ParameterIDs::filterResonanceQ(currentLane))->load();

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

        // Vertical guide at the cutoff frequency, drawn in the lane
        // accent colour (set by Section::setAccentColour). We don't
        // know it here, so use a neutral accent that reads on the
        // dark background.
        const float cutoffX = xForFreq(juce::jlimit(kMinHz, kMaxHz, cutoff));
        g.setColour(juce::Colour::fromRGB(120, 200, 255).withAlpha(0.35f));
        g.drawVerticalLine((int) cutoffX,
                           plotArea.getY(), plotArea.getBottom());

        // Magnitude curve — sample at log-spaced points so the
        // resonance peak (which sits around fc) gets enough
        // resolution wherever the user has placed it.
        juce::Path response;
        for (int i = 0; i < kCurvePoints; ++i)
        {
            const float t  = (float) i / (float) (kCurvePoints - 1);
            const float hz = std::pow(10.0f, logMin + t * logSpan);
            const float db = magnitudeDb(hz, cutoff, q);
            const float x  = plotArea.getX() + t * plotArea.getWidth();
            const float y  = yForDb(db);

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
