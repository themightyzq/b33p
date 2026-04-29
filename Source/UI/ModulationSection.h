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
    class ModulationSection : public Section
    {
    public:
        explicit ModulationSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
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

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSection)
    };
}
