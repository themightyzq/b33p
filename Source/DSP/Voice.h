#pragma once

#include "DSP/AmpEnvelope.h"
#include "DSP/Bitcrush.h"
#include "DSP/Distortion.h"
#include "DSP/LowpassFilter.h"
#include "DSP/Oscillator.h"
#include "DSP/PitchEnvelope.h"

#include <vector>

namespace B33p
{
    // Monophonic composed voice. Owns one of each DSP primitive and
    // renders the fixed MVP signal chain sample-by-sample:
    //
    //     oscillator -> (x amp envelope)
    //                -> lowpass filter
    //                -> bitcrush
    //                -> distortion
    //                -> (x gain)
    //
    // The pitch envelope is a frequency modulator, not an audio
    // processor: each sample its semitone-offset output is combined
    // with basePitchHz and the per-event pitch offset to produce the
    // oscillator's instantaneous frequency.
    //
    // trigger() kicks off a new event: it latches the per-event pitch
    // offset, starts the pitch envelope's curve playback over the
    // given duration, and calls the amp envelope's noteOn(). The
    // pattern engine is expected to call noteOff() at event end;
    // the amp envelope's release tail then plays out.
    //
    // Filter, bitcrush, and distortion state intentionally carry
    // across triggers — zeroing them mid-ring would click. Only
    // reset() clears that state.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> any setters
    // in any order -> trigger -> processSample... -> noteOff ->
    // processSample until !isActive(). Before prepare(),
    // processSample() returns silence (0.0f).
    class Voice
    {
    public:
        void prepare(double sampleRate);
        void reset();

        void setWaveform(Oscillator::Waveform waveform);
        void setBasePitchHz(float hz);

        void setAmpAttack(float seconds);
        void setAmpDecay(float seconds);
        void setAmpSustain(float level);
        void setAmpRelease(float seconds);

        void setPitchCurve(const std::vector<PitchEnvelopePoint>& points);

        void setFilterCutoff(float hz);
        void setFilterResonance(float q);

        void setBitcrushBitDepth(float bits);
        void setBitcrushSampleRate(float targetHz);

        void setDistortionDrive(float drive);

        void setGain(float linearGain);

        void trigger(float durationSeconds, float pitchOffsetSemitones);
        void noteOff();

        float processSample();

        bool isActive() const;

    private:
        Oscillator    oscillator;
        AmpEnvelope   ampEnvelope;
        PitchEnvelope pitchEnvelope;
        LowpassFilter filter;
        Bitcrush      bitcrush;
        Distortion    distortion;

        float basePitchHz          { 440.0f };
        float pitchOffsetSemitones { 0.0f };
        float gain                 { 1.0f };
        bool  prepared             { false };
    };
}
