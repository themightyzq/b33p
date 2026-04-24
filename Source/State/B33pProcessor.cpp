#include "B33pProcessor.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "ParameterLayout.h"

namespace B33p
{
    namespace
    {
        constexpr float kAuditionDurationSeconds = 0.5f;
    }

    B33pProcessor::B33pProcessor()
        : juce::AudioProcessor(BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
        , apvts(*this, &undoManager, "B33pParameters", createParameterLayout())
        , randomizer(apvts)
    {
    }

    void B33pProcessor::prepareToPlay(double sampleRate, int /*blockSize*/)
    {
        currentSampleRate           = sampleRate;
        samplesUntilAuditionRelease = 0;
        voice.prepare(sampleRate);
        voice.reset();
    }

    void B33pProcessor::releaseResources()
    {
    }

    void B33pProcessor::setPitchCurve(std::vector<PitchEnvelopePoint> newCurve)
    {
        const juce::ScopedLock lock(pitchCurveLock);
        pitchCurve = std::move(newCurve);
    }

    void B33pProcessor::triggerAudition()
    {
        pendingAudition.store(true, std::memory_order_release);
    }

    void B33pProcessor::pushParametersToVoice()
    {
        const auto* wfParam = apvts.getRawParameterValue(ParameterIDs::oscWaveform);
        voice.setWaveform(static_cast<Oscillator::Waveform>(
            juce::jlimit(0, 4, static_cast<int>(wfParam->load()))));

        voice.setBasePitchHz         (apvts.getRawParameterValue(ParameterIDs::basePitchHz)->load());
        voice.setAmpAttack           (apvts.getRawParameterValue(ParameterIDs::ampAttack)->load());
        voice.setAmpDecay            (apvts.getRawParameterValue(ParameterIDs::ampDecay)->load());
        voice.setAmpSustain          (apvts.getRawParameterValue(ParameterIDs::ampSustain)->load());
        voice.setAmpRelease          (apvts.getRawParameterValue(ParameterIDs::ampRelease)->load());
        voice.setFilterCutoff        (apvts.getRawParameterValue(ParameterIDs::filterCutoffHz)->load());
        voice.setFilterResonance     (apvts.getRawParameterValue(ParameterIDs::filterResonanceQ)->load());
        voice.setBitcrushBitDepth    (apvts.getRawParameterValue(ParameterIDs::bitcrushBitDepth)->load());
        voice.setBitcrushSampleRate  (apvts.getRawParameterValue(ParameterIDs::bitcrushSampleRateHz)->load());
        voice.setDistortionDrive     (apvts.getRawParameterValue(ParameterIDs::distortionDrive)->load());
        voice.setGain                (apvts.getRawParameterValue(ParameterIDs::voiceGain)->load());
    }

    void B33pProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        pushParametersToVoice();

        if (pendingAudition.exchange(false, std::memory_order_acq_rel))
        {
            {
                const juce::ScopedTryLock tryLock(pitchCurveLock);
                if (tryLock.isLocked())
                    voice.setPitchCurve(pitchCurve);
            }
            voice.trigger(kAuditionDurationSeconds, 0.0f);
            samplesUntilAuditionRelease = static_cast<int>(
                kAuditionDurationSeconds * currentSampleRate);
        }

        // Decrement the auto-release counter at block granularity —
        // a few milliseconds of jitter is imperceptible for audition.
        if (samplesUntilAuditionRelease > 0)
        {
            samplesUntilAuditionRelease -= numSamples;
            if (samplesUntilAuditionRelease <= 0)
            {
                samplesUntilAuditionRelease = 0;
                voice.noteOff();
            }
        }

        auto* left  = numChannels > 0 ? buffer.getWritePointer(0) : nullptr;
        auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const float s = voice.processSample();
            if (left  != nullptr) left[i]  = s;
            if (right != nullptr) right[i] = s;
        }

        // Zero any additional output channels (defensive — stereo
        // standalones typically have at most 2 output channels).
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
}
