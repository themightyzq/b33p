#pragma once

#include "DSP/AmpEnvelope.h"
#include "DSP/Filter.h"
#include "DSP/ModulationEffect.h"
#include "DSP/Oscillator.h"
#include "DSP/OversampledBitcrush.h"
#include "DSP/OversampledDistortion.h"
#include "DSP/PitchEnvelope.h"

#include <vector>

namespace B33p
{
    // Monophonic composed voice. Owns one of each DSP primitive and
    // renders the fixed MVP signal chain sample-by-sample:
    //
    //     oscillator -> (x amp envelope)
    //                -> lowpass filter
    //                -> bitcrush (4x oversampled, see OversampledBitcrush)
    //                -> distortion (4x oversampled, see OversampledDistortion)
    //                -> (x gain)
    //
    // The bitcrush and distortion stages are each individually wrapped
    // with 4x oversampling to anti-alias their nonlinearities; that adds
    // a small, fixed amount of algorithmic latency (getLatencySamples()
    // below) that B33pProcessor reports via setLatencySamples().
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

        // Replaces the custom-waveform table the oscillator reads
        // when waveform is Custom. Empty table = silence. Convenience
        // alias for setWavetableSlot(0, samples).
        void setCustomWaveformTable(const std::vector<float>& samples);

        // Per-slot replacement for the wavetable storage. slot is
        // clamped to [0, Oscillator::kNumWavetableSlots). Used by
        // Wavetable mode; Custom mode reads slot 0 only.
        void setWavetableSlot(int slot, const std::vector<float>& samples);

        // 0..1 morph position used by Wavetable mode to blend between
        // the four slots. Ignored in Custom / built-in waveform modes.
        void setWavetableMorph(float morph01);

        // Two-operator FM: ratio = modulator/carrier frequency,
        // depth = modulation index (0 = pure carrier sine). Both
        // ignored outside FM mode.
        void setFmRatio(float ratio);
        void setFmDepth(float depth);

        // Ring modulation: ratio sets the modulator pitch relative
        // to the carrier; mix is a 0..1 wet/dry crossfade between
        // the dry carrier sine and the multiplied product. Both
        // ignored outside Ring mode.
        void setRingRatio(float ratio);
        void setRingMix(float mix01);

        void setAmpAttack(float seconds);
        void setAmpDecay(float seconds);
        void setAmpSustain(float level);
        void setAmpRelease(float seconds);

        void setPitchCurve(const std::vector<PitchEnvelopePoint>& points);

        void setFilterType(Filter::Type type);
        void setFilterCutoff(float hz);
        void setFilterResonance(float q);
        void setFilterVowel(float vowel01);

        void setBitcrushBitDepth(float bits);
        void setBitcrushSampleRate(float targetHz);

        void setDistortionDrive(float drive);

        // Modulation effect slot at the end of the chain.
        // setModEffectType picks the active mode; the three
        // continuous setters carry type-dependent semantics — see
        // ModulationEffect.h for the per-type meaning.
        void setModEffectType(ModulationEffect::Type type);
        void setModEffectParam1(float v01);
        void setModEffectParam2(float v01);
        void setModEffectMix(float v01);

        void setGain(float linearGain);

        // velocity is a per-trigger 0..1 scalar applied on top of
        // gain. Defaults to 1.0 so existing callers (audition,
        // tests) keep their previous behaviour.
        void trigger(float durationSeconds,
                     float pitchOffsetSemitones,
                     float velocity = 1.0f);
        void noteOff();

        // Advances the oscillator -> amp envelope -> filter chain by one
        // sample and returns the result BEFORE the bitcrush / distortion /
        // modEffect / gain stages. Exists so B33pProcessor can batch the
        // (expensive) oversampled bitcrush + distortion stages across a
        // whole block instead of paying juce::dsp::Oversampling's per-call
        // overhead once per single sample -- see applyEffectsBlock() below
        // and OversamplingConfig.h. trigger()/noteOff() must still be
        // called once per sample before generateCore() for that sample,
        // exactly as before this split -- only where the bitcrush/
        // distortion work happens moved, not when triggers take effect.
        float generateCore();

        // Runs the bitcrush -> distortion -> modEffect -> gain tail of the
        // chain across a whole block of pre-effect samples produced by
        // generateCore() (`buffer`, overwritten in place with the final
        // output) and the per-sample trigger velocities in effect when
        // each was generated (`velocities`, same length -- a mid-block
        // retrigger changes triggerVelocity for the rest of the block, so
        // the caller snapshots it per sample via getTriggerVelocity()
        // rather than relying on the member's value after the fact).
        // numSamples should not exceed OversamplingConfig.h's
        // kMaxOversampledBlockSize; larger spans still work (bitcrush /
        // distortion chunk internally) but lose some of the batching
        // benefit. No allocation -- safe on the audio thread.
        void applyEffectsBlock(float* buffer, const float* velocities, int numSamples);

        // Convenience single-sample form: generateCore() followed by
        // applyEffectsBlock() on a one-sample span. Produces output
        // identical to calling those two directly, and is what all
        // existing single-sample callers (unit tests, the B33pRenderVoice
        // CLI) use.
        float processSample();

        // The trigger velocity latched by the most recent trigger() call
        // (0..1). B33pProcessor snapshots this once per sample right after
        // calling generateCore() so applyEffectsBlock() applies the value
        // that was in effect at generation time, even if a later sample in
        // the same block retriggers the voice with a different velocity.
        float getTriggerVelocity() const noexcept { return triggerVelocity; }

        bool isActive() const;

        // Combined latency (samples) added by the bitcrush + distortion
        // stages' 4x oversampling (OversampledBitcrush /
        // OversampledDistortion). 0 before prepare(). B33pProcessor
        // reports this via setLatencySamples() so host PDC compensates
        // correctly.
        int getLatencySamples() const;

        // Snapshots of the internal envelopes' current output, used by
        // the UI's modulation-glow halos on the gain + base-pitch knobs.
        // Const accessors; safe to call from any thread relative to
        // processSample as long as the caller tolerates a one-sample
        // race (the read is racey by design — UI just needs a recent
        // sample for the pulse, not a synchronised one).
        float getAmpEnvelopeLevel()  const { return ampEnvelope  .getCurrentLevel(); }
        float getPitchEnvelopeValue() const { return pitchEnvelope.getCurrentValue(); }

        // Snapshots of which amp-env stage is currently running and how
        // far through it (seconds). Drive the AmpEnvelopeVisualizer's
        // live playhead (P31).
        AmpEnvelope::Stage getAmpEnvelopeStage()           const { return ampEnvelope.getStage(); }
        float              getAmpEnvelopeStageElapsedSec() const { return ampEnvelope.getStageElapsedSeconds(); }

    private:
        Oscillator       oscillator;
        AmpEnvelope      ampEnvelope;
        PitchEnvelope    pitchEnvelope;
        Filter                filter;
        OversampledBitcrush   bitcrush;
        OversampledDistortion distortion;
        ModulationEffect      modEffect;

        float basePitchHz          { 440.0f };
        float pitchOffsetSemitones { 0.0f };
        float gain                 { 1.0f };
        float triggerVelocity      { 1.0f };
        bool  prepared             { false };

        // Per-sample gain smoother — 10 ms ramp. Fast voice_gain
        // automation otherwise zippers the final stage of the voice
        // (CLAUDE.md "Parameter smoothing"; voice_gain is excluded
        // from the randomizer but the user can still automate it).
        juce::SmoothedValue<float> gainSmoother;
        bool                       firstGainSetAfterPrepare { true };
    };
}
