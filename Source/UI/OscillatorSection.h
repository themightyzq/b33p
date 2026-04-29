#pragma once

#include "IconButton.h"
#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"
#include "WaveformEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class OscillatorSection : public Section
    {
    public:
        explicit OscillatorSection(B33pProcessor& processor);

        void resized() override;

        void retargetLane(int lane);

    private:

        B33pProcessor& processor;

        void onWaveformChanged();
        void openCustomWaveformEditor();

        juce::ComboBox   waveformSelector;
        IconButton       waveformDice { IconButton::Glyph::Die  };
        IconButton       waveformLock { IconButton::Glyph::Lock };
        juce::TextButton customEditButton { "Edit..." };

        LabeledSlider    basePitchSlider { "Pitch" };
        LabeledSlider    morphSlider     { "Morph" };
        LabeledSlider    fmRatioSlider   { "Ratio" };
        LabeledSlider    fmDepthSlider   { "Depth" };

        // unique_ptr so retargetLane can swap them out.
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   basePitchAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   morphAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   fmRatioAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   fmDepthAttachment;

        // Lazily-created popup for editing the lane's custom
        // single-cycle waveform. Visibility is toggled by the
        // Edit button + waveform-combo state.
        std::unique_ptr<WaveformEditorWindow>                                   customEditorWindow;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorSection)
    };
}
