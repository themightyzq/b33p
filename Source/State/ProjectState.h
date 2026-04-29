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
        //
        // History:
        //   v1: single-voice MVP (one set of parameters, one custom table)
        //   v2: per-lane voices (lane-prefixed parameter IDs, per-lane custom_waveform attribute)
        //   v3: per-lane wavetable slots (WAVETABLE child with N SLOT children) +
        //       wavetable_morph parameter; v2's custom_waveform attribute migrates
        //       into WAVETABLE/SLOT[index=0]
        //   v4: FM oscillator (new fm_ratio + fm_depth parameters per lane); v3
        //       files migrate forward by simply bumping the version property —
        //       APVTS supplies the parameter defaults for the missing IDs
        //   v5: Ring modulator (new ring_ratio + ring_mix parameters per lane);
        //       same default-fallback migration as v4
        //   v6: multi-mode filter (new filter_type choice + filter_vowel float
        //       per lane); same default-fallback migration. v5 files load with
        //       filter_type defaulting to Lowpass — the existing cutoff / Q
        //       behaviour — so they sound identical.
        //   v7: modulation effect slot (mod_effect_type choice + p1/p2/mix
        //       floats per lane); same default-fallback migration. v6 files
        //       load with mod_effect_type defaulting to None (= bypass), so
        //       the wet effect chain stays inert.
        //   v8: LFOs and modulation matrix (lfoN_shape/_rate_hz, modN_source/
        //       _dest/_amount per lane); same default-fallback migration. v7
        //       files load with every matrix slot's source / destination
        //       defaulting to None, so the modulation engine stays inert.
        //   v9: per-event parameter overrides — events grow OVERRIDE child
        //       nodes carrying (destination, value) pairs. v8 events have no
        //       overrides; they reload as inert (every slot's destination
        //       defaults to None).
        inline constexpr int kCurrentVersion = 9;

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
