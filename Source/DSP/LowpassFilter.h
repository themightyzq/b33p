#pragma once

#include <juce_dsp/juce_dsp.h>

namespace B33p
{
    // 12 dB/octave biquad lowpass with cutoff and resonance controls.
    //
    // Wraps juce::dsp::IIR::Filter<float> with RBJ-style low-pass
    // coefficients. "Resonance" is expressed as Q: 0.707 is flat
    // Butterworth, higher values produce a cutoff-frequency peak. The
    // APVTS layer will eventually map a normalized 0..1 slider onto
    // this Q range; this class speaks in Q directly.
    //
    // Cutoff is clamped to [20 Hz, 0.499 * sampleRate]; Q is clamped
    // to [0.1, 20]. Parameters can be set before prepare() and take
    // effect once prepare() installs coefficients.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setCutoff /
    // setResonance -> processSample ... . Before prepare(),
    // processSample() returns silence (0.0f).
    class LowpassFilter
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setCutoff(float hz);
        void setResonance(float q);

        float processSample(float input);

    private:
        void updateCoefficients();

        double sampleRate { 0.0 };
        float  cutoffHz   { 20000.0f };
        float  resonanceQ { 0.707f };
        bool   prepared   { false };

        juce::dsp::IIR::Filter<float> filter;
    };
}
