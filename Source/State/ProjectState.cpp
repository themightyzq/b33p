#include "ProjectState.h"

#include "B33pProcessor.h"
#include "Pattern/Pattern.h"

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
        const juce::Identifier kEvent                 { "EVENT" };
        const juce::Identifier kEventStart            { "start_seconds" };
        const juce::Identifier kEventDuration         { "duration_seconds" };
        const juce::Identifier kEventPitch            { "pitch_offset_semitones" };
        const juce::Identifier kEventVelocity         { "velocity" };

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
            laneNode.setProperty(kLaneIndex, laneIdx,                        nullptr);
            laneNode.setProperty(kLaneName,  pattern.getLaneName(laneIdx),    nullptr);
            laneNode.setProperty(kLaneMuted, pattern.isLaneMuted(laneIdx),    nullptr);

            for (const auto& e : pattern.getEvents(laneIdx))
            {
                juce::ValueTree eventNode { kEvent };
                eventNode.setProperty(kEventStart,    e.startSeconds,                       nullptr);
                eventNode.setProperty(kEventDuration, e.durationSeconds,                    nullptr);
                eventNode.setProperty(kEventPitch,    static_cast<double>(e.pitchOffsetSemitones), nullptr);
                eventNode.setProperty(kEventVelocity, static_cast<double>(e.velocity),             nullptr);
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

    juce::ValueTree migrate(juce::ValueTree tree)
    {
        // No migrations needed yet. Once v2 lands, switch on the
        // current version and apply transformations until the tree
        // matches kCurrentVersion.
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

            // Default-tolerant: pre-Phase-8 v1 files have neither
            // attribute, so empty name + unmuted is the right default.
            pattern.setLaneName (laneIdx, laneNode.getProperty(kLaneName, juce::String{}).toString());
            pattern.setLaneMuted(laneIdx, static_cast<bool>(laneNode.getProperty(kLaneMuted, false)));

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
