#pragma once

#include "DSP/ModulationMatrix.h"
#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

namespace B33p
{
    // Per-lane modulation panel. Top: two LFO controls (shape combo
    // + rate slider). Bottom: four matrix slots, each a row with
    // source / destination combos and a horizontal amount slider.
    // Sources and destinations are wired via the standard APVTS
    // attachments — the rest of the engine is in B33pProcessor.
    class ModulationSection : public Section,
                              private juce::Timer
    {
    public:
        explicit ModulationSection(B33pProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void timerCallback() override;

        void retargetLane(int lane);

    private:
        // Live modulation magnitude for slot (|LFO × amount|, 0..1) when
        // its routing is active, else a negative sentinel meaning "not
        // routed". Drives the per-row activity indicator (REVIEW.md P14).
        // Returns 0 (routed-but-idle, drawn as a steady minimum-alpha
        // fill) when no audio is currently being produced — the
        // accessibility rule that nothing in the UI should animate
        // while the synth is silent.
        float slotActivity (int slot) const;

        int currentLane { 0 };
        // Edge-tracks playback state across timer ticks so the indicator
        // strip gets one final repaint on the playing→idle transition,
        // settling the visuals to their static state instead of freezing
        // mid-pulse. Without this the LFO indicators kept their last
        // pulsed alpha after Stop and read as "still doing something."
        bool wasPlayingLastTick { false };
        std::array<juce::Rectangle<int>, kNumModSlots> slotIndicatorBounds;

        struct LfoControls
        {
            juce::ComboBox shape;
            LabeledSlider  rate { "Rate" };
            std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   rateAttachment;
        };

        struct SlotControls
        {
            juce::Label    label;
            juce::ComboBox source;
            juce::ComboBox dest;
            juce::Slider   amount;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destAttachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   amountAttachment;
        };

        B33pProcessor& processor;

        std::array<LfoControls,  kNumLfosPerLane> lfoControls;
        std::array<SlotControls, kNumModSlots>    slotControls;

        // Empty-state hint sitting between the LFO row and the matrix
        // rows. Always visible — short enough not to distract a power
        // user with the section in full flight, prominent enough to
        // give a first-time user a starting point.
        juce::Label hintLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSection)
    };
}
