#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <string>
#include <unordered_set>
#include <vector>

namespace B33p
{
    // Per-parameter randomization + lock state layered over an APVTS.
    //
    // Rolls a parameter by drawing a uniform normalized [0, 1] value
    // and pushing it through setValueNotifyingHost, so the parameter's
    // own NormalisableRange (including skew) shapes the distribution.
    // Log-skewed Hz parameters therefore roll roughly uniform-per-octave
    // rather than uniform-in-Hz, which reads as musical rather than
    // bass-weighted.
    //
    // Locks are plain meta-state — a set of parameter IDs. They are
    // intentionally NOT routed through the UndoManager: toggling a
    // lock is a UX gesture, not a parameter edit. Individual rolls
    // and the "Dice All" batch DO go through the UndoManager (one
    // transaction per click; one transaction for the full batch).
    //
    // Lives in State/ alongside the APVTS wiring. The lock set will
    // travel into the project ValueTree in Phase 6.
    class ParameterRandomizer
    {
    public:
        explicit ParameterRandomizer(juce::AudioProcessorValueTreeState& apvts);

        bool isLocked(const juce::String& parameterID) const;
        void setLocked(const juce::String& parameterID, bool locked);

        // Snapshot of every currently-locked parameter ID. Used by
        // the project-state serializer; ordering is not guaranteed.
        std::vector<juce::String> getLockedParameterIDs() const;

        // Wipes every lock. Used when loading a project so locks
        // restored from disk fully replace any in-memory locks.
        void clearAllLocks();

        // Returns true iff the parameter existed and was unlocked.
        // Begins its own UndoManager transaction so each click is one
        // undoable step.
        bool rollOne(const juce::String& parameterID, juce::Random& rng);

        // Rolls every unlocked parameter that has an ID. All edits
        // land in a single UndoManager transaction labelled "Dice All".
        void rollAllUnlocked(juce::Random& rng);

        // Rolls every unlocked parameter in `parameterIDs`. One
        // undoable transaction. Used by the "Dice Lane" entry point
        // to scope randomisation to a single lane's voice.
        void rollMany(const std::vector<juce::String>& parameterIDs,
                      juce::Random& rng,
                      const juce::String& transactionName);

    private:
        // Shared core used by both public entry points so the batch
        // call doesn't fragment its transaction.
        bool rollOneNoTransaction(const juce::String& parameterID, juce::Random& rng);

        juce::AudioProcessorValueTreeState& apvts;
        std::unordered_set<std::string>     lockedIds;
    };
}
