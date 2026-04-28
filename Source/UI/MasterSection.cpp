#include "MasterSection.h"

#include "Core/ParameterIDs.h"
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

        // "Randomize Lane" rolls every unlocked parameter for the
        // currently-selected lane only. Rolling all four lanes at
        // once lives in the pattern editor + the Lane menu so the
        // more frequent single-lane action stays in the master strip.
        diceAllButton.setButtonText("Randomize Lane");
        diceAllButton.onClick = [this]
        {
            juce::Random rng;
            const int lane = processor.getSelectedLane();
            processor.getRandomizer().rollMany(
                ParameterIDs::allForLane(lane), rng,
                "Randomize Lane " + juce::String(lane + 1));
        };
        addAndMakeVisible(diceAllButton);

        gainSlider    .setTooltip("Master output level");
        auditionButton.setTooltip("Play a single beep with the current settings (Shift+Space)");
        diceAllButton .setTooltip("Randomize every unlocked parameter on the currently-selected lane");

        retargetLane(processor.getSelectedLane());
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
        SliderFormatting::applyDecimal(gainSlider.getSlider(), 2);
        SliderFormatting::applyDoubleClickReset(gainSlider.getSlider(),
                                                processor.getApvts(),
                                                ParameterIDs::voiceGain(lane));

        setTitleSuffix(processor.laneTitleSuffix(lane));
    }

    void MasterSection::flashAuditionButton()
    {
        // Brief amber tint so the user gets a visual confirmation
        // that the click registered — the audition is a fire-and-
        // forget half-second beep with no other on-screen feedback.
        auditionButton.setColour(juce::TextButton::buttonColourId,
                                  juce::Colour::fromRGB(220, 140, 60));
        startTimer(180);
    }

    void MasterSection::timerCallback()
    {
        auditionButton.removeColour(juce::TextButton::buttonColourId);
        stopTimer();
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kSliderWidth  = 140;
        constexpr int kButtonHeight = 28;
        constexpr int kButtonGap    = 6;
        constexpr int kRowGap       = 6;

        auto buttonRow = bounds.removeFromBottom(kButtonHeight);
        bounds.removeFromBottom(kRowGap);

        const int xSlider = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(xSlider, bounds.getY(), kSliderWidth, bounds.getHeight());

        const int cellWidth = (buttonRow.getWidth() - kButtonGap) / 2;
        diceAllButton.setBounds(buttonRow.removeFromLeft(cellWidth));
        buttonRow.removeFromLeft(kButtonGap);
        auditionButton.setBounds(buttonRow);
    }
}
