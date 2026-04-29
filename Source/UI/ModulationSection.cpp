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

        retargetLane(processor.getSelectedLane());
    }

    void ModulationSection::retargetLane(int lane)
    {
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
        }

        setTitleSuffix(processor.laneTitleSuffix(lane));
        setAccentColour(processor.laneAccentColour(lane));
    }

    void ModulationSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kLfoRowHeight  = 70;
        constexpr int kRowGap        = 6;
        constexpr int kSlotRowHeight = 24;
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

        // ---- Matrix rows --------------------------------------------
        for (int i = 0; i < kNumModSlots; ++i)
        {
            auto row = bounds.removeFromTop(kSlotRowHeight);
            bounds.removeFromTop(2);

            auto& slot = slotControls[static_cast<size_t>(i)];

            constexpr int kLabelWidth  = 48;
            constexpr int kSourceWidth = 70;
            constexpr int kDestWidth   = 140;
            constexpr int kSmallGap    = 4;

            slot.label.setBounds(row.removeFromLeft(kLabelWidth));
            row.removeFromLeft(kSmallGap);
            slot.source.setBounds(row.removeFromLeft(kSourceWidth));
            row.removeFromLeft(kSmallGap);
            slot.dest.setBounds(row.removeFromLeft(kDestWidth));
            row.removeFromLeft(kSmallGap);
            slot.amount.setBounds(row);
        }
    }
}
