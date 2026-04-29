#include "OscillatorSection.h"

#include "Core/ParameterIDs.h"
#include "RandomizerWiring.h"
#include "SliderFormatting.h"

namespace B33p
{
    OscillatorSection::OscillatorSection(B33pProcessor& processorRef)
        : Section("Oscillator"),
          processor(processorRef)
    {
        // Item IDs (1..7) match B33p::Oscillator::Waveform enum order
        // so the APVTS choice index maps cleanly at wiring time.
        waveformSelector.addItem("Sine",      1);
        waveformSelector.addItem("Square",    2);
        waveformSelector.addItem("Triangle",  3);
        waveformSelector.addItem("Saw",       4);
        waveformSelector.addItem("Noise",     5);
        waveformSelector.addItem("Custom",    6);
        waveformSelector.addItem("Wavetable", 7);

        addAndMakeVisible(waveformSelector);
        addAndMakeVisible(waveformDice);
        addAndMakeVisible(waveformLock);
        addAndMakeVisible(basePitchSlider);
        // Mode-specific sliders start hidden; onWaveformChanged
        // shows the relevant ones based on the selected waveform.
        addChildComponent(morphSlider);
        addChildComponent(fmRatioSlider);
        addChildComponent(fmDepthSlider);
        addChildComponent(ringRatioSlider);
        addChildComponent(ringMixSlider);

        // Edit button — only visible when waveform is Custom or
        // Wavetable, since both modes draw from the slot storage.
        customEditButton.setTooltip("Open the wavetable editor");
        customEditButton.onClick = [this] { openCustomWaveformEditor(); };
        addChildComponent(customEditButton);   // hidden until needed

        waveformSelector.setTooltip("Oscillator waveform");
        basePitchSlider .setTooltip("Base pitch of the oscillator (carrier pitch in FM / Ring modes)");
        morphSlider     .setTooltip("Wavetable mode: 0 = Slot 1, 1 = Slot 4. Blends adjacent slots in between.");
        fmRatioSlider   .setTooltip("FM mode: modulator pitch as a multiple of the carrier (1.0 = unison)");
        fmDepthSlider   .setTooltip("FM mode: modulation index. 0 = clean carrier sine; higher = more sidebands");
        ringRatioSlider .setTooltip("Ring mode: modulator pitch as a multiple of the carrier");
        ringMixSlider   .setTooltip("Ring mode: 0 = clean carrier sine, 1 = full ring-modulated product");

        retargetLane(processor.getSelectedLane());
    }

    void OscillatorSection::onWaveformChanged()
    {
        const int selectedId = waveformSelector.getSelectedId();
        const bool isCustom    = selectedId == 6;
        const bool isWavetable = selectedId == 7;
        const bool isFm        = selectedId == 8;
        const bool isRing      = selectedId == 9;
        const bool needsEditor = isCustom || isWavetable;

        customEditButton.setVisible(needsEditor);
        customEditButton.setButtonText(isWavetable ? "Edit slots..." : "Edit...");

        // Conditional layout: only the mode-relevant sliders show up
        // alongside Pitch. The hidden sliders still exist (and stay
        // attached to APVTS) so randomising "Lane" still touches
        // every parameter — they just don't take up screen real
        // estate when their mode isn't active. Pitch always remains
        // since every mode uses it.
        morphSlider    .setVisible(isWavetable);
        fmRatioSlider  .setVisible(isFm);
        fmDepthSlider  .setVisible(isFm);
        ringRatioSlider.setVisible(isRing);
        ringMixSlider  .setVisible(isRing);

        // Visibility changes don't re-run resized() on their own —
        // call it explicitly so the layout reflows around whatever
        // is now visible.
        resized();

        // If we already had the editor open, swing it onto the
        // currently-selected lane (the user may have flipped this
        // lane to Custom or Wavetable mid-session).
        if (needsEditor && customEditorWindow != nullptr
            && customEditorWindow->isVisible())
        {
            customEditorWindow->getEditor().setLane(processor.getSelectedLane());
        }
    }

    void OscillatorSection::openCustomWaveformEditor()
    {
        if (customEditorWindow == nullptr)
            customEditorWindow = std::make_unique<WaveformEditorWindow>(processor);
        customEditorWindow->getEditor().setLane(processor.getSelectedLane());
        customEditorWindow->setName("Custom Waveform"
                                      + processor.laneTitleSuffix(processor.getSelectedLane()));
        customEditorWindow->setVisible(true);
        customEditorWindow->toFront(true);
    }

    void OscillatorSection::retargetLane(int lane)
    {
        // Drop the old attachments first; constructing a new one
        // immediately starts pushing parameter values into the
        // controls, which is what we want.
        waveformAttachment.reset();
        basePitchAttachment.reset();
        morphAttachment.reset();
        fmRatioAttachment.reset();
        fmDepthAttachment.reset();
        ringRatioAttachment.reset();
        ringMixAttachment.reset();

        waveformAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getApvts(),
                ParameterIDs::oscWaveform(lane),
                waveformSelector);

        basePitchAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::basePitchHz(lane),
                basePitchSlider.getSlider());

        morphAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::wavetableMorph(lane),
                morphSlider.getSlider());

        fmRatioAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::fmRatio(lane),
                fmRatioSlider.getSlider());

        fmDepthAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::fmDepth(lane),
                fmDepthSlider.getSlider());

        ringRatioAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::ringRatio(lane),
                ringRatioSlider.getSlider());

        ringMixAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getApvts(),
                ParameterIDs::ringMix(lane),
                ringMixSlider.getSlider());

        wireRandomizerButtons(processor, waveformDice, waveformLock,
                              ParameterIDs::oscWaveform(lane));
        basePitchSlider.attachRandomizer(processor, ParameterIDs::basePitchHz(lane));
        morphSlider    .attachRandomizer(processor, ParameterIDs::wavetableMorph(lane));
        fmRatioSlider  .attachRandomizer(processor, ParameterIDs::fmRatio(lane));
        fmDepthSlider  .attachRandomizer(processor, ParameterIDs::fmDepth(lane));
        ringRatioSlider.attachRandomizer(processor, ParameterIDs::ringRatio(lane));
        ringMixSlider  .attachRandomizer(processor, ParameterIDs::ringMix(lane));

        // SliderAttachment installs its own textFromValueFunction at
        // construction, so re-apply our formatting AFTER the
        // attachment exists.
        SliderFormatting::applyHz(basePitchSlider.getSlider());
        SliderFormatting::applyDecimal(morphSlider.getSlider(),     2);
        SliderFormatting::applyDecimal(fmRatioSlider.getSlider(),   2);
        SliderFormatting::applyDecimal(fmDepthSlider.getSlider(),   2);
        SliderFormatting::applyDecimal(ringRatioSlider.getSlider(), 2);
        SliderFormatting::applyDecimal(ringMixSlider.getSlider(),   2);
        SliderFormatting::applyDoubleClickReset(basePitchSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::basePitchHz(lane));
        SliderFormatting::applyDoubleClickReset(morphSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::wavetableMorph(lane));
        SliderFormatting::applyDoubleClickReset(fmRatioSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::fmRatio(lane));
        SliderFormatting::applyDoubleClickReset(fmDepthSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::fmDepth(lane));
        SliderFormatting::applyDoubleClickReset(ringRatioSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::ringRatio(lane));
        SliderFormatting::applyDoubleClickReset(ringMixSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::ringMix(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));

        // Whenever the user switches lanes, the combo's selected ID
        // is reset by the new ComboBoxAttachment. Hook the onChange
        // here (after attachment construction) so we catch both
        // user-driven combo edits and lane-switch syncs.
        waveformSelector.onChange = [this] { onWaveformChanged(); };
        onWaveformChanged();

        // Keep the popup editor in sync when retargetLane fires
        // (which happens for selected-lane changes AND for project
        // Open / New via OnFullStateLoaded). Otherwise the editor
        // would display the previous project's table for the same
        // lane index.
        if (customEditorWindow != nullptr && customEditorWindow->isVisible())
        {
            customEditorWindow->getEditor().setLane(lane);
            customEditorWindow->setName("Custom Waveform"
                                          + processor.laneTitleSuffix(lane));
        }
    }

    void OscillatorSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kComboHeight  = 26;
        constexpr int kButtonWidth  = 32;
        constexpr int kEditWidth    = 80;
        constexpr int kButtonGap    = 2;
        constexpr int kComboGap     = 4;
        constexpr int kRowGap       = 8;
        constexpr int kSliderGap    = 6;

        auto topRow = bounds.removeFromTop(kComboHeight);
        waveformLock.setBounds(topRow.removeFromRight(kButtonWidth));
        topRow.removeFromRight(kButtonGap);
        waveformDice.setBounds(topRow.removeFromRight(kButtonWidth));
        topRow.removeFromRight(kComboGap);
        // Edit button sits between the combo and the dice — only
        // visible (per setVisible in onWaveformChanged) when the
        // combo is on Custom or Wavetable.
        if (customEditButton.isVisible())
        {
            customEditButton.setBounds(topRow.removeFromRight(kEditWidth));
            topRow.removeFromRight(kButtonGap);
        }
        waveformSelector.setBounds(topRow);

        bounds.removeFromTop(kRowGap);

        // Conditional bottom row: Pitch is always present; mode-
        // specific sliders join it left-to-right only when their
        // mode is active. Total visible slider count drives the
        // cell width so two sliders fill half each, three split
        // into thirds, etc. Keeps the bottom area tidy regardless
        // of how many oscillator modes ship in the future.
        std::vector<juce::Component*> visibleSliders;
        visibleSliders.push_back(&basePitchSlider);
        if (morphSlider.isVisible())     visibleSliders.push_back(&morphSlider);
        if (fmRatioSlider.isVisible())   visibleSliders.push_back(&fmRatioSlider);
        if (fmDepthSlider.isVisible())   visibleSliders.push_back(&fmDepthSlider);
        if (ringRatioSlider.isVisible()) visibleSliders.push_back(&ringRatioSlider);
        if (ringMixSlider.isVisible())   visibleSliders.push_back(&ringMixSlider);

        const int n = static_cast<int>(visibleSliders.size());
        const int sliderCellWidth = (bounds.getWidth() - (n - 1) * kSliderGap) / n;
        for (int i = 0; i < n; ++i)
        {
            visibleSliders[static_cast<size_t>(i)]->setBounds(
                i == n - 1 ? bounds : bounds.removeFromLeft(sliderCellWidth));
            if (i < n - 1)
                bounds.removeFromLeft(kSliderGap);
        }
    }
}
