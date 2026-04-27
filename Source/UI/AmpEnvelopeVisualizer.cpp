#include "AmpEnvelopeVisualizer.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    namespace
    {
        constexpr float kOutlineInset   = 1.0f;
        constexpr float kCornerRadius   = 3.0f;
        constexpr float kInnerInset     = 6.0f;
        constexpr float kStrokeWidth    = 2.0f;

        // Synthetic sustain display length: a small baseline plus a
        // fraction of the other stages' total length. Ensures the
        // sustain segment is always visible without letting it
        // dominate when A/D/R are short.
        float sustainDisplaySeconds(float a, float d, float r)
        {
            return 0.1f + 0.3f * (a + d + r);
        }
    }

    AmpEnvelopeVisualizer::AmpEnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
    {
        attachListeners(currentLane);
    }

    AmpEnvelopeVisualizer::~AmpEnvelopeVisualizer()
    {
        detachListeners(currentLane);
    }

    void AmpEnvelopeVisualizer::retargetLane(int lane)
    {
        if (lane == currentLane) return;
        detachListeners(currentLane);
        currentLane = lane;
        attachListeners(currentLane);
        repaint();
    }

    void AmpEnvelopeVisualizer::attachListeners(int lane)
    {
        apvts.addParameterListener(ParameterIDs::ampAttack(lane),  this);
        apvts.addParameterListener(ParameterIDs::ampDecay(lane),   this);
        apvts.addParameterListener(ParameterIDs::ampSustain(lane), this);
        apvts.addParameterListener(ParameterIDs::ampRelease(lane), this);
    }

    void AmpEnvelopeVisualizer::detachListeners(int lane)
    {
        apvts.removeParameterListener(ParameterIDs::ampAttack(lane),  this);
        apvts.removeParameterListener(ParameterIDs::ampDecay(lane),   this);
        apvts.removeParameterListener(ParameterIDs::ampSustain(lane), this);
        apvts.removeParameterListener(ParameterIDs::ampRelease(lane), this);
    }

    void AmpEnvelopeVisualizer::parameterChanged(const juce::String&, float)
    {
        // juce::Component::repaint is thread-safe; no need to hop to
        // the message thread manually.
        repaint();
    }

    void AmpEnvelopeVisualizer::paint(juce::Graphics& g)
    {
        const float a = apvts.getRawParameterValue(ParameterIDs::ampAttack(currentLane))->load();
        const float d = apvts.getRawParameterValue(ParameterIDs::ampDecay(currentLane))->load();
        const float s = apvts.getRawParameterValue(ParameterIDs::ampSustain(currentLane))->load();
        const float r = apvts.getRawParameterValue(ParameterIDs::ampRelease(currentLane))->load();

        auto frame = getLocalBounds().toFloat().reduced(kOutlineInset);

        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(frame, kCornerRadius);

        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(frame, kCornerRadius, 1.0f);

        const float sustainSeconds = sustainDisplaySeconds(a, d, r);
        const float totalSeconds   = a + d + sustainSeconds + r;
        if (totalSeconds <= 0.0f)
            return;

        const auto plotArea = frame.reduced(kInnerInset);
        const float pxPerSec = plotArea.getWidth() / totalSeconds;
        const float yBaseline = plotArea.getBottom();
        const float yPeak     = plotArea.getY();

        auto yFor = [&](float level)
        {
            return yBaseline - juce::jlimit(0.0f, 1.0f, level) * plotArea.getHeight();
        };

        juce::Path envelope;
        envelope.startNewSubPath(plotArea.getX(), yBaseline);

        float x = plotArea.getX() + a * pxPerSec;
        envelope.lineTo(x, yPeak);

        x += d * pxPerSec;
        envelope.lineTo(x, yFor(s));

        x += sustainSeconds * pxPerSec;
        envelope.lineTo(x, yFor(s));

        x += r * pxPerSec;
        envelope.lineTo(x, yBaseline);

        envelope.closeSubPath();

        g.setColour(juce::Colour::fromRGB(90, 180, 255).withAlpha(0.22f));
        g.fillPath(envelope);

        g.setColour(juce::Colour::fromRGB(120, 200, 255));
        g.strokePath(envelope, juce::PathStrokeType(kStrokeWidth));
    }
}
