#include "ProjectState.h"

#include "B33pProcessor.h"
#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "Pattern/Pattern.h"

#include <vector>

namespace B33p::ProjectState
{
    namespace
    {
        // ValueTree identifiers — kept here so renames stay in one
        // place and any change is loudly visible at review time.
        const juce::Identifier kRoot                  { "B33P" };
        const juce::Identifier kVersion               { "version" };

        const juce::Identifier kParameters            { "PARAMETERS" };

        const juce::Identifier kPitchCurve            { "PITCH_CURVE" };
        const juce::Identifier kPoint                 { "POINT" };
        const juce::Identifier kPointTime             { "t" };
        const juce::Identifier kPointSemitones        { "st" };

        const juce::Identifier kPattern               { "PATTERN" };
        const juce::Identifier kPatternLength         { "length_seconds" };
        const juce::Identifier kPatternLooping        { "looping" };
        const juce::Identifier kLane                  { "LANE" };
        const juce::Identifier kLaneIndex             { "index" };
        const juce::Identifier kLaneName              { "name" };
        const juce::Identifier kLaneMuted             { "muted" };
        const juce::Identifier kLaneSoloed            { "soloed" };
        const juce::Identifier kLaneCustomWaveform    { "custom_waveform" };
        const juce::Identifier kWavetable             { "WAVETABLE" };
        const juce::Identifier kSlot                  { "SLOT" };
        const juce::Identifier kSlotIndex             { "index" };
        const juce::Identifier kSlotSamples           { "samples" };
        const juce::Identifier kEvent                 { "EVENT" };
        const juce::Identifier kEventStart            { "start_seconds" };
        const juce::Identifier kEventDuration         { "duration_seconds" };
        const juce::Identifier kEventPitch            { "pitch_offset_semitones" };
        const juce::Identifier kEventVelocity         { "velocity" };
        const juce::Identifier kEventProbability      { "probability" };
        const juce::Identifier kEventRatchets         { "ratchets" };
        const juce::Identifier kEventHumanize         { "humanize" };
        const juce::Identifier kEventOverride         { "OVERRIDE" };
        const juce::Identifier kOverrideDest          { "dest" };
        const juce::Identifier kOverrideValue         { "value" };

        const juce::Identifier kLocks                 { "LOCKS" };
        const juce::Identifier kLock                  { "LOCK" };
        const juce::Identifier kLockId                { "id" };
    }

    juce::ValueTree save(B33pProcessor& processor)
    {
        juce::ValueTree root { kRoot };
        root.setProperty(kVersion, kCurrentVersion, nullptr);

        // ---- Parameters -------------------------------------------
        // APVTS state has its own type ("B33pParameters") so we wrap
        // it inside a PARAMETERS container — the wrapper means the
        // outer schema doesn't need to know that internal name.
        juce::ValueTree paramsNode { kParameters };
        paramsNode.appendChild(processor.getApvts().copyState(), nullptr);
        root.appendChild(paramsNode, nullptr);

        // ---- Pitch curve ------------------------------------------
        juce::ValueTree curveNode { kPitchCurve };
        for (const auto& p : processor.getPitchCurveCopy())
        {
            juce::ValueTree pt { kPoint };
            pt.setProperty(kPointTime,      static_cast<double>(p.normalizedTime), nullptr);
            pt.setProperty(kPointSemitones, static_cast<double>(p.semitones),       nullptr);
            curveNode.appendChild(pt, nullptr);
        }
        root.appendChild(curveNode, nullptr);

        // ---- Pattern ----------------------------------------------
        juce::ValueTree patternNode { kPattern };
        const auto& pattern = processor.getPattern();
        patternNode.setProperty(kPatternLength,  pattern.getLengthSeconds(), nullptr);
        patternNode.setProperty(kPatternLooping, processor.getLooping(),     nullptr);

        for (int laneIdx = 0; laneIdx < Pattern::kNumLanes; ++laneIdx)
        {
            juce::ValueTree laneNode { kLane };
            laneNode.setProperty(kLaneIndex,  laneIdx,                          nullptr);
            laneNode.setProperty(kLaneName,   pattern.getLaneName(laneIdx),      nullptr);
            laneNode.setProperty(kLaneMuted,  pattern.isLaneMuted(laneIdx),      nullptr);
            laneNode.setProperty(kLaneSoloed, pattern.isLaneSoloed(laneIdx),     nullptr);

            // Wavetable storage — each non-empty slot gets its own
            // SLOT child holding a comma-separated CSV of samples.
            // Custom mode reads slot 0 (so a Custom-only lane saves
            // exactly one SLOT child); Wavetable mode reads all
            // four. Slots that have never been edited are simply
            // omitted to keep the file compact.
            juce::ValueTree wavetableNode { kWavetable };
            for (int slot = 0; slot < Oscillator::kNumWavetableSlots; ++slot)
            {
                const auto table = processor.getWavetableSlotCopy(laneIdx, slot);
                if (table.empty())
                    continue;
                juce::String csv;
                csv.preallocateBytes(table.size() * 8);
                for (size_t i = 0; i < table.size(); ++i)
                {
                    if (i > 0) csv += ",";
                    csv += juce::String(table[i], 4);
                }
                juce::ValueTree slotNode { kSlot };
                slotNode.setProperty(kSlotIndex,   slot, nullptr);
                slotNode.setProperty(kSlotSamples, csv,  nullptr);
                wavetableNode.appendChild(slotNode, nullptr);
            }
            if (wavetableNode.getNumChildren() > 0)
                laneNode.appendChild(wavetableNode, nullptr);

            for (const auto& e : pattern.getEvents(laneIdx))
            {
                juce::ValueTree eventNode { kEvent };
                eventNode.setProperty(kEventStart,       e.startSeconds,                              nullptr);
                eventNode.setProperty(kEventDuration,    e.durationSeconds,                           nullptr);
                eventNode.setProperty(kEventPitch,       static_cast<double>(e.pitchOffsetSemitones), nullptr);
                eventNode.setProperty(kEventVelocity,    static_cast<double>(e.velocity),             nullptr);
                eventNode.setProperty(kEventProbability, static_cast<double>(e.probability),          nullptr);
                eventNode.setProperty(kEventRatchets,    e.ratchets,                                  nullptr);
                eventNode.setProperty(kEventHumanize,    static_cast<double>(e.humanizeAmount),       nullptr);

                // Per-event overrides — only emit slots whose
                // destination is non-None, so events without
                // overrides serialise as before.
                for (const auto& o : e.overrides)
                {
                    if (o.destination == ModDestination::None)
                        continue;
                    juce::ValueTree overrideNode { kEventOverride };
                    overrideNode.setProperty(kOverrideDest,
                                              static_cast<int>(o.destination), nullptr);
                    overrideNode.setProperty(kOverrideValue,
                                              static_cast<double>(o.value),    nullptr);
                    eventNode.appendChild(overrideNode, nullptr);
                }

                laneNode.appendChild(eventNode, nullptr);
            }

            patternNode.appendChild(laneNode, nullptr);
        }
        root.appendChild(patternNode, nullptr);

        // ---- Locks -------------------------------------------------
        juce::ValueTree locksNode { kLocks };
        for (const auto& id : processor.getRandomizer().getLockedParameterIDs())
        {
            juce::ValueTree lockNode { kLock };
            lockNode.setProperty(kLockId, id, nullptr);
            locksNode.appendChild(lockNode, nullptr);
        }
        root.appendChild(locksNode, nullptr);

        return root;
    }

    namespace
    {
        // v1 → v2: parameter IDs grew a per-lane prefix when the
        // single voice became four voices. The single set of v1
        // parameters is fanned out to every lane so a v1 file
        // sounds identical when reloaded under v2 (every lane
        // plays the same timbre until the user starts editing
        // individual lanes).
        struct ParamRename { const char* v1; juce::String (*v2)(int); };
        constexpr ParamRename kParamRenames[] = {
            { ParameterIDs::v1::oscWaveform,           ParameterIDs::oscWaveform           },
            { ParameterIDs::v1::basePitchHz,           ParameterIDs::basePitchHz           },
            { ParameterIDs::v1::ampAttack,             ParameterIDs::ampAttack             },
            { ParameterIDs::v1::ampDecay,              ParameterIDs::ampDecay              },
            { ParameterIDs::v1::ampSustain,            ParameterIDs::ampSustain            },
            { ParameterIDs::v1::ampRelease,            ParameterIDs::ampRelease            },
            { ParameterIDs::v1::filterCutoffHz,        ParameterIDs::filterCutoffHz        },
            { ParameterIDs::v1::filterResonanceQ,      ParameterIDs::filterResonanceQ      },
            { ParameterIDs::v1::bitcrushBitDepth,      ParameterIDs::bitcrushBitDepth      },
            { ParameterIDs::v1::bitcrushSampleRateHz,  ParameterIDs::bitcrushSampleRateHz  },
            { ParameterIDs::v1::distortionDrive,       ParameterIDs::distortionDrive       },
            { ParameterIDs::v1::voiceGain,             ParameterIDs::voiceGain             },
        };

        // Rewrites the embedded APVTS subtree: each <PARAM id="X"/>
        // entry with a v1 ID becomes four <PARAM id="laneN_X"/>
        // entries with the same value.
        void migrateApvtsV1ToV2(juce::ValueTree apvtsState)
        {
            // Snapshot the v1 PARAM children first so we can wipe
            // them and re-add the lane-prefixed equivalents without
            // iterator invalidation.
            struct Existing { juce::String id; juce::var value; };
            std::vector<Existing> existing;
            for (auto child : apvtsState)
                if (child.hasType("PARAM"))
                    existing.push_back({ child.getProperty("id").toString(),
                                          child.getProperty("value") });

            // Drop every v1 PARAM child.
            for (int i = apvtsState.getNumChildren() - 1; i >= 0; --i)
            {
                auto child = apvtsState.getChild(i);
                if (child.hasType("PARAM"))
                    apvtsState.removeChild(child, nullptr);
            }

            // For each known v1 ID, fan its value out to all four lanes.
            for (const auto& e : existing)
            {
                for (const auto& rename : kParamRenames)
                {
                    if (e.id != rename.v1)
                        continue;
                    for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
                    {
                        juce::ValueTree p { "PARAM" };
                        p.setProperty("id",    rename.v2(lane), nullptr);
                        p.setProperty("value", e.value,         nullptr);
                        apvtsState.appendChild(p, nullptr);
                    }
                    break;
                }
            }
        }
    }

    namespace
    {
        // v2 → v3: lift each LANE's `custom_waveform` attribute (a
        // CSV of float samples) into a WAVETABLE child holding a
        // single SLOT[index=0] entry. The attribute is dropped from
        // the LANE node so that v3 readers find the new structure
        // canonically. Empty / missing attributes are no-ops; the
        // pattern, parameters, locks, and pitch curve round-trip
        // unchanged.
        void migratePatternV2ToV3(juce::ValueTree patternNode)
        {
            for (auto laneNode : patternNode)
            {
                if (! laneNode.hasType(kLane))
                    continue;
                const auto csv = laneNode.getProperty(kLaneCustomWaveform,
                                                      juce::String{}).toString();
                if (csv.isEmpty())
                    continue;

                juce::ValueTree wavetableNode { kWavetable };
                juce::ValueTree slotNode      { kSlot };
                slotNode.setProperty(kSlotIndex,   0,   nullptr);
                slotNode.setProperty(kSlotSamples, csv, nullptr);
                wavetableNode.appendChild(slotNode, nullptr);
                laneNode.appendChild(wavetableNode, nullptr);
                laneNode.removeProperty(kLaneCustomWaveform, nullptr);
            }
        }
    }

    juce::ValueTree migrate(juce::ValueTree tree)
    {
        int version = tree.getProperty(kVersion, 0);

        if (version == 1)
        {
            // v1 → v2: fan the single voice's APVTS parameters out
            // to all four lanes. Pitch curve, pattern, and locks
            // round-trip unchanged.
            const auto paramsNode = tree.getChildWithName(kParameters);
            if (paramsNode.getNumChildren() > 0)
                migrateApvtsV1ToV2(paramsNode.getChild(0));

            tree.setProperty(kVersion, 2, nullptr);
            version = 2;
        }

        if (version == 2)
        {
            // v2 → v3: existing per-lane custom_waveform attributes
            // become WAVETABLE/SLOT[index=0] children. The new
            // wavetable_morph parameter wasn't in v2 — APVTS will
            // fall back to its registered default (0.0) for any ID
            // missing from the loaded state.
            const auto patternNode = tree.getChildWithName(kPattern);
            if (patternNode.isValid())
                migratePatternV2ToV3(patternNode);

            tree.setProperty(kVersion, 3, nullptr);
            version = 3;
        }

        if (version == 3)
        {
            // v3 → v4: fm_ratio + fm_depth parameters were added.
            // No data migration is needed — APVTS::replaceState
            // leaves any parameter that isn't present in the loaded
            // tree at its registered default (1.0 ratio, 0.0 depth),
            // which produces the same audible result a v3 file had
            // (no FM mode existed, so silent FM params don't change
            // anything for non-FM lanes).
            tree.setProperty(kVersion, 4, nullptr);
            version = 4;
        }

        if (version == 4)
        {
            // v4 → v5: ring_ratio + ring_mix parameters were added.
            // Same default-fallback story as v4 — a v4 file can't
            // have selected the new Ring waveform anyway, so the
            // missing-but-defaulted ring params are inert on load.
            tree.setProperty(kVersion, 5, nullptr);
            version = 5;
        }

        if (version == 5)
        {
            // v5 → v6: filter_type (Choice) + filter_vowel (Float)
            // were added. Same default-fallback story — the new
            // filter_type defaults to Lowpass (= 0), which matches
            // the only filter shape v5 binaries could produce, so
            // a v5 file sounds identical when reloaded.
            tree.setProperty(kVersion, 6, nullptr);
            version = 6;
        }

        if (version == 6)
        {
            // v6 → v7: mod_effect_type (Choice) + mod_effect_p1 /
            // _p2 / _mix (Floats) were added. mod_effect_type
            // defaults to None (= 0, bypass), so the wet effect
            // is inert on a v6 file load and the audio chain ends
            // exactly where it did under v6.
            tree.setProperty(kVersion, 7, nullptr);
            version = 7;
        }

        if (version == 7)
        {
            // v7 → v8: LFO + modulation matrix params were added.
            // Every matrix slot defaults to source = dest = None
            // and the LFO rates default to 1 Hz / Sine — none of
            // which contribute modulation, so a v7 file's audio
            // is unchanged when reloaded.
            tree.setProperty(kVersion, 8, nullptr);
            version = 8;
        }

        if (version == 8)
        {
            // v8 → v9: per-event overrides. v8 events have no
            // OVERRIDE children, which the loader treats as "every
            // override slot stays at destination = None" (inert) —
            // so a v8 file is audibly unchanged on reload.
            tree.setProperty(kVersion, 9, nullptr);
            version = 9;
        }

        if (version == 9)
        {
            // v9 → v10: per-event probability / ratchets / humanize.
            // Missing attributes default to (1.0, 1, 0.0) — every
            // event fires every loop, no ratcheting, no jitter —
            // which is exactly the v9 trigger semantics.
            tree.setProperty(kVersion, 10, nullptr);
            version = 10;
        }

        return tree;
    }

    bool load(B33pProcessor& processor, juce::ValueTree tree)
    {
        if (! tree.hasType(kRoot))
            return false;

        const int version = tree.getProperty(kVersion, 0);
        if (version <= 0 || version > kCurrentVersion)
            return false;

        tree = migrate(std::move(tree));

        // ---- Parameters -------------------------------------------
        const auto paramsNode = tree.getChildWithName(kParameters);
        if (paramsNode.getNumChildren() > 0)
            processor.getApvts().replaceState(paramsNode.getChild(0));

        // ---- Pitch curve ------------------------------------------
        const auto curveNode = tree.getChildWithName(kPitchCurve);
        std::vector<PitchEnvelopePoint> curve;
        for (auto pt : curveNode)
        {
            if (! pt.hasType(kPoint))
                continue;
            curve.push_back({
                static_cast<float>(static_cast<double>(pt.getProperty(kPointTime,      0.0))),
                static_cast<float>(static_cast<double>(pt.getProperty(kPointSemitones, 0.0)))
            });
        }
        processor.setPitchCurve(std::move(curve));

        // ---- Pattern ----------------------------------------------
        const auto patternNode = tree.getChildWithName(kPattern);
        auto& pattern = processor.getPattern();
        pattern.clearAll();
        pattern.resetAllLaneMeta();
        pattern.setLengthSeconds(
            static_cast<double>(patternNode.getProperty(kPatternLength,
                                                        Pattern::kDefaultLengthSeconds)));
        processor.setLooping(static_cast<bool>(patternNode.getProperty(kPatternLooping, true)));

        for (auto laneNode : patternNode)
        {
            if (! laneNode.hasType(kLane))
                continue;
            const int laneIdx = laneNode.getProperty(kLaneIndex, -1);
            if (laneIdx < 0 || laneIdx >= Pattern::kNumLanes)
                continue;

            // Default-tolerant: older files may lack any of these
            // attributes, so empty name / unmuted / unsoloed are the
            // right defaults.
            pattern.setLaneName  (laneIdx, laneNode.getProperty(kLaneName,  juce::String{}).toString());
            pattern.setLaneMuted (laneIdx, static_cast<bool>(laneNode.getProperty(kLaneMuted,  false)));
            pattern.setLaneSoloed(laneIdx, static_cast<bool>(laneNode.getProperty(kLaneSoloed, false)));

            // Wavetable storage. Each SLOT child carries an index
            // (0..N-1) and a CSV of float samples; missing slots
            // stay empty (= silent for that morph position).
            const auto wavetableNode = laneNode.getChildWithName(kWavetable);
            for (auto slotNode : wavetableNode)
            {
                if (! slotNode.hasType(kSlot))
                    continue;
                const int slotIdx = slotNode.getProperty(kSlotIndex, -1);
                if (slotIdx < 0 || slotIdx >= Oscillator::kNumWavetableSlots)
                    continue;
                const auto csv = slotNode.getProperty(kSlotSamples,
                                                       juce::String{}).toString();
                if (csv.isEmpty())
                    continue;
                juce::StringArray parts;
                parts.addTokens(csv, ",", "");
                std::vector<float> table;
                table.reserve(static_cast<size_t>(parts.size()));
                for (const auto& p : parts)
                    table.push_back(p.getFloatValue());
                processor.setWavetableSlot(laneIdx, slotIdx, std::move(table));
            }

            for (auto eventNode : laneNode)
            {
                if (! eventNode.hasType(kEvent))
                    continue;
                Event e;
                e.startSeconds         = static_cast<double>(eventNode.getProperty(kEventStart,    0.0));
                e.durationSeconds      = static_cast<double>(eventNode.getProperty(kEventDuration, 0.1));
                e.pitchOffsetSemitones = static_cast<float> (static_cast<double>(eventNode.getProperty(kEventPitch,    0.0)));
                // velocity defaults to 1.0 for v1 files that pre-date this field.
                e.velocity             = static_cast<float> (static_cast<double>(eventNode.getProperty(kEventVelocity, 1.0)));
                // probability / ratchets / humanize default to inert
                // values when missing — matches pre-v10 semantics
                // (every event always fires, no ratcheting, no jitter).
                e.probability          = static_cast<float> (static_cast<double>(eventNode.getProperty(kEventProbability, 1.0)));
                e.ratchets             = juce::jlimit(1, kMaxRatchets,
                                                       static_cast<int>(eventNode.getProperty(kEventRatchets, 1)));
                e.humanizeAmount       = static_cast<float> (static_cast<double>(eventNode.getProperty(kEventHumanize,    0.0)));

                // Walk OVERRIDE children, populating override slots
                // in encounter order. Excess children (more than
                // kNumEventOverrides) are silently dropped — same
                // tolerant policy v1 events use for missing fields.
                int slotIx = 0;
                for (auto overrideNode : eventNode)
                {
                    if (! overrideNode.hasType(kEventOverride))
                        continue;
                    if (slotIx >= kNumEventOverrides)
                        break;
                    const int destInt = juce::jlimit(0,
                        kNumModDestinations - 1,
                        static_cast<int>(overrideNode.getProperty(kOverrideDest, 0)));
                    e.overrides[static_cast<size_t>(slotIx)] = EventOverride {
                        static_cast<ModDestination>(destInt),
                        static_cast<float>(static_cast<double>(overrideNode.getProperty(kOverrideValue, 0.0)))
                    };
                    ++slotIx;
                }

                pattern.addEvent(laneIdx, e);
            }
        }

        // ---- Locks -------------------------------------------------
        auto& randomizer = processor.getRandomizer();
        randomizer.clearAllLocks();
        const auto locksNode = tree.getChildWithName(kLocks);
        for (auto lockNode : locksNode)
        {
            if (! lockNode.hasType(kLock))
                continue;
            randomizer.setLocked(lockNode.getProperty(kLockId, "").toString(), true);
        }

        // The restore touched APVTS, pitch curve, and looping state;
        // each of those independently flagged the processor dirty.
        // The whole point of "load" is "state now matches disk", so
        // markClean wins after all the bulk mutations are done.
        processor.markClean();

        // Tell the UI to resync widgets that don't auto-track APVTS
        // — pitch editor, pattern grid, length / grid combos.
        processor.notifyFullStateLoaded();

        return true;
    }

    juce::String toXmlString(const juce::ValueTree& projectTree)
    {
        return projectTree.toXmlString();
    }

    juce::ValueTree fromXmlString(const juce::String& xml)
    {
        if (auto parsed = juce::parseXML(xml))
            return juce::ValueTree::fromXml(*parsed);
        return {};
    }

    bool writeToFile(B33pProcessor& processor, const juce::File& destination)
    {
        const auto xml = toXmlString(save(processor));
        return destination.replaceWithText(xml);
    }

    bool readFromFile(B33pProcessor& processor, const juce::File& source)
    {
        if (! source.existsAsFile())
            return false;

        const auto xml  = source.loadFileAsString();
        const auto tree = fromXmlString(xml);
        if (! tree.isValid())
            return false;

        return load(processor, tree);
    }
}
