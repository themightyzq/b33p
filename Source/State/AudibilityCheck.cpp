#include "AudibilityCheck.h"

#include "Core/ParameterIDs.h"
#include "DSP/Filter.h"
#include "DSP/ModulationEffect.h"
// Filter::Type / Oscillator::Waveform are referenced as choice-param
// indices by the fallback below.
#include "DSP/Oscillator.h"
#include "DSP/Voice.h"
#include "Pattern/Pattern.h"

#include <algorithm>
#include <cmath>

namespace B33p::AudibilityCheck
{
    namespace
    {
        // Static-only mirror of B33pProcessor::pushParametersToLane —
        // sets every voice parameter from the raw APVTS lane value.
        // KEEP THIS IN SYNC with pushParametersToLane: any setter
        // added there must be added here too, or the offline
        // audibility render silently drifts from the live audition.
        void configureVoiceFromApvts(Voice& v,
                                     juce::AudioProcessorValueTreeState& apvts,
                                     int lane)
        {
            const auto load = [&](const juce::String& id) -> float
            {
                if (auto* raw = apvts.getRawParameterValue(id))
                    return raw->load();
                return 0.0f;
            };

            v.setWaveform(static_cast<Oscillator::Waveform>(
                juce::jlimit(0,
                             static_cast<int>(Oscillator::Waveform::Ring),
                             static_cast<int>(load(ParameterIDs::oscWaveform(lane))))));
            v.setBasePitchHz       (load(ParameterIDs::basePitchHz(lane)));
            v.setWavetableMorph    (load(ParameterIDs::wavetableMorph(lane)));
            v.setFmRatio           (load(ParameterIDs::fmRatio(lane)));
            v.setFmDepth           (load(ParameterIDs::fmDepth(lane)));
            v.setRingRatio         (load(ParameterIDs::ringRatio(lane)));
            v.setRingMix           (load(ParameterIDs::ringMix(lane)));
            v.setAmpAttack         (load(ParameterIDs::ampAttack(lane)));
            v.setAmpDecay          (load(ParameterIDs::ampDecay(lane)));
            v.setAmpSustain        (load(ParameterIDs::ampSustain(lane)));
            v.setAmpRelease        (load(ParameterIDs::ampRelease(lane)));
            v.setFilterType(static_cast<Filter::Type>(
                juce::jlimit(0,
                             static_cast<int>(Filter::Type::Formant),
                             static_cast<int>(load(ParameterIDs::filterType(lane))))));
            v.setFilterCutoff      (load(ParameterIDs::filterCutoffHz(lane)));
            v.setFilterResonance   (load(ParameterIDs::filterResonanceQ(lane)));
            v.setFilterVowel       (load(ParameterIDs::filterVowel(lane)));
            v.setBitcrushBitDepth  (load(ParameterIDs::bitcrushBitDepth(lane)));
            v.setBitcrushSampleRate(load(ParameterIDs::bitcrushSampleRateHz(lane)));
            v.setDistortionDrive   (load(ParameterIDs::distortionDrive(lane)));
            v.setModEffectType(static_cast<ModulationEffect::Type>(
                juce::jlimit(0,
                             static_cast<int>(ModulationEffect::Type::Phaser),
                             static_cast<int>(load(ParameterIDs::modEffectType(lane))))));
            v.setModEffectParam1   (load(ParameterIDs::modEffectParam1(lane)));
            v.setModEffectParam2   (load(ParameterIDs::modEffectParam2(lane)));
            v.setModEffectMix      (load(ParameterIDs::modEffectMix(lane)));
            v.setGain              (load(ParameterIDs::voiceGain(lane)));
        }

        void writeRealValue(juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& id,
                            float realValue)
        {
            if (auto* p = apvts.getParameter(id))
            {
                const float norm = p->getNormalisableRange().convertTo0to1(realValue);
                p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, norm));
            }
        }
    }

    float measureLanePeakOffline(juce::AudioProcessorValueTreeState& apvts,
                                 int lane,
                                 double sampleRate,
                                 double durationSeconds)
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return 0.0f;
        if (sampleRate <= 0.0 || durationSeconds <= 0.0)
            return 0.0f;

        Voice voice;
        voice.prepare(sampleRate);
        voice.reset();
        configureVoiceFromApvts(voice, apvts, lane);

        voice.trigger(static_cast<float>(durationSeconds), 0.0f, 1.0f);

        // Render the audition window plus a small tail so a slow attack
        // or short decay/release still gets sampled (kAuditionDurationSeconds
        // in the live processor is 0.5 s; the tail covers up to ~ amp_release
        // capped by the randomizer to 100 ms).
        const int sampleCount = static_cast<int>(
            std::ceil(sampleRate * (durationSeconds + 0.1)));

        float peak = 0.0f;
        for (int i = 0; i < sampleCount; ++i)
            peak = std::max(peak, std::fabs(voice.processSample()));
        return peak;
    }

    std::vector<juce::String> levelCriticalParamsForLane(int lane)
    {
        // Ordered roughly by how often each one is the silence culprit:
        // amp env + filter cutoff dominate; Mod FX mix can hide the dry;
        // the osc-mix params (FM depth, ring mix) can mute a carrier.
        return {
            ParameterIDs::ampAttack(lane),
            ParameterIDs::ampDecay(lane),
            ParameterIDs::ampSustain(lane),
            ParameterIDs::ampRelease(lane),
            ParameterIDs::filterCutoffHz(lane),
            ParameterIDs::filterResonanceQ(lane),
            ParameterIDs::modEffectMix(lane),
            ParameterIDs::fmDepth(lane),
            ParameterIDs::ringMix(lane),
        };
    }

    void forceLaneToAudibleFallback(juce::AudioProcessorValueTreeState& apvts,
                                    int lane)
    {
        // Known-audible safe values. These won't sound interesting — they
        // sound generic — but that's the right trade for the guaranteed
        // fallback: the rule is "never silent," not "always interesting."
        // To guarantee audibility we have to pin:
        //   * the carrier itself (waveform = Sine; FM depth + ring mix = 0
        //     so the carrier isn't muted or shifted out of band),
        //   * the amplitude envelope (so it has a level during the audition),
        //   * the filter (open enough to pass 440 Hz),
        //   * and the Mod FX wet/dry (fully dry so the wet path can't hide
        //     the dry signal — e.g. a long delay with mix=1 silences the
        //     audition window before the first echo returns).
        writeRealValue(apvts, ParameterIDs::oscWaveform(lane),
                       static_cast<float>(Oscillator::Waveform::Sine));
        writeRealValue(apvts, ParameterIDs::wavetableMorph(lane),   0.0f);
        writeRealValue(apvts, ParameterIDs::fmDepth(lane),          0.0f);
        writeRealValue(apvts, ParameterIDs::ringMix(lane),          0.0f);
        writeRealValue(apvts, ParameterIDs::ampAttack(lane),        0.005f);
        writeRealValue(apvts, ParameterIDs::ampDecay(lane),         0.10f);
        writeRealValue(apvts, ParameterIDs::ampSustain(lane),       0.70f);
        writeRealValue(apvts, ParameterIDs::ampRelease(lane),       0.10f);
        // Filter type matters as much as cutoff: a Highpass at 4 kHz
        // would block the 440 Hz audition entirely. Force Lowpass.
        writeRealValue(apvts, ParameterIDs::filterType(lane),
                       static_cast<float>(Filter::Type::Lowpass));
        writeRealValue(apvts, ParameterIDs::filterCutoffHz(lane),   4000.0f);
        writeRealValue(apvts, ParameterIDs::filterResonanceQ(lane), 1.0f);
        // Distortion drive < 1 attenuates linearly (tanh(drive * x) ~ drive * x
        // for small x); a randomized drive of 0.1 produces a 10x attenuation
        // on top of everything else. Pin to unity.
        writeRealValue(apvts, ParameterIDs::distortionDrive(lane),  1.0f);
        // Aggressive bitcrush can quantize a quiet beep to zero or fold it
        // into inaudible aliases. Pin to clean 16-bit / 48 kHz.
        writeRealValue(apvts, ParameterIDs::bitcrushBitDepth(lane),     16.0f);
        writeRealValue(apvts, ParameterIDs::bitcrushSampleRateHz(lane), 48000.0f);
        writeRealValue(apvts, ParameterIDs::modEffectMix(lane),     0.0f);
    }
}
