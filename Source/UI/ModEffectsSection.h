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
                            , private juce::Timer
    {
    public:
        explicit ModEffectsSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:
        void onTypeChanged();
        void timerCallback() override;
        // Returns the Mod FX "activity" pulse (0..1) for the current
        // effect — a UI-side phase clock at the effect's internal LFO
        // rate, scaled by Mix so a dry effect doesn't glow. Returns 0
        // for None / Reverb / Delay (those modes don't warble).
        float computeModFxActivity();

        B33pProcessor& processor;
        int            currentLane { 0 };

        // UI-side phase for the activity-glow pulse. Lives at -infty..2π;
        // wrapped per tick. Resets to 0 whenever the type leaves the
        // chorus / flanger / phaser group so re-entering starts clean.
        float          activityPhase { 0.0f };

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
