#include "B33pProcessor.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "ParameterLayout.h"
#include "ProjectState.h"

#include <cmath>

#if B33P_HAS_EDITOR
namespace B33p { juce::AudioProcessorEditor* createB33pEditor(B33pProcessor&); }
#endif

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
        for (auto& v : midiVoices)
        {
            v.prepare(sampleRate);
            v.reset();
        }
        for (auto& slot : midiNoteToVoice)
            slot = -1;
        nextMidiVoiceIndex = 0;
        for (auto& laneLfos : lfos)
            for (auto& lfo : laneLfos)
            {
                lfo.prepare(sampleRate);
                lfo.reset();
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

    bool B33pProcessor::hasEditor() const
    {
#if B33P_HAS_EDITOR
        return true;
#else
        return false;
#endif
    }

    juce::AudioProcessorEditor* B33pProcessor::createEditor()
    {
#if B33P_HAS_EDITOR
        return createB33pEditor(*this);
#else
        return nullptr;
#endif
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
        auto snapshot = std::make_shared<const PatternSnapshot>(makeSnapshot(pattern, snapshotRng));
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
        auto snap = std::make_shared<const PatternSnapshot>(makeSnapshot(pattern, snapshotRng));
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
        pattern.setBpm(Pattern::kDefaultBpm);
        pattern.setTimeSignature(Pattern::kDefaultTimeSigNum,
                                  Pattern::kDefaultTimeSigDen);
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

    std::array<float, kNumModDestinations>
    B33pProcessor::evaluateModulationContributions(int lane)
    {
        // Re-sync each LFO's runtime config from APVTS. Cheap to do
        // every block; the LFO's setShape / setRate are no-ops when
        // values haven't actually changed.
        for (int i = 0; i < kNumLfosPerLane; ++i)
        {
            auto& l = lfos[static_cast<size_t>(lane)][static_cast<size_t>(i)];
            const auto* shapeParam = apvts.getRawParameterValue(ParameterIDs::lfoShape (lane, i));
            const auto* rateParam  = apvts.getRawParameterValue(ParameterIDs::lfoRateHz(lane, i));
            l.setShape(static_cast<LFO::Shape>(
                juce::jlimit(0, 3, static_cast<int>(shapeParam->load()))));
            l.setRate(rateParam->load());
        }

        std::array<float, kNumModDestinations> contributions {};
        contributions.fill(0.0f);

        for (int slot = 0; slot < kNumModSlots; ++slot)
        {
            const int srcIdx = juce::jlimit(0, kNumModSources - 1,
                static_cast<int>(apvts.getRawParameterValue(ParameterIDs::modSlotSource(lane, slot))->load()));
            const int dstIdx = juce::jlimit(0, kNumModDestinations - 1,
                static_cast<int>(apvts.getRawParameterValue(ParameterIDs::modSlotDest(lane, slot))->load()));
            const float amount = apvts.getRawParameterValue(ParameterIDs::modSlotAmount(lane, slot))->load();

            const auto src = static_cast<ModSource>(srcIdx);
            if (src == ModSource::None || dstIdx == 0)
                continue;

            const int lfoIdx = (src == ModSource::LFO1) ? 0 : 1;
            const float lfoOut = lfos[static_cast<size_t>(lane)][static_cast<size_t>(lfoIdx)]
                                     .currentValue();

            // 0.5 scale puts a full-amplitude LFO with amount=1 at
            // ±half the parameter's normalised range — strong but
            // not destination-saturating. Users who want full sweep
            // can stack two slots on the same destination.
            contributions[static_cast<size_t>(dstIdx)] += lfoOut * amount * 0.5f;
        }

        return contributions;
    }

    namespace
    {
        // Maps each modulation destination back to the lane's
        // corresponding APVTS parameter ID. None returns an empty
        // string; the caller is expected to never call this with
        // ModDestination::None.
        juce::String paramIdForDest(int lane, ModDestination dest)
        {
            switch (dest)
            {
                case ModDestination::OscBasePitch:    return ParameterIDs::basePitchHz(lane);
                case ModDestination::WavetableMorph:  return ParameterIDs::wavetableMorph(lane);
                case ModDestination::FmDepth:         return ParameterIDs::fmDepth(lane);
                case ModDestination::RingMix:         return ParameterIDs::ringMix(lane);
                case ModDestination::FilterCutoff:    return ParameterIDs::filterCutoffHz(lane);
                case ModDestination::FilterResonance: return ParameterIDs::filterResonanceQ(lane);
                case ModDestination::DistortionDrive: return ParameterIDs::distortionDrive(lane);
                case ModDestination::ModEffectParam1: return ParameterIDs::modEffectParam1(lane);
                case ModDestination::ModEffectParam2: return ParameterIDs::modEffectParam2(lane);
                case ModDestination::ModEffectMix:    return ParameterIDs::modEffectMix(lane);
                case ModDestination::VoiceGain:       return ParameterIDs::voiceGain(lane);
                case ModDestination::None:            break;
            }
            return {};
        }
    }

    float B33pProcessor::effectiveParamValue(int lane,
                                              ModDestination destination,
                                              float modAmount) const
    {
        const auto id = paramIdForDest(lane, destination);
        if (id.isEmpty())
            return 0.0f;
        auto* param = apvts.getParameter(id);
        if (param == nullptr)
            return 0.0f;
        const float baseNorm = param->getValue();
        const float effNorm  = juce::jlimit(0.0f, 1.0f, baseNorm + modAmount);
        return param->convertFrom0to1(effNorm);
    }

    bool B33pProcessor::tryGetActiveOverride(int lane,
                                              ModDestination destination,
                                              float& outValue) const
    {
        if (lane < 0 || lane >= Pattern::kNumLanes
            || destination == ModDestination::None)
            return false;

        const auto& slots = activeOverrides[static_cast<size_t>(lane)];
        for (const auto& slot : slots)
        {
            if (slot.destination != destination)
                continue;
            const auto id = paramIdForDest(lane, destination);
            if (id.isEmpty())
                return false;
            auto* param = apvts.getParameter(id);
            if (param == nullptr)
                return false;
            outValue = param->convertFrom0to1(juce::jlimit(0.0f, 1.0f, slot.value));
            return true;
        }
        return false;
    }

    void B33pProcessor::pushParametersToLane(int lane)
    {
        pushParametersToVoiceImpl(lane,
                                   voices[static_cast<size_t>(lane)],
                                   /*applyOverrides=*/true);
    }

    void B33pProcessor::pushParametersToVoiceImpl(int lane,
                                                   Voice& v,
                                                   bool applyOverrides)
    {
        const auto modContribs = evaluateModulationContributions(lane);

        // Picks the effective value for a modulatable destination.
        // Priority: per-event override (wins outright if set, when
        // applyOverrides is true) > matrix-modulated value > raw
        // APVTS value. The override path skips matrix evaluation
        // entirely so a clip can pin a parameter independently of
        // any wiring. MIDI voices skip the override path because
        // they aren't event-driven.
        auto modulated = [&](ModDestination d, const juce::String& fallbackId)
        {
            if (applyOverrides)
            {
                float overrideValue = 0.0f;
                if (tryGetActiveOverride(lane, d, overrideValue))
                    return overrideValue;
            }
            const float c = modContribs[static_cast<size_t>(d)];
            if (c != 0.0f)
                return effectiveParamValue(lane, d, c);
            return apvts.getRawParameterValue(fallbackId)->load();
        };

        const auto* wfParam = apvts.getRawParameterValue(ParameterIDs::oscWaveform(lane));
        v.setWaveform(static_cast<Oscillator::Waveform>(
            juce::jlimit(0,
                         static_cast<int>(Oscillator::Waveform::Ring),
                         static_cast<int>(wfParam->load()))));

        v.setBasePitchHz         (modulated(ModDestination::OscBasePitch,    ParameterIDs::basePitchHz(lane)));
        v.setWavetableMorph      (modulated(ModDestination::WavetableMorph,  ParameterIDs::wavetableMorph(lane)));
        v.setFmRatio             (apvts.getRawParameterValue(ParameterIDs::fmRatio(lane))->load());
        v.setFmDepth             (modulated(ModDestination::FmDepth,         ParameterIDs::fmDepth(lane)));
        v.setRingRatio           (apvts.getRawParameterValue(ParameterIDs::ringRatio(lane))->load());
        v.setRingMix             (modulated(ModDestination::RingMix,         ParameterIDs::ringMix(lane)));

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
        const auto* ftParam = apvts.getRawParameterValue(ParameterIDs::filterType(lane));
        v.setFilterType(static_cast<Filter::Type>(
            juce::jlimit(0,
                         static_cast<int>(Filter::Type::Formant),
                         static_cast<int>(ftParam->load()))));
        v.setFilterCutoff        (modulated(ModDestination::FilterCutoff,    ParameterIDs::filterCutoffHz(lane)));
        v.setFilterResonance     (modulated(ModDestination::FilterResonance, ParameterIDs::filterResonanceQ(lane)));
        v.setFilterVowel         (apvts.getRawParameterValue(ParameterIDs::filterVowel(lane))->load());
        v.setBitcrushBitDepth    (apvts.getRawParameterValue(ParameterIDs::bitcrushBitDepth(lane))->load());
        v.setBitcrushSampleRate  (apvts.getRawParameterValue(ParameterIDs::bitcrushSampleRateHz(lane))->load());
        v.setDistortionDrive     (modulated(ModDestination::DistortionDrive, ParameterIDs::distortionDrive(lane)));

        const auto* mxParam = apvts.getRawParameterValue(ParameterIDs::modEffectType(lane));
        v.setModEffectType(static_cast<ModulationEffect::Type>(
            juce::jlimit(0,
                         static_cast<int>(ModulationEffect::Type::Phaser),
                         static_cast<int>(mxParam->load()))));
        v.setModEffectParam1     (modulated(ModDestination::ModEffectParam1, ParameterIDs::modEffectParam1(lane)));
        v.setModEffectParam2     (modulated(ModDestination::ModEffectParam2, ParameterIDs::modEffectParam2(lane)));
        v.setModEffectMix        (modulated(ModDestination::ModEffectMix,    ParameterIDs::modEffectMix(lane)));

        v.setGain                (modulated(ModDestination::VoiceGain,       ParameterIDs::voiceGain(lane)));
    }

    void B33pProcessor::triggerVoiceFromEvent(int lane, const Event& event)
    {
        if (lane < 0 || lane >= Pattern::kNumLanes)
            return;

        auto& v = voices[static_cast<size_t>(lane)];

        // Replace this lane's active override slots with the event's
        // overrides, then re-run pushParametersToLane so the new
        // overrides take effect on the very first sample of the
        // trigger. Without the re-push, overrides would only apply
        // starting at the next audio block (~10 ms latency).
        activeOverrides[static_cast<size_t>(lane)] = event.overrides;
        pushParametersToLane(lane);

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
                                     juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        pushParametersToVoices();

        // ---- MIDI input ------------------------------------------
        // Route every note-on to the next MIDI voice slot; route
        // note-off to whichever slot was assigned that note. The
        // MIDI voices live in their own pool so a held chord
        // doesn't fight pattern playback for voice slots. Each
        // MIDI voice inherits the currently-selected lane's params
        // at trigger time — sound-design happens on the selected
        // lane and MIDI plays the same patch.
        for (const auto meta : midi)
        {
            const auto& msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                const int lane = juce::jlimit(0, Pattern::kNumLanes - 1,
                                               selectedLane.load());
                const int note = juce::jlimit(0, 127, msg.getNoteNumber());

                // Round-robin allocation. If the slot is currently
                // ringing, the assignment retriggers it (Voice's
                // amp envelope handles the click-free retrigger
                // from current level per CLAUDE.md).
                const int slot = nextMidiVoiceIndex;
                nextMidiVoiceIndex = (nextMidiVoiceIndex + 1) % kMidiPolyphony;

                // If the stolen slot was already mapped to a held
                // note, that note's tracking is now stale — clear
                // the entry so a future note-off doesn't release
                // a voice the new note has claimed.
                for (auto& m : midiNoteToVoice)
                    if (m == slot) m = -1;
                midiNoteToVoice[static_cast<size_t>(note)] = slot;

                auto& v = midiVoices[static_cast<size_t>(slot)];
                pushParametersToVoiceImpl(lane, v, /*applyOverrides=*/false);
                {
                    const juce::ScopedTryLock tryLock(pitchCurveLock);
                    if (tryLock.isLocked())
                        v.setPitchCurve(pitchCurve);
                }
                v.trigger(/*durationSeconds=*/10.0f,   // long; release on note-off
                          static_cast<float>(note - 60),
                          msg.getVelocity() / 127.0f);
            }
            else if (msg.isNoteOff())
            {
                const int note = juce::jlimit(0, 127, msg.getNoteNumber());
                const int slot = midiNoteToVoice[static_cast<size_t>(note)];
                if (slot >= 0 && slot < kMidiPolyphony)
                {
                    midiVoices[static_cast<size_t>(slot)].noteOff();
                    midiNoteToVoice[static_cast<size_t>(note)] = -1;
                }
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& v : voices)     v.noteOff();
                for (auto& v : midiVoices) v.noteOff();
                for (auto& m : midiNoteToVoice) m = -1;
            }
        }
        midi.clear();   // we never produce MIDI

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

            // ---- Mix all four lane voices + the MIDI voice pool ---
            float s = 0.0f;
            for (auto& v : voices)     s += v.processSample();
            for (auto& v : midiVoices) s += v.processSample();

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

        // Advance every lane's LFOs by one block's worth of phase
        // so the next block's pushParametersToVoices reads the
        // updated phase position. LFOs run free regardless of
        // playback state — modulation persists between events.
        for (auto& laneLfos : lfos)
            for (auto& lfo : laneLfos)
                lfo.advance(numSamples);
    }
}
