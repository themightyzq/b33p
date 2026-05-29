#include "ModEffectsSection.h"

#include "Core/ParameterIDs.h"
#include "DSP/ModulationEffect.h"
#include "ModulationGlow.h"
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
            { "—",      "—",      "—"   },   // None — neutral labels rather
                                              // than the misleading "Rate /
                                              // Depth / Mix" placeholder.
                                              // The dashes pair with the
                                              // disabled slider state to say
                                              // "nothing meaningful here yet"
                                              // without advertising
                                              // functionality that isn't
                                              // active.
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
        startTimerHz(30);
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
        currentLane = lane;
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

        setTitleSuffix(processor.laneTitleSuffix(lane));   // REVIEW-USER R-MISSING-6
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

    void ModEffectsSection::timerCallback()
    {
        // Gate: glow reflects current playback only. Without audio
        // moving through the effect there's no warble to surface, so
        // the section stays dark — which was the user's complaint with
        // the first cut (Mod FX = Chorus glowed continuously even when
        // nothing was playing).
        if (! processor.isSelectedLaneVoiceActive())
        {
            p1Slider .setModulationIntensity(0.0f);
            p2Slider .setModulationIntensity(0.0f);
            mixSlider.setModulationIntensity(0.0f);
            activityPhase = 0.0f;   // restart pulse when playback resumes
            return;
        }

        const float lfo1 = processor.getSelectedLaneLfoValue(0);
        const float lfo2 = processor.getSelectedLaneLfoValue(1);
        auto& apvts = processor.getApvts();

        // Mod FX activity pulse. When the active type has an internal
        // LFO (chorus/flanger/phaser) AND audio is playing through it,
        // pulses at the effect's rate so the user can SEE which section
        // is producing the warble. Combined with matrix intensity via
        // max so a routed LFO still shows through.
        const float activity = computeModFxActivity();

        const auto applyGlow = [&](LabeledSlider& slider, ModDestination dest)
        {
            const float matrix = ModulationGlow::computeMatrixIntensity(
                apvts, currentLane, dest, lfo1, lfo2);
            slider.setModulationIntensity(std::max(matrix, activity));
        };

        applyGlow(p1Slider,  ModDestination::ModEffectParam1);
        applyGlow(p2Slider,  ModDestination::ModEffectParam2);
        applyGlow(mixSlider, ModDestination::ModEffectMix);
    }

    float ModEffectsSection::computeModFxActivity()
    {
        auto& apvts = processor.getApvts();
        const auto* typeRaw = apvts.getRawParameterValue(
            ParameterIDs::modEffectType(currentLane));
        if (typeRaw == nullptr)
            return 0.0f;

        const int typeIndex = static_cast<int>(typeRaw->load());
        const bool hasInternalLfo =
               typeIndex == static_cast<int>(ModulationEffect::Type::Chorus)
            || typeIndex == static_cast<int>(ModulationEffect::Type::Flanger)
            || typeIndex == static_cast<int>(ModulationEffect::Type::Phaser);

        if (! hasInternalLfo)
        {
            activityPhase = 0.0f;
            return 0.0f;
        }

        const auto* p1Raw  = apvts.getRawParameterValue(
            ParameterIDs::modEffectParam1(currentLane));
        const auto* mixRaw = apvts.getRawParameterValue(
            ParameterIDs::modEffectMix(currentLane));
        if (p1Raw == nullptr || mixRaw == nullptr)
            return 0.0f;

        const float p1  = juce::jlimit(0.0f, 1.0f, p1Raw->load());
        const float mix = juce::jlimit(0.0f, 1.0f, mixRaw->load());

        // Rate ranges per ModulationEffect.h: Chorus 0.1..5 Hz, Flanger
        // 0.1..3 Hz, Phaser 0.1..5 Hz. UI is a phase clock, not the
        // audio LFO itself — so it'll drift in phase from the actual
        // effect, but it tracks the rate, which is what makes the
        // pulse visibly correlate with the warble.
        const float maxRate = (typeIndex == static_cast<int>(ModulationEffect::Type::Flanger))
                                ? 3.0f : 5.0f;
        const float rateHz  = 0.1f + p1 * (maxRate - 0.1f);

        constexpr float kTimerHz = 30.0f;
        activityPhase += juce::MathConstants<float>::twoPi * rateHz / kTimerHz;
        while (activityPhase > juce::MathConstants<float>::twoPi)
            activityPhase -= juce::MathConstants<float>::twoPi;

        // Bell-shaped 0..1 pulse via (1+sin)/2. Scale by mix so a
        // fully-dry effect — which can't actually warble the output —
        // doesn't glow.
        const float pulse = 0.5f * (1.0f + std::sin(activityPhase));
        return pulse * mix;
    }
}
