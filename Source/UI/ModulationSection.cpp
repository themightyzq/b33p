#include "ModulationSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    namespace
    {
        const juce::StringArray kShapeNames { "Sine", "Triangle", "Saw", "Square" };
        const juce::StringArray kSourceNames { "None", "LFO 1", "LFO 2" };
        const juce::StringArray kDestNames {
            "None",
            "Osc Pitch",
            "Wavetable Morph",
            "FM Depth",
            "Ring Mix",
            "Filter Cutoff",
            "Filter Resonance",
            "Distortion Drive",
            "Mod FX P1",
            "Mod FX P2",
            "Mod FX Mix",
            "Voice Gain"
        };
    }

    ModulationSection::ModulationSection(B33pProcessor& processorRef)
        : Section("Modulation"),
          processor(processorRef)
    {
        for (int i = 0; i < kNumLfosPerLane; ++i)
        {
            auto& lfo = lfoControls[static_cast<size_t>(i)];
            lfo.shape.addItemList(kShapeNames, 1);
            addAndMakeVisible(lfo.shape);
            addAndMakeVisible(lfo.rate);
            lfo.shape.setTooltip("LFO " + juce::String(i + 1) + " waveform shape");
            lfo.rate .setTooltip("LFO " + juce::String(i + 1) + " rate (0..30 Hz)");
        }

        for (int i = 0; i < kNumModSlots; ++i)
        {
            auto& slot = slotControls[static_cast<size_t>(i)];
            slot.label.setText("Slot " + juce::String(i + 1),
                                juce::dontSendNotification);
            slot.label.setJustificationType(juce::Justification::centredLeft);
            slot.label.setFont(juce::FontOptions(11.0f));
            addAndMakeVisible(slot.label);

            slot.source.addItemList(kSourceNames, 1);
            slot.dest  .addItemList(kDestNames,   1);
            addAndMakeVisible(slot.source);
            addAndMakeVisible(slot.dest);

            slot.amount.setSliderStyle(juce::Slider::LinearHorizontal);
            slot.amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 16);
            slot.amount.setRange(-1.0, 1.0, 0.01);
            slot.amount.setDoubleClickReturnValue(true, 0.0);
            slot.amount.setPopupDisplayEnabled(true, false, this);
            addAndMakeVisible(slot.amount);

            slot.source.setTooltip("Modulation source for slot " + juce::String(i + 1));
            slot.dest  .setTooltip("Modulation destination for slot " + juce::String(i + 1));
            slot.amount.setTooltip("Modulation amount: -1 = full inverse, 0 = off, +1 = full positive");
        }

        hintLabel.setText(
            "Route a Source (LFO 1 or LFO 2) to a Destination, then dial the Amount to start modulating.",
            juce::dontSendNotification);
        hintLabel.setJustificationType(juce::Justification::centred);
        hintLabel.setFont(juce::FontOptions(10.5f).withStyle("Italic"));
        hintLabel.setColour(juce::Label::textColourId,
                             juce::Colour::fromRGB(120, 120, 120));
        hintLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(hintLabel);

        retargetLane(processor.getSelectedLane());

        // Drive the live mod-activity indicators (P14). 30 Hz matches the
        // master meter; the repainted region is just the thin indicator
        // strip on the left of the matrix rows.
        startTimerHz(30);
    }

    void ModulationSection::retargetLane(int lane)
    {
        currentLane = lane;

        for (int i = 0; i < kNumLfosPerLane; ++i)
        {
            auto& lfo = lfoControls[static_cast<size_t>(i)];
            lfo.shapeAttachment.reset();
            lfo.rateAttachment.reset();

            lfo.shapeAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.getApvts(), ParameterIDs::lfoShape(lane, i), lfo.shape);
            lfo.rateAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getApvts(), ParameterIDs::lfoRateHz(lane, i),
                    lfo.rate.getSlider());

            lfo.rate.attachRandomizer(processor, ParameterIDs::lfoRateHz(lane, i));

            SliderFormatting::applyHz(lfo.rate.getSlider());
            SliderFormatting::applyDoubleClickReset(lfo.rate.getSlider(),
                                                    processor.getApvts(),
                                                    ParameterIDs::lfoRateHz(lane, i));
        }

        for (int i = 0; i < kNumModSlots; ++i)
        {
            auto& slot = slotControls[static_cast<size_t>(i)];
            slot.sourceAttachment.reset();
            slot.destAttachment.reset();
            slot.amountAttachment.reset();

            slot.sourceAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.getApvts(), ParameterIDs::modSlotSource(lane, i), slot.source);
            slot.destAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.getApvts(), ParameterIDs::modSlotDest(lane, i), slot.dest);
            slot.amountAttachment = std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    processor.getApvts(), ParameterIDs::modSlotAmount(lane, i), slot.amount);
            // After the attachment — it installs the param's own textFromValue
            // (full float precision); override it for a clean bipolar readout
            // ("+0.29" / "-1.00") (REVIEW-DESIGN).
            SliderFormatting::applyBipolar(slot.amount);
        }

        setAccentColour(processor.laneAccentColour(lane));
    }

    void ModulationSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kLfoRowHeight  = 56;   // leaves room for all 4 matrix rows
        constexpr int kRowGap        = 6;
        constexpr int kInnerGap      = 6;

        // ---- LFO row ------------------------------------------------
        auto lfoRow = bounds.removeFromTop(kLfoRowHeight);
        bounds.removeFromTop(kRowGap);

        const int lfoCellWidth = (lfoRow.getWidth() - kInnerGap) / kNumLfosPerLane;
        for (int i = 0; i < kNumLfosPerLane; ++i)
        {
            auto cell = (i == kNumLfosPerLane - 1)
                            ? lfoRow
                            : lfoRow.removeFromLeft(lfoCellWidth);
            if (i < kNumLfosPerLane - 1)
                lfoRow.removeFromLeft(kInnerGap);

            // Shape combo on top, rate slider below.
            auto& lfo = lfoControls[static_cast<size_t>(i)];
            auto shapeRow = cell.removeFromTop(22);
            cell.removeFromTop(4);
            lfo.shape.setBounds(shapeRow);
            lfo.rate .setBounds(cell);
        }

        // ---- Empty-state / orientation hint -------------------------
        // Slim italic line above the matrix rows so a first-time user
        // sees a starting point. Always visible — quiet enough that a
        // power user with the section in full flight reads past it.
        constexpr int kHintHeight = 16;
        hintLabel.setBounds(bounds.removeFromTop(kHintHeight));
        bounds.removeFromTop(kRowGap);

        // ---- Matrix rows --------------------------------------------
        // Divide the remaining height evenly across the slots so all four
        // always fit inside the (column-packed, short) section — fixed row
        // heights previously overflowed and clipped slot 4.
        constexpr int kSlotGap = 2;
        const int slotRowHeight = juce::jmax(
            16, (bounds.getHeight() - (kNumModSlots - 1) * kSlotGap) / kNumModSlots);
        for (int i = 0; i < kNumModSlots; ++i)
        {
            auto row = bounds.removeFromTop(slotRowHeight);
            if (i < kNumModSlots - 1)
                bounds.removeFromTop(kSlotGap);

            auto& slot = slotControls[static_cast<size_t>(i)];

            constexpr int kIndicatorWidth = 5;
            constexpr int kIndicatorGap   = 5;
            constexpr int kLabelWidth  = 48;
            constexpr int kSourceWidth = 70;
            constexpr int kDestWidth   = 140;
            constexpr int kSmallGap    = 4;

            // Live mod-activity indicator (P14): a thin strip at the row's
            // left edge, vertically centred, painted in paint().
            auto indicatorCol = row.removeFromLeft(kIndicatorWidth);
            slotIndicatorBounds[static_cast<size_t>(i)] =
                indicatorCol.withSizeKeepingCentre(kIndicatorWidth, juce::jmax(6, slotRowHeight - 8));
            row.removeFromLeft(kIndicatorGap);

            slot.label.setBounds(row.removeFromLeft(kLabelWidth));
            row.removeFromLeft(kSmallGap);
            slot.source.setBounds(row.removeFromLeft(kSourceWidth));
            row.removeFromLeft(kSmallGap);
            slot.dest.setBounds(row.removeFromLeft(kDestWidth));
            row.removeFromLeft(kSmallGap);
            slot.amount.setBounds(row);
        }
    }

    float ModulationSection::slotActivity(int slot) const
    {
        auto& apvts = processor.getApvts();
        const int source = static_cast<int>(
            apvts.getRawParameterValue(ParameterIDs::modSlotSource(currentLane, slot))->load());
        const int dest = static_cast<int>(
            apvts.getRawParameterValue(ParameterIDs::modSlotDest(currentLane, slot))->load());

        // Source "None" (0) or destination "None" (0) → routing is idle.
        if (source <= 0 || dest <= 0)
            return -1.0f;

        // Accessibility: routed but no audio playing → return 0 so the
        // indicator renders as a steady, minimum-brightness fill (the
        // user can still SEE the slot is wired up at a glance — paint
        // draws the accent frame + min-alpha fill for activity >= 0 —
        // but nothing animates while the synth is silent.
        if (! processor.isSelectedLaneVoiceActive())
            return 0.0f;

        const float amount = apvts.getRawParameterValue(
            ParameterIDs::modSlotAmount(currentLane, slot))->load();
        const float lfo = processor.getSelectedLaneLfoValue(source - 1);   // 1=LFO1→0, 2=LFO2→1
        return juce::jlimit(0.0f, 1.0f, std::abs(lfo * amount));
    }

    void ModulationSection::timerCallback()
    {
        // Accessibility: only repaint indicator strips while audio is
        // being produced. Repaint once more on the playing→idle edge so
        // the strips settle to their static configured-but-idle state
        // (otherwise they'd freeze at whatever pulse value was on at
        // the moment of transition and read as "still doing something").
        const bool playing = processor.isSelectedLaneVoiceActive();
        if (playing || wasPlayingLastTick)
            for (const auto& r : slotIndicatorBounds)
                repaint(r);
        wasPlayingLastTick = playing;
    }

    void ModulationSection::paint(juce::Graphics& g)
    {
        Section::paint(g);

        const auto accent = processor.laneAccentColour(currentLane);

        for (int i = 0; i < kNumModSlots; ++i)
        {
            const auto r = slotIndicatorBounds[static_cast<size_t>(i)].toFloat();
            if (r.isEmpty())
                continue;

            const float activity = slotActivity(i);
            if (activity < 0.0f)
            {
                // Not routed — faint hollow tick so the column reads "idle here".
                g.setColour(juce::Colour::fromRGB(60, 60, 64));
                g.drawRoundedRectangle(r.reduced(1.0f), 1.5f, 1.0f);
                continue;
            }

            // Routed — a steady accent frame plus a fill whose brightness
            // tracks the live |LFO × amount|, so the slot visibly pulses
            // while it modulates (and sits faint when amount/LFO are at 0).
            g.setColour(accent.withAlpha(0.30f));
            g.drawRoundedRectangle(r.reduced(0.5f), 1.5f, 1.0f);

            g.setColour(accent.withAlpha(juce::jlimit(0.15f, 1.0f, activity)));
            g.fillRoundedRectangle(r.reduced(1.5f), 1.0f);
        }
    }
}
