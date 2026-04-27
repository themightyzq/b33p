#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace B33p
{
    class B33pProcessor;

    namespace ProjectState
    {
        // .beep file format version. Bump on any breaking schema
        // change and add a corresponding case to the migrate()
        // function below — never break older saved projects.
        inline constexpr int kCurrentVersion = 2;

        // Captures every persistable piece of state the processor
        // owns: APVTS parameter values, the drawn pitch envelope
        // curve, the pattern (length + looping flag + four lanes
        // of events), and the randomizer's locked-parameter set.
        //
        // Transient runtime state (audition trigger, playhead
        // position, etc.) is NOT captured.
        //
        // Non-const because juce::AudioProcessorValueTreeState's
        // copyState() locks internally and is therefore non-const.
        juce::ValueTree save(B33pProcessor& processor);

        // Mutates the processor in place to match the supplied
        // tree. Returns true on success, false if the tree is not
        // a B33P tree, the version is from the future, or the
        // root tag is missing.
        bool load(B33pProcessor& processor, juce::ValueTree tree);

        // Forward-only schema migration. Inspects the tree's
        // version property and applies whatever migrations are
        // needed to bring it up to kCurrentVersion. Currently a
        // no-op since v1 is the only known version.
        juce::ValueTree migrate(juce::ValueTree tree);

        // Convenience round-trip helpers that wrap the above with
        // XML serialization. The processor's getStateInformation /
        // setStateInformation overrides use these so the JUCE
        // state contract and the .beep file format share one
        // serialization path.
        juce::String     toXmlString(const juce::ValueTree& projectTree);
        juce::ValueTree  fromXmlString(const juce::String& xml);

        // Disk I/O wrappers. writeToFile serialises via save +
        // toXmlString and overwrites the destination atomically;
        // readFromFile parses via fromXmlString + load. Both
        // return true on success. Failure modes: missing source
        // file, malformed XML, version mismatch, unwriteable
        // destination. Errors surface as false; logging is the
        // caller's job.
        bool writeToFile(B33pProcessor& processor, const juce::File& destination);
        bool readFromFile(B33pProcessor& processor, const juce::File& source);
    }
}
