#include "AmpEnvelopeVisualizer.h"

#include "Core/ParameterIDs.h"
#include "DSP/AmpEnvelope.h"
#include "State/B33pProcessor.h"

namespace B33p
{
    namespace
    {
        constexpr float kOutlineInset   = 1.0f;
        constexpr float kCornerRadius   = 3.0f;
        constexpr float kInnerInset     = 6.0f;
        constexpr float kStrokeWidth    = 2.0f;
        constexpr float kPlayheadWidth  = 1.5f;

        // Synthetic sustain display length: a small baseline plus a
        // fraction of the other stages' total length. Ensures the
        // sustain segment is always visible without letting it
        // dominate when A/D/R are short.
        float sustainDisplaySeconds(float a, float d, float r)
        {
            return 0.1f + 0.3f * (a + d + r);
        }
    }

    AmpEnvelopeVisualizer::AmpEnvelopeVisualizer(B33pProcessor& processorRef)
        : processor(processorRef),
          apvts(processorRef.getApvts())
    {
        attachListeners(currentLane);
        // 30 Hz playhead repaint while audio is playing. The gate inside
        // timerCallback ensures the timer self-elides repaints when the
        // selected lane is idle (accessibility: no motion when silent).
        startTimerHz(30);
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

    void AmpEnvelopeVisualizer::timerCallback()
    {
        // Accessibility: only repaint while audio is playing (so the
        // playhead actually moves) plus one final tick on the
        // playing→idle edge so the playhead clears instead of freezing
        // at its last position.
        const bool playing = processor.isSelectedLaneVoiceActive();
        if (playing || wasPlayingLastTick)
            repaint();
        wasPlayingLastTick = playing;
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

        // Live playhead (P31). Only painted while the selected lane's
        // voice is producing audio. Stage + elapsed-in-stage come from
        // the processor's per-block mirror; we map them onto the same
        // x axis the envelope curve uses so the playhead reads cleanly
        // as "where we are on this curve."
        if (! processor.isSelectedLaneVoiceActive())
            return;

        const auto stageInt = processor.getSelectedLaneAmpEnvStageInt();
        const float elapsed = processor.getSelectedLaneAmpEnvElapsedSec();
        const auto  stage   = static_cast<AmpEnvelope::Stage>(stageInt);

        // The visualizer's x axis: 0=plotArea.getX, then a, then d,
        // then sustainSeconds, then r. Compute the playhead's x by
        // accumulating offsets per the active stage.
        const float xAttackEnd  = plotArea.getX() + a * pxPerSec;
        const float xDecayEnd   = xAttackEnd + d * pxPerSec;
        const float xSustainEnd = xDecayEnd  + sustainSeconds * pxPerSec;
        const float xReleaseEnd = xSustainEnd + r * pxPerSec;

        float playheadX = plotArea.getX();
        switch (stage)
        {
            case AmpEnvelope::Stage::Idle:
                return;   // guarded above, but defensive
            case AmpEnvelope::Stage::Attack:
                playheadX = plotArea.getX() + std::min(elapsed, a) * pxPerSec;
                break;
            case AmpEnvelope::Stage::Decay:
                playheadX = xAttackEnd + std::min(elapsed, d) * pxPerSec;
                break;
            case AmpEnvelope::Stage::Sustain:
                // Park at the start of the sustain region — sustain holds
                // a constant level by definition, so showing the playhead
                // walking across it would just be motion for its own
                // sake. The "we're holding here" state reads as a
                // stationary playhead, which matches the mental model.
                playheadX = xDecayEnd;
                break;
            case AmpEnvelope::Stage::Release:
                playheadX = xSustainEnd + std::min(elapsed, r) * pxPerSec;
                break;
        }
        playheadX = juce::jlimit(plotArea.getX(), xReleaseEnd, playheadX);

        g.setColour(juce::Colour::fromRGB(220, 235, 255).withAlpha(0.85f));
        g.fillRect(juce::Rectangle<float>(playheadX - kPlayheadWidth * 0.5f,
                                          plotArea.getY(),
                                          kPlayheadWidth,
                                          plotArea.getHeight()));
    }
}
