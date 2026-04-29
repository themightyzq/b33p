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
        addAndMakeVisible(morphSlider);
        addAndMakeVisible(fmRatioSlider);
        addAndMakeVisible(fmDepthSlider);

        // Edit button — only visible when waveform is Custom or
        // Wavetable, since both modes draw from the slot storage.
        customEditButton.setTooltip("Open the wavetable editor");
        customEditButton.onClick = [this] { openCustomWaveformEditor(); };
        addChildComponent(customEditButton);   // hidden until needed

        waveformSelector.setTooltip("Oscillator waveform");
        basePitchSlider .setTooltip("Base pitch of the oscillator (carrier pitch in FM mode)");
        morphSlider     .setTooltip("Wavetable mode: 0 = Slot 1, 1 = Slot 4. Blends adjacent slots in between.");
        fmRatioSlider   .setTooltip("FM mode: modulator pitch as a multiple of the carrier (1.0 = unison)");
        fmDepthSlider   .setTooltip("FM mode: modulation index. 0 = clean carrier sine; higher = more sidebands");

        retargetLane(processor.getSelectedLane());
    }

    void OscillatorSection::onWaveformChanged()
    {
        const int selectedId = waveformSelector.getSelectedId();
        const bool isCustom    = selectedId == 6;
        const bool isWavetable = selectedId == 7;
        const bool isFm        = selectedId == 8;
        const bool needsEditor = isCustom || isWavetable;

        const bool wasVisible = customEditButton.isVisible();
        customEditButton.setVisible(needsEditor);
        customEditButton.setButtonText(isWavetable ? "Edit slots..." : "Edit...");

        // Mode-specific sliders are greyed out (not hidden) outside
        // their relevant mode so the layout never shifts and the
        // user can still see at-a-glance which controls a given
        // mode exposes.
        morphSlider  .getSlider().setEnabled(isWavetable);
        fmRatioSlider.getSlider().setEnabled(isFm);
        fmDepthSlider.getSlider().setEnabled(isFm);

        // setVisible doesn't re-run resized() on its own, so the
        // button would otherwise sit at (0,0,0,0). Re-layout when
        // the visibility actually changed.
        if (wasVisible != needsEditor)
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

        wireRandomizerButtons(processor, waveformDice, waveformLock,
                              ParameterIDs::oscWaveform(lane));
        basePitchSlider.attachRandomizer(processor, ParameterIDs::basePitchHz(lane));
        morphSlider    .attachRandomizer(processor, ParameterIDs::wavetableMorph(lane));
        fmRatioSlider  .attachRandomizer(processor, ParameterIDs::fmRatio(lane));
        fmDepthSlider  .attachRandomizer(processor, ParameterIDs::fmDepth(lane));

        // SliderAttachment installs its own textFromValueFunction at
        // construction, so re-apply our formatting AFTER the
        // attachment exists.
        SliderFormatting::applyHz(basePitchSlider.getSlider());
        SliderFormatting::applyDecimal(morphSlider.getSlider(),   2);
        SliderFormatting::applyDecimal(fmRatioSlider.getSlider(), 2);
        SliderFormatting::applyDecimal(fmDepthSlider.getSlider(), 2);
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

        // Four sliders side-by-side in the bottom area. Mode-
        // specific sliders are greyed out (not hidden) so the
        // layout never shifts when the user changes the waveform.
        // Order: Pitch (always), Morph (Wavetable), Ratio + Depth (FM).
        const int sliderCellWidth = (bounds.getWidth() - 3 * kSliderGap) / 4;
        basePitchSlider.setBounds(bounds.removeFromLeft(sliderCellWidth));
        bounds.removeFromLeft(kSliderGap);
        morphSlider.setBounds(bounds.removeFromLeft(sliderCellWidth));
        bounds.removeFromLeft(kSliderGap);
        fmRatioSlider.setBounds(bounds.removeFromLeft(sliderCellWidth));
        bounds.removeFromLeft(kSliderGap);
        fmDepthSlider.setBounds(bounds);
    }
}
