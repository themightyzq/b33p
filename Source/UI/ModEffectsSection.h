#pragma once

#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    // Per-lane wet effect slot. Picks one of the modulation effect
    // modes (None / Chorus / Reverb / Delay / Flanger / Phaser) via
    // a combo and exposes three normalised continuous parameters
    // (P1 / P2 / Mix) whose semantics depend on the active type.
    // The slider labels update on every type change so a fresh user
    // can tell what each knob is currently doing.
    class ModEffectsSection : public Section
    {
    public:
        explicit ModEffectsSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
        void onTypeChanged();

        B33pProcessor& processor;

        juce::ComboBox typeSelector;
        LabeledSlider  p1Slider  { "Param 1" };
        LabeledSlider  p2Slider  { "Param 2" };
        LabeledSlider  mixSlider { "Mix"     };

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   p1Attachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   p2Attachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModEffectsSection)
    };
}
