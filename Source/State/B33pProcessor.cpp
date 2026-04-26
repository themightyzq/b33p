#include "B33pProcessor.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "ParameterLayout.h"
#include "ProjectState.h"

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
        currentSampleRate    = sampleRate;
        samplesUntilNoteOff  = 0;
        playheadSeconds.store(0.0);
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

    std::vector<PitchEnvelopePoint> B33pProcessor::getPitchCurveCopy() const
    {
        const juce::ScopedLock lock(pitchCurveLock);
        return pitchCurve;
    }

    void B33pProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        const auto xml = ProjectState::toXmlString(ProjectState::save(*this));
        destData.replaceAll(xml.toRawUTF8(), xml.getNumBytesAsUTF8());
    }

    void B33pProcessor::setStateInformation(const void* data, int sizeInBytes)
    {
        if (data == nullptr || sizeInBytes <= 0)
            return;

        const juce::String xml { static_cast<const char*>(data),
                                  static_cast<size_t>(sizeInBytes) };
        const auto tree = ProjectState::fromXmlString(xml);
        if (tree.isValid())
            ProjectState::load(*this, tree);
    }

    void B33pProcessor::triggerAudition()
    {
        pendingAudition.store(true, std::memory_order_release);
    }

    void B33pProcessor::startPlayback()
    {
        // Build the snapshot, swap it into the audio-thread slot,
        // reset the playhead, then signal "playing". The release of
        // playing happens-before the audio thread's acquire; the
        // shared_ptr atomic_store provides its own ordering for
        // pointer reads.
        auto snapshot = std::make_shared<const PatternSnapshot>(makeSnapshot(pattern));
        std::atomic_store(&snapshotSlot, snapshot);
        playheadSeconds.store(0.0);
        playing.store(true, std::memory_order_release);
    }

    void B33pProcessor::stopPlayback()
    {
        playing.store(false, std::memory_order_release);
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

    void B33pProcessor::triggerVoiceFromEvent(const Event& event)
    {
        // Try to refresh the voice's pitch curve before triggering;
        // if the lock is contended we keep whatever curve was last
        // pushed in. Acceptable because the next free trigger picks
        // it up.
        {
            const juce::ScopedTryLock tryLock(pitchCurveLock);
            if (tryLock.isLocked())
                voice.setPitchCurve(pitchCurve);
        }

        voice.trigger(static_cast<float>(event.durationSeconds),
                      event.pitchOffsetSemitones);
        samplesUntilNoteOff = static_cast<int>(event.durationSeconds * currentSampleRate);
    }

    void B33pProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        pushParametersToVoice();

        // ---- Detect playback start / stop transitions -------------
        const bool nowPlaying = playing.load(std::memory_order_acquire);
        if (nowPlaying && ! audioThreadPlaying)
        {
            activeSnapshot      = std::atomic_load(&snapshotSlot);
            nextEventIndex      = 0;
            playheadSeconds.store(0.0);
        }
        else if (! nowPlaying && audioThreadPlaying)
        {
            voice.noteOff();
            samplesUntilNoteOff = 0;
            activeSnapshot.reset();
        }
        audioThreadPlaying = nowPlaying;

        // ---- Audition trigger fires before the per-sample loop ----
        if (pendingAudition.exchange(false, std::memory_order_acq_rel))
        {
            Event auditionEvent { 0.0,
                                  static_cast<double>(kAuditionDurationSeconds),
                                  0.0f };
            triggerVoiceFromEvent(auditionEvent);
        }

        auto* left  = numChannels > 0 ? buffer.getWritePointer(0) : nullptr;
        auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

        const double sampleDuration = currentSampleRate > 0.0
                                          ? 1.0 / currentSampleRate
                                          : 0.0;
        double playhead = playheadSeconds.load();

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- Pattern playback: advance playhead, fire events --
            if (audioThreadPlaying && activeSnapshot != nullptr)
            {
                if (playhead >= activeSnapshot->lengthSeconds)
                {
                    if (looping.load())
                    {
                        playhead       -= activeSnapshot->lengthSeconds;
                        nextEventIndex  = 0;
                    }
                    else
                    {
                        voice.noteOff();
                        playing.store(false, std::memory_order_release);
                        audioThreadPlaying  = false;
                        samplesUntilNoteOff = 0;
                        activeSnapshot.reset();
                    }
                }

                if (audioThreadPlaying && activeSnapshot != nullptr)
                {
                    const auto& events = activeSnapshot->events;
                    while (nextEventIndex < static_cast<int>(events.size())
                           && events[static_cast<size_t>(nextEventIndex)].startSeconds <= playhead)
                    {
                        triggerVoiceFromEvent(events[static_cast<size_t>(nextEventIndex)]);
                        ++nextEventIndex;
                    }
                }
            }

            // ---- Sample-accurate noteOff for the active note ------
            if (samplesUntilNoteOff > 0)
            {
                if (--samplesUntilNoteOff == 0)
                    voice.noteOff();
            }

            const float s = voice.processSample();
            if (left  != nullptr) left[i]  = s;
            if (right != nullptr) right[i] = s;

            playhead += sampleDuration;
        }

        if (audioThreadPlaying)
            playheadSeconds.store(playhead);

        // Zero any additional output channels (defensive — stereo
        // standalones typically have at most 2 output channels).
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
}
