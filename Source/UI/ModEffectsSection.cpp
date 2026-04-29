#include "ModEffectsSection.h"

#include "Core/ParameterIDs.h"
#include "DSP/ModulationEffect.h"
#include "SliderFormatting.h"

namespace B33p
{
    namespace
    {
        // Per-type slider labels, keyed by the active ModulationEffect::Type
        // enum value (cast to int). None reuses the chorus labels but greys
        // the sliders out — the section's enabled state communicates the
        // bypass.
        struct TypeLabels { const char* p1; const char* p2; const char* mix; };
        constexpr TypeLabels kTypeLabels[] = {
            { "Rate",   "Depth",  "Mix" },   // None (placeholder; sliders disabled)
            { "Rate",   "Depth",  "Mix" },   // Chorus
            { "Size",   "Damp",   "Mix" },   // Reverb
            { "Time",   "Fback",  "Mix" },   // Delay
            { "Rate",   "Depth",  "Mix" },   // Flanger
            { "Rate",   "Depth",  "Mix" },   // Phaser
        };

        constexpr int kNumModTypes = 6;
    }

    ModEffectsSection::ModEffectsSection(B33pProcessor& processorRef)
        : Section("Mod FX"),
          processor(processorRef)
    {
        // Item IDs (1..6) match ModulationEffect::Type enum order so
        // the APVTS choice index maps cleanly at attachment time.
        typeSelector.addItem("None",    1);
        typeSelector.addItem("Chorus",  2);
        typeSelector.addItem("Reverb",  3);
        typeSelector.addItem("Delay",   4);
        typeSelector.addItem("Flanger", 5);
        typeSelector.addItem("Phaser",  6);

        addAndMakeVisible(typeSelector);
        addAndMakeVisible(p1Slider);
        addAndMakeVisible(p2Slider);
        addAndMakeVisible(mixSlider);

        typeSelector.setTooltip("Modulation effect at the end of the chain (after distortion)");
        p1Slider .setTooltip("First effect parameter — meaning depends on the type (Rate / Size / Time)");
        p2Slider .setTooltip("Second effect parameter — meaning depends on the type (Depth / Damping / Feedback)");
        mixSlider.setTooltip("Wet/dry blend; 0 keeps the dry signal, 1 = full wet");

        retargetLane(processor.getSelectedLane());
    }

    void ModEffectsSection::onTypeChanged()
    {
        const int typeIndex = juce::jlimit(0, kNumModTypes - 1,
                                            typeSelector.getSelectedId() - 1);
        const auto& labels = kTypeLabels[typeIndex];

        p1Slider .setLabelText(labels.p1);
        p2Slider .setLabelText(labels.p2);
        mixSlider.setLabelText(labels.mix);

        // None mode is a bypass — grey the sliders out so a fresh
        // user gets a strong visual cue that nothing the knobs do
        // matters until they pick an effect.
        const bool effectActive = typeIndex != 0;
        p1Slider .getSlider().setEnabled(effectActive);
        p2Slider .getSlider().setEnabled(effectActive);
        mixSlider.getSlider().setEnabled(effectActive);
    }

    void ModEffectsSection::retargetLane(int lane)
    {
        typeAttachment.reset();
        p1Attachment.reset();
        p2Attachment.reset();
        mixAttachment.reset();

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getApvts(), ParameterIDs::modEffectType(lane), typeSelector);

        p1Attachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::modEffectParam1(lane), p1Slider .getSlider());
        p2Attachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::modEffectParam2(lane), p2Slider .getSlider());
        mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::modEffectMix(lane),    mixSlider.getSlider());

        p1Slider .attachRandomizer(processor, ParameterIDs::modEffectParam1(lane));
        p2Slider .attachRandomizer(processor, ParameterIDs::modEffectParam2(lane));
        mixSlider.attachRandomizer(processor, ParameterIDs::modEffectMix(lane));

        SliderFormatting::applyDecimal(p1Slider .getSlider(), 2);
        SliderFormatting::applyDecimal(p2Slider .getSlider(), 2);
        SliderFormatting::applyDecimal(mixSlider.getSlider(), 2);

        SliderFormatting::applyDoubleClickReset(p1Slider .getSlider(), processor.getApvts(), ParameterIDs::modEffectParam1(lane));
        SliderFormatting::applyDoubleClickReset(p2Slider .getSlider(), processor.getApvts(), ParameterIDs::modEffectParam2(lane));
        SliderFormatting::applyDoubleClickReset(mixSlider.getSlider(), processor.getApvts(), ParameterIDs::modEffectMix(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));

        typeSelector.onChange = [this] { onTypeChanged(); };
        onTypeChanged();
    }

    void ModEffectsSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kComboHeight = 24;
        constexpr int kComboGap    = 6;
        constexpr int kSliderGap   = 8;

        typeSelector.setBounds(bounds.removeFromTop(kComboHeight));
        bounds.removeFromTop(kComboGap);

        const int cellWidth = (bounds.getWidth() - 2 * kSliderGap) / 3;
        p1Slider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kSliderGap);
        p2Slider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kSliderGap);
        mixSlider.setBounds(bounds);
    }
}
