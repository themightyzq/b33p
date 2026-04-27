#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Paints the A-D-S-R shape derived from the four amp-envelope APVTS
    // parameters. Listens to parameter changes and repaints in response
    // so the curve tracks slider drags live.
    //
    // Uses proportional time scaling with a synthetic sustain display
    // length (0.1s + 0.3 * (A + D + R)) so the sustain segment stays
    // visible regardless of the A/D/R magnitudes. Does not attempt a
    // log-time axis — that is post-MVP polish if the proportional form
    // turns out to read poorly.
    class AmpEnvelopeVisualizer
        : public juce::Component
        , private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        explicit AmpEnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts);
        ~AmpEnvelopeVisualizer() override;

        void paint(juce::Graphics& g) override;

        // Switch which lane's amp-env params drive the painted curve.
        // Detaches the listener from the old lane's params and attaches
        // to the new lane's. Repaints once with the new values.
        void retargetLane(int lane);

    private:
        void parameterChanged(const juce::String& parameterID, float newValue) override;

        void detachListeners(int lane);
        void attachListeners(int lane);

        juce::AudioProcessorValueTreeState& apvts;
        int currentLane { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpEnvelopeVisualizer)
    };
}
