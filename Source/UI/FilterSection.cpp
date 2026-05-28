#include "FilterSection.h"

#include "Core/ParameterIDs.h"
#include "ModulationGlow.h"
#include "SliderFormatting.h"

#include <vector>

namespace B33p
{
    FilterSection::FilterSection(B33pProcessor& processorRef)
        : Section("Filter"),
          processor(processorRef),
          visualizer(processorRef.getApvts())
    {
        // Item IDs (1..5) match Filter::Type enum order so the
        // APVTS choice index maps cleanly at attachment time.
        typeSelector.addItem("Lowpass",  1);
        typeSelector.addItem("Highpass", 2);
        typeSelector.addItem("Bandpass", 3);
        typeSelector.addItem("Comb",     4);
        typeSelector.addItem("Formant",  5);

        addAndMakeVisible(visualizer);
        addAndMakeVisible(typeSelector);
        addAndMakeVisible(cutoffSlider);
        addAndMakeVisible(resonanceSlider);
        // Vowel starts hidden; onTypeChanged shows / hides it as
        // the filter type flips.
        addChildComponent(vowelSlider);

        typeSelector   .setTooltip("Filter mode: shapes how the voice is filtered before bitcrush + distortion");
        cutoffSlider   .setTooltip("Cutoff / centre / comb fundamental — meaning depends on filter type");
        resonanceSlider.setTooltip("Resonance / Q / comb feedback — meaning depends on filter type");
        vowelSlider    .setTooltip("Formant mode: 0 = A, 1 = U with E / I / O at quarter steps in between");

        retargetLane(processor.getSelectedLane());

        // 30 Hz — fast enough that an LFO at the maximum rate still reads
        // as a smooth pulse, slow enough to be cheap when nothing's
        // routed (setModulationIntensity short-circuits on zero).
        startTimerHz(30);
    }

    void FilterSection::onTypeChanged()
    {
        const int selectedId = typeSelector.getSelectedId();
        const bool isFormant = selectedId == 5;

        // Formant mode replaces the cutoff/resonance pair with a
        // single vowel slider. The other modes share the
        // two-slider layout, so visibility flips per type.
        cutoffSlider   .setVisible(! isFormant);
        resonanceSlider.setVisible(! isFormant);
        vowelSlider    .setVisible(isFormant);

        resized();
    }

    void FilterSection::retargetLane(int lane)
    {
        currentLane = lane;
        typeAttachment.reset();
        cutoffAttachment.reset();
        resonanceAttachment.reset();
        vowelAttachment.reset();

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getApvts(), ParameterIDs::filterType(lane), typeSelector);

        cutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::filterCutoffHz(lane),   cutoffSlider   .getSlider());
        resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::filterResonanceQ(lane), resonanceSlider.getSlider());
        vowelAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::filterVowel(lane),      vowelSlider    .getSlider());

        cutoffSlider   .attachRandomizer(processor, ParameterIDs::filterCutoffHz(lane));
        resonanceSlider.attachRandomizer(processor, ParameterIDs::filterResonanceQ(lane));
        vowelSlider    .attachRandomizer(processor, ParameterIDs::filterVowel(lane));

        SliderFormatting::applyHz     (cutoffSlider   .getSlider());
        SliderFormatting::applyDecimal(resonanceSlider.getSlider(), 2);
        SliderFormatting::applyDecimal(vowelSlider    .getSlider(), 2);

        SliderFormatting::applyDoubleClickReset(cutoffSlider   .getSlider(), processor.getApvts(), ParameterIDs::filterCutoffHz(lane));
        SliderFormatting::applyDoubleClickReset(resonanceSlider.getSlider(), processor.getApvts(), ParameterIDs::filterResonanceQ(lane));
        SliderFormatting::applyDoubleClickReset(vowelSlider    .getSlider(), processor.getApvts(), ParameterIDs::filterVowel(lane));

        visualizer.retargetLane(lane);
        setAccentColour(processor.laneAccentColour(lane));

        // Re-hook onChange after attachment swap so the listener
        // reflects the new lane's type immediately.
        typeSelector.onChange = [this] { onTypeChanged(); };
        onTypeChanged();
    }

    void FilterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kVisualizerHeight = 90;
        constexpr int kVisualizerGap    = 6;
        constexpr int kComboHeight      = 24;
        constexpr int kComboGap         = 6;
        constexpr int kSliderGap        = 8;

        visualizer.setBounds(bounds.removeFromTop(kVisualizerHeight));
        bounds.removeFromTop(kVisualizerGap);

        typeSelector.setBounds(bounds.removeFromTop(kComboHeight));
        bounds.removeFromTop(kComboGap);

        std::vector<juce::Component*> visibleSliders;
        if (cutoffSlider.isVisible())    visibleSliders.push_back(&cutoffSlider);
        if (resonanceSlider.isVisible()) visibleSliders.push_back(&resonanceSlider);
        if (vowelSlider.isVisible())     visibleSliders.push_back(&vowelSlider);

        if (visibleSliders.empty())
            return;

        const int n = static_cast<int>(visibleSliders.size());

        // Cap cell width so unchanged sliders (e.g. Cutoff) do not
        // visibly grow when switching to a filter mode with fewer
        // visible knobs. See OscillatorSection::resized for the
        // rationale — same pattern applied here.
        constexpr int kMaxSliderCellWidth = 180;
        const int proportionalWidth = (bounds.getWidth() - (n - 1) * kSliderGap) / n;
        const int cellWidth         = std::min(proportionalWidth, kMaxSliderCellWidth);

        const int totalCellsWidth = n * cellWidth + (n - 1) * kSliderGap;
        const int leftPadding     = (bounds.getWidth() - totalCellsWidth) / 2;
        bounds.removeFromLeft(leftPadding);

        for (int i = 0; i < n; ++i)
        {
            visibleSliders[static_cast<size_t>(i)]->setBounds(
                bounds.removeFromLeft(cellWidth));
            if (i < n - 1)
                bounds.removeFromLeft(kSliderGap);
        }
    }

    void FilterSection::timerCallback()
    {
        // Gate: glow reflects current playback only.
        if (! processor.isSelectedLaneVoiceActive())
        {
            cutoffSlider   .setModulationIntensity(0.0f);
            resonanceSlider.setModulationIntensity(0.0f);
            return;
        }

        const float lfo1 = processor.getSelectedLaneLfoValue(0);
        const float lfo2 = processor.getSelectedLaneLfoValue(1);
        auto& apvts = processor.getApvts();

        cutoffSlider.setModulationIntensity(
            ModulationGlow::computeMatrixIntensity(
                apvts, currentLane, ModDestination::FilterCutoff, lfo1, lfo2));
        resonanceSlider.setModulationIntensity(
            ModulationGlow::computeMatrixIntensity(
                apvts, currentLane, ModDestination::FilterResonance, lfo1, lfo2));
    }
}
