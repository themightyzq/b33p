#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Paints the magnitude response of the per-lane filter, picking
    // the appropriate analytical curve based on the lane's
    // filter_type parameter (Lowpass / Highpass / Bandpass / Comb /
    // Formant). Listens to filter_type, filter_cutoff_hz,
    // filter_resonance_q, and filter_vowel for the current lane and
    // repaints in response so the curve tracks slider drags live.
    //
    // The plot uses a log frequency axis (20 Hz .. 20 kHz) and a
    // linear dB axis (-36 dB .. +18 dB so the resonance peak is
    // always visible). For LP/HP/BP the analog 2nd-order transfer
    // function is used; for Comb the closed-form feedback-comb
    // magnitude; for Formant the sum of three tuned bandpasses.
    // The digital biquads actually used at runtime use
    // bilinear-warped coefficients, so the digital response deviates
    // from these analog approximations near Nyquist — but the
    // analog forms are the convention for filter response plots.
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
