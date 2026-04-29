#include "B33pProcessor.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "ParameterLayout.h"
#include "ProjectState.h"

#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float kAuditionDurationSeconds = 0.5f;
    }

    namespace
    {
        // Every APVTS parameter ID across all lanes — used for
        // dirty-tracking listener registration and for the New /
        // resetToDefaults reset loop. Re-derived each call so it
        // stays in sync with ParameterIDs::allForLane.
        std::vector<juce::String> allLaneParamIds()
        {
            std::vector<juce::String> ids;
            for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
                for (auto& id : ParameterIDs::allForLane(lane))
                    ids.push_back(id);
            return ids;
        }
    }

    B33pProcessor::B33pProcessor()
        : juce::AudioProcessor(BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
        , apvts(*this, &undoManager, "B33pParameters", createParameterLayout())
        , randomizer(apvts)
    {
        registerAsApvtsListener();
    }

    B33pProcessor::~B33pProcessor()
    {
        unregisterAsApvtsListener();
    }

    void B33pProcessor::registerAsApvtsListener()
    {
        for (const auto& id : allLaneParamIds())
            apvts.addParameterListener(id, this);
    }

    void B33pProcessor::unregisterAsApvtsListener()
    {
        for (const auto& id : allLaneParamIds())
            apvts.removeParameterListener(id, this);
    }

    void B33pProcessor::parameterChanged(const juce::String& parameterID, float newValue)
    {
        markDirty();

        // When a lane's waveform parameter flips to Custom or
        // Wavetable, seed any empty slot with one cycle of sine —
        // so audition / playback produces sound immediately instead
        // of silence until the user opens the editor and draws
        // something. Custom only needs slot 0; Wavetable benefits
        // from every slot being seeded so the morph parameter
        // produces sine-to-sine blends until the user differentiates
        // any individual slot.
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            if (parameterID != ParameterIDs::oscWaveform(lane))
                continue;

            const int newIndex = static_cast<int>(newValue);
            const bool isCustom    = newIndex == static_cast<int>(Oscillator::Waveform::Custom);
            const bool isWavetable = newIndex == static_cast<int>(Oscillator::Waveform::Wavetable);
            if (! isCustom && ! isWavetable)
                break;

            auto sineCycle = []
            {
                std::vector<float> sine(static_cast<size_t>(Oscillator::kCustomTableSize));
                for (size_t i = 0; i < sine.size(); ++i)
                    sine[i] = static_cast<float>(std::sin(
                        2.0 * 3.14159265358979323846
                        * static_cast<double>(i) / static_cast<double>(sine.size())));
                return sine;
            };

            const int seedSlots = isWavetable ? Oscillator::kNumWavetableSlots : 1;
            for (int slot = 0; slot < seedSlots; ++slot)
            {
                auto current = std::atomic_load(
                    &wavetableSlots[static_cast<size_t>(lane)][static_cast<size_t>(slot)]);
                if (current != nullptr && ! current->empty())
                    continue;   // slot already has a user-edited table
                setWavetableSlot(lane, slot, sineCycle());
            }
            break;
        }
    }

    void B33pProcessor::prepareToPlay(double sampleRate, int /*blockSize*/)
    {
        currentSampleRate = sampleRate;
        for (auto& n : samplesUntilNoteOff) n = 0;
        playheadSeconds.store(0.0);
        for (auto& v : voices)
        {
            v.prepare(sampleRate);
            v.reset();
        }
    }

    void B33pProcessor::releaseResources()
    {
    }

    void B33pProcessor::setPitchCurve(std::vector<PitchEnvelopePoint> newCurve)
    {
        {
            const juce::ScopedLock lock(pitchCurveLock);
            pitchCurve = std::move(newCurve);
        }
        markDirty();
    }

    std::vector<PitchEnvelopePoint> B33pProcessor::getPitchCurveCopy() const
    {
        const juce::ScopedLock lock(pitchCurveLock);
        return pitchCurve;
    }

    void B33pProcessor::setCustomWaveform(int lane, std::vector<float> samples)
    {
        // Custom mode reads slot 0 of the wavetable storage — a
        // single-table edit lands in the same slot the Wavetable
        // mode treats as "morph position 0".
        setWavetableSlot(lane, 0, std::move(samples));
    }

    std::vector<float> B33pProcessor::getCustomWaveformCopy(int lane) const
    {
        return getWavetableSlotCopy(lane, 0);
    }

    void B33pProcessor::setWavetableSlot(int lane, int slot,
                                          std::vector<float> samples)
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return;
        if (slot < 0 || slot >= Oscillator::kNumWavetableSlots)
            return;
        auto next = std::make_shared<const std::vector<float>>(std::move(samples));
        std::atomic_store(&wavetableSlots[static_cast<size_t>(lane)]
                                         [static_cast<size_t>(slot)],
                          next);
        markDirty();
    }

    std::vector<float> B33pProcessor::getWavetableSlotCopy(int lane, int slot) const
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return {};
        if (slot < 0 || slot >= Oscillator::kNumWavetableSlots)
            return {};
        auto current = std::atomic_load(&wavetableSlots[static_cast<size_t>(lane)]
                                                       [static_cast<size_t>(slot)]);
        return current ? *current : std::vector<float>{};
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

    void B33pProcessor::setPlayheadSeconds(double seconds)
    {
        const double len = pattern.getLengthSeconds();
        playheadSeconds.store(std::clamp(seconds, 0.0, std::max(0.0, len - 1e-6)));
    }

    void B33pProcessor::refreshPatternSnapshot()
    {
        auto snap = std::make_shared<const PatternSnapshot>(makeSnapshot(pattern));
        std::atomic_store(&snapshotSlot, snap);
    }

    void B33pProcessor::setLooping(bool shouldLoop)
    {
        if (looping.exchange(shouldLoop) != shouldLoop)
            markDirty();
    }

    void B33pProcessor::markDirty()
    {
        if (! dirty.exchange(true))
            notifyDirtyChanged();
    }

    void B33pProcessor::markClean()
    {
        if (dirty.exchange(false))
            notifyDirtyChanged();
    }

    void B33pProcessor::setOnDirtyChanged(OnDirtyChanged callback)
    {
        onDirtyChangedCallback = std::move(callback);
    }

    void B33pProcessor::setOnFullStateLoaded(OnFullStateLoaded callback)
    {
        onFullStateLoadedCallback = std::move(callback);
    }

    void B33pProcessor::setSelectedLane(int lane)
    {
        const int clamped = std::clamp(lane, 0, Pattern::kNumLanes - 1);
        if (selectedLane.exchange(clamped) == clamped)
            return;
        if (onSelectedLaneChangedCallback)
            onSelectedLaneChangedCallback(clamped);
    }

    void B33pProcessor::setOnSelectedLaneChanged(OnSelectedLaneChanged callback)
    {
        onSelectedLaneChangedCallback = std::move(callback);
    }

    juce::Colour B33pProcessor::laneAccentColour(int lane) const
    {
        // Four mid-saturation colours far apart on the wheel so the
        // lanes read distinctly even at low alpha.
        switch (lane)
        {
            case 0: return juce::Colour::fromRGB(120, 200, 255);  // blue
            case 1: return juce::Colour::fromRGB(120, 220, 140);  // green
            case 2: return juce::Colour::fromRGB(220, 180,  90);  // amber
            case 3: return juce::Colour::fromRGB(220, 130, 200);  // pink
            default: return juce::Colour::fromRGB(150, 150, 150);
        }
    }

    juce::String B33pProcessor::laneTitleSuffix(int lane) const
    {
        const auto& name = pattern.getLaneName(lane);
        juce::String s = " (Lane " + juce::String(lane + 1);
        if (name.isNotEmpty())
            s += ": " + name;
        s += ")";
        return s;
    }

    void B33pProcessor::copyLaneSettingsToAll(int sourceLane)
    {
        if (sourceLane < 0 || sourceLane >= Pattern::kNumLanes)
            return;

        const auto srcIds = ParameterIDs::allForLane(sourceLane);

        // Snapshot every wavetable slot up front so we don't repeat
        // the (lock + copy) work for each destination lane.
        std::array<std::vector<float>, Oscillator::kNumWavetableSlots> srcSlots;
        for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
            srcSlots[static_cast<size_t>(slot)] = getWavetableSlotCopy(sourceLane, slot);

        undoManager.beginNewTransaction("Copy lane to all lanes");

        for (int dest = 0; dest < Pattern::kNumLanes; ++dest)
        {
            if (dest == sourceLane) continue;
            const auto dstIds = ParameterIDs::allForLane(dest);
            for (size_t i = 0; i < srcIds.size(); ++i)
            {
                auto* src = apvts.getParameter(srcIds[i]);
                auto* dst = apvts.getParameter(dstIds[i]);
                if (src != nullptr && dst != nullptr)
                    dst->setValueNotifyingHost(src->getValue());
            }
            // Copy every wavetable slot too — otherwise the
            // destination lanes keep their old shapes and "Copy
            // voice to all" silently produces non-uniform timbre
            // for any lane that was on Custom or Wavetable.
            for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
                setWavetableSlot(dest, slot, srcSlots[static_cast<size_t>(slot)]);
        }
    }

    void B33pProcessor::resetLaneVoice(int lane)
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return;

        undoManager.beginNewTransaction("Reset lane voice");

        for (const auto& id : ParameterIDs::allForLane(lane))
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->getDefaultValue());

        // Drop every wavetable slot too — "Reset voice to defaults"
        // should put the lane back to a fresh state, not leave drawn
        // shapes lurking on the Custom / Wavetable modes.
        for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
            setWavetableSlot(lane, slot, {});
    }

    void B33pProcessor::notifyDirtyChanged()
    {
        if (onDirtyChangedCallback)
            juce::MessageManager::callAsync(onDirtyChangedCallback);
    }

    void B33pProcessor::notifyFullStateLoaded()
    {
        if (onFullStateLoadedCallback)
            juce::MessageManager::callAsync(onFullStateLoadedCallback);
    }

    void B33pProcessor::resetToDefaults()
    {
        // Reset every APVTS parameter to its registered default.
        // setValueNotifyingHost takes a normalised value (0..1)
        // which is exactly what getDefaultValue() returns.
        for (const auto& id : allLaneParamIds())
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->getDefaultValue());

        setPitchCurve({ { 0.0f, 0.0f }, { 1.0f, 0.0f } });

        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
            for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
                setWavetableSlot(lane, slot, {});

        pattern.clearAll();
        pattern.resetAllLaneMeta();
        pattern.setLengthSeconds(Pattern::kDefaultLengthSeconds);
        looping.store(true);

        randomizer.clearAllLocks();

        // History from the previous project no longer maps onto the
        // fresh state — clear it so cmd+z can't reach into ghosts.
        undoManager.clearUndoHistory();

        // The bulk mutations marked us dirty along the way; the
        // whole point of "new" is "nothing worth saving yet".
        markClean();
        notifyFullStateLoaded();
    }

    void B33pProcessor::pushParametersToVoices()
    {
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
            pushParametersToLane(lane);
    }

    void B33pProcessor::pushParametersToLane(int lane)
    {
        auto& v = voices[static_cast<size_t>(lane)];

        const auto* wfParam = apvts.getRawParameterValue(ParameterIDs::oscWaveform(lane));
        v.setWaveform(static_cast<Oscillator::Waveform>(
            juce::jlimit(0,
                         static_cast<int>(Oscillator::Waveform::Ring),
                         static_cast<int>(wfParam->load()))));

        v.setBasePitchHz         (apvts.getRawParameterValue(ParameterIDs::basePitchHz(lane))->load());
        v.setWavetableMorph      (apvts.getRawParameterValue(ParameterIDs::wavetableMorph(lane))->load());
        v.setFmRatio             (apvts.getRawParameterValue(ParameterIDs::fmRatio(lane))->load());
        v.setFmDepth             (apvts.getRawParameterValue(ParameterIDs::fmDepth(lane))->load());
        v.setRingRatio           (apvts.getRawParameterValue(ParameterIDs::ringRatio(lane))->load());
        v.setRingMix             (apvts.getRawParameterValue(ParameterIDs::ringMix(lane))->load());

        // Push every slot only when its shared_ptr has changed since
        // last push — keeps the common no-edit path zero-cost (one
        // atomic_load + pointer compare per slot).
        for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
        {
            auto current = std::atomic_load(
                &wavetableSlots[static_cast<size_t>(lane)][static_cast<size_t>(slot)]);
            auto& last = lastPushedWavetableSlots[static_cast<size_t>(lane)]
                                                 [static_cast<size_t>(slot)];
            if (current == last)
                continue;
            if (current != nullptr)
                v.setWavetableSlot(slot, *current);
            else
                v.setWavetableSlot(slot, {});
            last = current;
        }
        v.setAmpAttack           (apvts.getRawParameterValue(ParameterIDs::ampAttack(lane))->load());
        v.setAmpDecay            (apvts.getRawParameterValue(ParameterIDs::ampDecay(lane))->load());
        v.setAmpSustain          (apvts.getRawParameterValue(ParameterIDs::ampSustain(lane))->load());
        v.setAmpRelease          (apvts.getRawParameterValue(ParameterIDs::ampRelease(lane))->load());
        v.setFilterCutoff        (apvts.getRawParameterValue(ParameterIDs::filterCutoffHz(lane))->load());
        v.setFilterResonance     (apvts.getRawParameterValue(ParameterIDs::filterResonanceQ(lane))->load());
        v.setBitcrushBitDepth    (apvts.getRawParameterValue(ParameterIDs::bitcrushBitDepth(lane))->load());
        v.setBitcrushSampleRate  (apvts.getRawParameterValue(ParameterIDs::bitcrushSampleRateHz(lane))->load());
        v.setDistortionDrive     (apvts.getRawParameterValue(ParameterIDs::distortionDrive(lane))->load());
        v.setGain                (apvts.getRawParameterValue(ParameterIDs::voiceGain(lane))->load());
    }

    void B33pProcessor::triggerVoiceFromEvent(int lane, const Event& event)
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return;

        auto& v = voices[static_cast<size_t>(lane)];

        // Try to refresh the voice's pitch curve before triggering;
        // if the lock is contended we keep whatever curve the voice
        // already has. The pitch curve is shared across all lanes for
        // this MVP — per-lane curves are post-MVP polish.
        {
            const juce::ScopedTryLock tryLock(pitchCurveLock);
            if (tryLock.isLocked())
                v.setPitchCurve(pitchCurve);
        }

        v.trigger(static_cast<float>(event.durationSeconds),
                   event.pitchOffsetSemitones,
                   event.velocity);
        samplesUntilNoteOff[static_cast<size_t>(lane)]
            = static_cast<int>(event.durationSeconds * currentSampleRate);
    }

    void B33pProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        pushParametersToVoices();

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
            for (auto& v : voices)              v.noteOff();
            for (auto& n : samplesUntilNoteOff) n = 0;
            activeSnapshot.reset();
        }
        audioThreadPlaying = nowPlaying;

        // ---- Live snapshot swap while playing ---------------------
        // The message-thread timer republishes the snapshot whenever
        // the pattern is mutated. Compare pointers; if the slot
        // changed, take the new snapshot and re-seed nextEventIndex
        // from the current playhead so we don't re-fire events that
        // have already played in this loop iteration.
        if (audioThreadPlaying)
        {
            auto latest = std::atomic_load(&snapshotSlot);
            if (latest != activeSnapshot)
            {
                activeSnapshot = latest;
                nextEventIndex = 0;
                if (activeSnapshot != nullptr)
                {
                    const double now = playheadSeconds.load();
                    const auto& evs  = activeSnapshot->events;
                    while (nextEventIndex < static_cast<int>(evs.size())
                           && evs[static_cast<size_t>(nextEventIndex)].event.startSeconds < now)
                        ++nextEventIndex;
                }
            }
        }

        // ---- Audition trigger fires before the per-sample loop ----
        // Routes to whichever lane the editor sections currently
        // target so the user hears the voice they're tweaking.
        if (pendingAudition.exchange(false, std::memory_order_acq_rel))
        {
            Event auditionEvent { 0.0,
                                  static_cast<double>(kAuditionDurationSeconds),
                                  0.0f };
            triggerVoiceFromEvent(selectedLane.load(), auditionEvent);
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
                        for (auto& v : voices)              v.noteOff();
                        for (auto& n : samplesUntilNoteOff) n = 0;
                        playing.store(false, std::memory_order_release);
                        audioThreadPlaying  = false;
                        activeSnapshot.reset();
                    }
                }

                if (audioThreadPlaying && activeSnapshot != nullptr)
                {
                    const auto& events = activeSnapshot->events;
                    while (nextEventIndex < static_cast<int>(events.size())
                           && events[static_cast<size_t>(nextEventIndex)].event.startSeconds <= playhead)
                    {
                        const auto& sched = events[static_cast<size_t>(nextEventIndex)];
                        triggerVoiceFromEvent(sched.lane, sched.event);
                        ++nextEventIndex;
                    }
                }
            }

            // ---- Per-lane sample-accurate noteOff -----------------
            for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
            {
                int& count = samplesUntilNoteOff[static_cast<size_t>(lane)];
                if (count > 0 && --count == 0)
                    voices[static_cast<size_t>(lane)].noteOff();
            }

            // ---- Mix all four voices -----------------------------
            float s = 0.0f;
            for (auto& v : voices)
                s += v.processSample();

            if (left  != nullptr) left[i]  = s;
            if (right != nullptr) right[i] = s;

            playhead += sampleDuration;
        }

        if (audioThreadPlaying)
            playheadSeconds.store(playhead);

        // Output peak for the level meter — scan the L channel,
        // decay the previous value slightly so the meter falls when
        // the buffer goes quiet. The factor (~0.95 per block) gives
        // a roughly 200 ms fall time at typical block sizes.
        {
            float blockPeak = 0.0f;
            if (left != nullptr)
                for (int i = 0; i < numSamples; ++i)
                    blockPeak = std::max(blockPeak, std::fabs(left[i]));
            const float prev = outputPeak.load() * 0.95f;
            outputPeak.store(std::max(blockPeak, prev));
        }

        // Zero any additional output channels (defensive — stereo
        // standalones typically have at most 2 output channels).
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
}
