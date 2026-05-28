#include "MasterSection.h"

#include "Core/ParameterIDs.h"
#include "ModulationGlow.h"
#include "SliderFormatting.h"

namespace B33p
{
    MasterSection::MasterSection(B33pProcessor& processorRef)
        : Section("Master"),
          processor(processorRef)
    {
        addAndMakeVisible(gainSlider);

        auditionButton.onClick = [this]
        {
            processor.triggerAudition();
            flashAuditionButton();
        };
        addAndMakeVisible(auditionButton);

        // "Randomize All" rolls every unlocked parameter for the
        // currently-selected lane. The scope is the selected lane —
        // "all" refers to all of that lane's parameters. Rolling
        // parameters across multiple lanes lives in the pattern
        // editor's "Randomize Params" button + the Lane menu, so the
        // more frequent single-lane action stays in the master strip.
        diceAllButton.setButtonText("Randomize All");
        diceAllButton.onClick = [this]
        {
            juce::Random rng;
            const int lane = processor.getSelectedLane();
            processor.getRandomizer().rollMany(
                ParameterIDs::allForLane(lane), rng,
                "Randomize All (lane " + juce::String(lane + 1) + ")");
            // Audible-randomize rule: never leave the lane silent.
            processor.ensureLaneAudibleAfterRandomize(lane, rng);
        };
        addAndMakeVisible(diceAllButton);

        gainSlider    .setTooltip("Master output level");
        auditionButton.setTooltip("Play a single beep with the current settings (Shift+Space)");
        diceAllButton .setTooltip("Randomize all unlocked parameters on the currently-selected lane");

        // ---- A/B compare buttons ----------------------------------
        abButtonA.setClickingTogglesState(false);
        abButtonB.setClickingTogglesState(false);
        abButtonA.setTooltip("A/B compare — switch to slot A. First switch to B copies A into B so you can tweak it independently.");
        abButtonB.setTooltip("A/B compare — switch to slot B. Tweak independently of A; click A again to compare.");
        abCopyButton.setTooltip("Copy the active A/B slot's settings into the other slot.");

        const auto kActiveBg   = juce::Colour::fromRGB(80, 130, 200);
        const auto kInactiveBg = juce::Colour::fromRGB(48, 48, 48);
        const auto kActiveText = juce::Colour::fromRGB(255, 255, 255);
        const auto kInactiveText = juce::Colour::fromRGB(160, 160, 160);
        for (auto* b : { &abButtonA, &abButtonB })
        {
            b->setColour(juce::TextButton::buttonColourId,    kInactiveBg);
            b->setColour(juce::TextButton::buttonOnColourId,  kActiveBg);
            b->setColour(juce::TextButton::textColourOffId,   kInactiveText);
            b->setColour(juce::TextButton::textColourOnId,    kActiveText);
        }

        abButtonA.onClick = [this]
        {
            processor.switchAbSlot('A');
            refreshAbButtonStates();
        };
        abButtonB.onClick = [this]
        {
            processor.switchAbSlot('B');
            refreshAbButtonStates();
        };
        abCopyButton.onClick = [this]
        {
            processor.copyActiveAbSlotToOther();
        };

        addAndMakeVisible(abButtonA);
        addAndMakeVisible(abButtonB);
        addAndMakeVisible(abCopyButton);
        refreshAbButtonStates();

        // ---- Undo / Redo buttons ----------------------------------
        undoButton.onClick = [this] { processor.getUndoManager().undo(); };
        redoButton.onClick = [this] { processor.getUndoManager().redo(); };
        undoButton.setTooltip("Undo (Cmd+Z) — the plugin's own undo, in case the host captures the keyboard shortcut.");
        redoButton.setTooltip("Redo (Cmd+Shift+Z)");
        addAndMakeVisible(undoButton);
        addAndMakeVisible(redoButton);
        refreshUndoButtonStates();

        // ---- Preset prev/next (P23) -------------------------------
        prevPresetButton.onClick = [this] { if (onPrevPreset) onPrevPreset(); };
        nextPresetButton.onClick = [this] { if (onNextPreset) onNextPreset(); };
        prevPresetButton.setTooltip("Load the previous preset");
        nextPresetButton.setTooltip("Load the next preset");
        addAndMakeVisible(prevPresetButton);
        addAndMakeVisible(nextPresetButton);

        presetNameLabel.setJustificationType(juce::Justification::centred);
        presetNameLabel.setFont(juce::FontOptions(11.0f));
        presetNameLabel.setColour(juce::Label::textColourId,
                                  juce::Colour::fromRGB(170, 170, 175));
        presetNameLabel.setTooltip("Current preset — use < and > to step through your presets");
        presetNameLabel.setInterceptsMouseClicks(false, false);
        setPresetName({});   // starts as the em-dash placeholder
        addAndMakeVisible(presetNameLabel);

        retargetLane(processor.getSelectedLane());

        // 30 Hz timer drives the level meter readout + handles the
        // brief audition-button flash via deadline check.
        startTimerHz(30);
    }

    void MasterSection::retargetLane(int lane)
    {
        gainAttachment.reset();
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getApvts(), ParameterIDs::voiceGain(lane), gainSlider.getSlider());

        // gainSlider was constructed with showRandomizer=false so
        // attachRandomizer is a no-op — we still call it here for
        // symmetry with the other sections in case the policy ever
        // changes per-lane.
        gainSlider.attachRandomizer(processor, ParameterIDs::voiceGain(lane));
        // Display the linear parameter value as dB — audio users speak
        // dB. Round-trips via valueFromTextFunction so a typed
        // "+6 dB" parses back to ~2.0 linear.
        SliderFormatting::applyLinearGainAsDb(gainSlider.getSlider());
        SliderFormatting::applyDoubleClickReset(gainSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::voiceGain(lane));

        setAccentColour(processor.laneAccentColour(lane));
    }

    void MasterSection::flashAuditionButton()
    {
        // Brief amber tint so the user gets a visual confirmation
        // that the click registered — the audition is a fire-and-
        // forget half-second beep with no other on-screen feedback.
        auditionButton.setColour(juce::TextButton::buttonColourId,
                                  juce::Colour::fromRGB(220, 140, 60));
        auditionFlashUntilMs = juce::Time::currentTimeMillis() + 180;
        auditionFlashActive  = true;
    }

    void MasterSection::timerCallback()
    {
        // Audition flash deadline — restore the button colour once
        // the flash window has elapsed.
        if (auditionFlashActive
            && juce::Time::currentTimeMillis() >= auditionFlashUntilMs)
        {
            auditionButton.removeColour(juce::TextButton::buttonColourId);
            auditionFlashActive = false;
        }

        // Modulation-glow halo on the (selected lane's) gain knob. Two
        // sources contribute: matrix LFO routings to VoiceGain and the
        // amp envelope itself (which always modulates gain over the
        // course of a note). Combined via max so a routed LFO still
        // shows through during the sustain plateau. The amp env value
        // is 0..1 — when no note plays the env is idle at zero, so
        // the glow only appears while a beep is actually sounding,
        // which is what makes the gain knob visibly track the note.
        {
            const float lfo1 = processor.getSelectedLaneLfoValue(0);
            const float lfo2 = processor.getSelectedLaneLfoValue(1);
            const float matrix = ModulationGlow::computeMatrixIntensity(
                processor.getApvts(), processor.getSelectedLane(),
                ModDestination::VoiceGain, lfo1, lfo2);
            const float ampEnv = juce::jlimit(0.0f, 1.0f,
                processor.getSelectedLaneAmpEnvValue());
            gainSlider.setModulationIntensity(std::max(matrix, ampEnv));
        }

        // Pull the latest output peak; only repaint the meter when something
        // it draws actually changed, so an idle app doesn't tax the GPU 30
        // times a second.
        const float newLevel = processor.getOutputPeak();
        const auto  nowMs    = juce::Time::currentTimeMillis();
        bool meterDirty = false;

        if (std::abs(newLevel - meterLevel) > 1.0f / 256.0f)
        {
            meterLevel = newLevel;
            meterDirty = true;
        }

        // Peak-hold (P18): snap up instantly, hold ~1.5 s, then ease down
        // toward the live level (~0.6/s at 30 Hz) so transients stay legible.
        if (newLevel >= peakHoldLevel)
        {
            peakHoldLevel   = newLevel;
            peakHoldUntilMs = nowMs + 1500;
            meterDirty      = true;
        }
        else if (nowMs >= peakHoldUntilMs && peakHoldLevel > newLevel)
        {
            peakHoldLevel = juce::jmax(newLevel, peakHoldLevel - 0.02f);
            meterDirty    = true;
        }

        // Clip latch (P18): light the red cap whenever the output reaches
        // full scale, and hold it ~1.5 s so a brief overload isn't missed.
        if (newLevel >= 0.999f)
        {
            clipLatched      = true;
            clipLatchUntilMs = nowMs + 1500;
            meterDirty       = true;
        }
        else if (clipLatched && nowMs >= clipLatchUntilMs)
        {
            clipLatched = false;
            meterDirty  = true;
        }

        if (meterDirty)
            repaint(meterBounds);

        // Undo/Redo enable state — undo history changes silently
        // when other surfaces (sliders, menu undo) write to the
        // UndoManager. Cheap to refresh; piggybacks on the 30 Hz
        // tick instead of opening a separate timer.
        refreshUndoButtonStates();
    }

    void MasterSection::paint(juce::Graphics& g)
    {
        Section::paint(g);

        if (meterBounds.isEmpty())
            return;

        const auto bounds = meterBounds.toFloat();
        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);

        // Filled width = meterLevel (0..1). Green up to 0.7, amber 0.7..0.9,
        // red above.
        const float clamped = juce::jlimit(0.0f, 1.0f, meterLevel);
        const auto  track   = bounds.reduced(1.5f);

        if (clamped > 0.0f)
        {
            auto fill = track;
            fill.setWidth(fill.getWidth() * clamped);

            auto barColour = juce::Colour::fromRGB(60, 180, 80);
            if (clamped > 0.9f)      barColour = juce::Colour::fromRGB(220,  60,  60);
            else if (clamped > 0.7f) barColour = juce::Colour::fromRGB(220, 180,  60);

            g.setColour(barColour);
            g.fillRoundedRectangle(fill, 1.5f);
        }

        // Peak-hold tick (P18) — bright vertical marker at the recent max.
        const float ph = juce::jlimit(0.0f, 1.0f, peakHoldLevel);
        if (ph > 0.0f)
        {
            const float x = track.getX() + track.getWidth() * ph;
            g.setColour(juce::Colour::fromRGB(235, 235, 245));
            g.fillRect(juce::Rectangle<float>(x - 0.75f, track.getY(), 1.5f, track.getHeight()));
        }

        // Clip cap (P18) — latched red block at the right end after 0 dBFS.
        if (clipLatched)
        {
            g.setColour(juce::Colour::fromRGB(255, 70, 70));
            g.fillRoundedRectangle(
                juce::Rectangle<float>(bounds.getRight() - 4.5f, bounds.getY(),
                                       4.5f, bounds.getHeight()).reduced(0.5f),
                1.0f);
        }

        // dBFS scale ruler (P18) — ticks + labels at -12 / -6 / -3 / 0 dBFS,
        // placed on the meter's linear-amplitude axis (level = gain for that
        // dB). Shares the meter's `track` inset so ticks line up with the bar.
        if (! meterScaleBounds.isEmpty())
        {
            const auto scale = meterScaleBounds.toFloat();
            g.setFont(juce::FontOptions(8.5f));

            struct DbMark { int db; const char* label; };
            for (const auto& mark : { DbMark { -12, "-12" }, DbMark { -6, "-6" },
                                      DbMark { -3, "-3" }, DbMark { 0, "0" } })
            {
                const float level = juce::Decibels::decibelsToGain((float) mark.db);
                const float x     = track.getX() + track.getWidth() * level;

                g.setColour(juce::Colour::fromRGB(110, 110, 120));
                g.fillRect(juce::Rectangle<float>(x - 0.5f, scale.getY(), 1.0f, 3.0f));

                // Centre the label under its tick, clamped so the right-most
                // "0" doesn't clip off the meter's edge.
                constexpr float textW = 22.0f;
                const float textX = juce::jlimit(scale.getX(),
                                                 scale.getRight() - textW,
                                                 x - textW * 0.5f);
                g.setColour(juce::Colour::fromRGB(150, 150, 160));
                g.drawText(mark.label,
                           juce::Rectangle<float>(textX, scale.getY() + 2.0f,
                                                  textW, scale.getHeight() - 2.0f),
                           juce::Justification::centred, false);
            }
        }
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kSliderWidth  = 140;
        constexpr int kButtonHeight = 28;
        constexpr int kMeterHeight  = 9;    // taller so the dB ticks read (P18)
        constexpr int kScaleHeight  = 10;   // dBFS ruler strip below the meter
        constexpr int kButtonGap    = 6;
        constexpr int kRowGap       = 6;
        constexpr int kAbRowHeight  = 22;
        constexpr int kAbButtonW    = 28;
        constexpr int kAbCopyW      = 48;
        constexpr int kAbGap        = 4;

        // ---- Top strip: Undo / Redo (left) + A/B (right) ----------
        auto abRow = bounds.removeFromTop(kAbRowHeight);
        bounds.removeFromTop(kRowGap);

        // Right side: A | B | Copy
        abCopyButton.setBounds(abRow.removeFromRight(kAbCopyW));
        abRow.removeFromRight(kAbGap);
        abButtonB.setBounds(abRow.removeFromRight(kAbButtonW));
        abRow.removeFromRight(kAbGap);
        abButtonA.setBounds(abRow.removeFromRight(kAbButtonW));

        // Left side: Undo | Redo
        constexpr int kUndoW = 48;
        undoButton.setBounds(abRow.removeFromLeft(kUndoW));
        abRow.removeFromLeft(kAbGap);
        redoButton.setBounds(abRow.removeFromLeft(kUndoW));

        // Middle: < [preset name] >  (P23). Uses whatever width is left
        // between the Undo/Redo and A/B clusters; the name label ellipsises
        // when the section is narrow.
        constexpr int kPresetArrowW = 22;
        abRow.removeFromLeft(kAbGap);
        abRow.removeFromRight(kAbGap);
        prevPresetButton.setBounds(abRow.removeFromLeft(kPresetArrowW));
        nextPresetButton.setBounds(abRow.removeFromRight(kPresetArrowW));
        presetNameLabel.setBounds(abRow);

        auto buttonRow = bounds.removeFromBottom(kButtonHeight);
        bounds.removeFromBottom(kRowGap);

        // Output level meter + dBFS ruler — thin horizontal strips just
        // above the button row, full content width. The scale sits directly
        // under the meter; both are carved from the bottom so the gain knob
        // takes whatever height remains (P18).
        meterScaleBounds = bounds.removeFromBottom(kScaleHeight);
        meterBounds      = bounds.removeFromBottom(kMeterHeight);
        bounds.removeFromBottom(kRowGap);

        const int xSlider = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(xSlider, bounds.getY(), kSliderWidth, bounds.getHeight());

        const int cellWidth = (buttonRow.getWidth() - kButtonGap) / 2;
        diceAllButton.setBounds(buttonRow.removeFromLeft(cellWidth));
        buttonRow.removeFromLeft(kButtonGap);
        auditionButton.setBounds(buttonRow);
    }

    void MasterSection::refreshAbButtonStates()
    {
        const char active = processor.getActiveAbSlot();
        abButtonA.setToggleState(active == 'A', juce::dontSendNotification);
        abButtonB.setToggleState(active == 'B', juce::dontSendNotification);
    }

    void MasterSection::refreshUndoButtonStates()
    {
        undoButton.setEnabled(processor.getUndoManager().canUndo());
        redoButton.setEnabled(processor.getUndoManager().canRedo());
    }

    void MasterSection::setPresetName(const juce::String& name)
    {
        // ASCII placeholder when not on a preset — keeps clear of the
        // non-ASCII String paint assertion seen elsewhere in the UI.
        presetNameLabel.setText(name.isNotEmpty() ? name : juce::String("(no preset)"),
                                juce::dontSendNotification);
    }
}
