#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Paints the magnitude response of the per-lane lowpass filter.
    // Listens to filterCutoffHz + filterResonanceQ for the current
    // lane and repaints in response so the curve tracks slider drags
    // live, mirroring AmpEnvelopeVisualizer's pattern.
    //
    // The plot uses a log frequency axis (20 Hz .. 20 kHz) and a
    // linear dB axis (-36 dB .. +18 dB so the resonance peak is
    // always visible). The curve is computed from the analog
    // 2nd-order LPF transfer function:
    //
    //   |H(f)|_dB = -10 * log10((1 - r²)² + (r/Q)²)   where r = f / fc
    //
    // The digital biquad LowpassFilter actually used at runtime uses
    // bilinear-warped coefficients, so the digital response deviates
    // from this analog approximation near Nyquist — but the analog
    // form is the convention for filter response plots and matches
    // what users expect to see for a "12 dB/oct LPF with Q peak".
    class FilterResponseVisualizer
        : public juce::Component
        , private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        explicit FilterResponseVisualizer(juce::AudioProcessorValueTreeState& apvts);
        ~FilterResponseVisualizer() override;

        void paint(juce::Graphics& g) override;

        // Switch which lane's filter params drive the painted curve.
        // Detaches the listener from the old lane's params and attaches
        // to the new lane's. Repaints once with the new values.
        void retargetLane(int lane);

    private:
        void parameterChanged(const juce::String& parameterID, float newValue) override;

        void detachListeners(int lane);
        void attachListeners(int lane);

        juce::AudioProcessorValueTreeState& apvts;
        int currentLane { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterResponseVisualizer)
    };
}
