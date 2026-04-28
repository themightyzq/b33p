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

        // Pull the latest output peak; only repaint when it actually
        // changes by more than 1 LSB so an idle app doesn't tax the
        // GPU 30 times a second.
        const float newLevel = processor.getOutputPeak();
        if (std::abs(newLevel - meterLevel) > 1.0f / 256.0f)
        {
            meterLevel = newLevel;
            repaint(meterBounds);
        }
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

        // Filled width = meterLevel (0..1). Green up to 0.7,
        // amber 0.7..0.9, red above. Anything above 1.0 is
        // already clipped — show the bar fully red at the top.
        const float clamped = juce::jlimit(0.0f, 1.0f, meterLevel);
        if (clamped <= 0.0f) return;

        auto fill = bounds.reduced(1.5f);
        fill.setWidth(fill.getWidth() * clamped);

        auto barColour = juce::Colour::fromRGB(60, 180, 80);
        if (clamped > 0.9f)      barColour = juce::Colour::fromRGB(220,  60,  60);
        else if (clamped > 0.7f) barColour = juce::Colour::fromRGB(220, 180,  60);

        g.setColour(barColour);
        g.fillRoundedRectangle(fill, 1.5f);
    }

    void MasterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kSliderWidth  = 140;
        constexpr int kButtonHeight = 28;
        constexpr int kMeterHeight  = 6;
        constexpr int kButtonGap    = 6;
        constexpr int kRowGap       = 6;

        auto buttonRow = bounds.removeFromBottom(kButtonHeight);
        bounds.removeFromBottom(kRowGap);

        // Output level meter — thin horizontal strip just above the
        // button row, full content width.
        meterBounds = bounds.removeFromBottom(kMeterHeight);
        bounds.removeFromBottom(kRowGap);

        const int xSlider = bounds.getX() + (bounds.getWidth() - kSliderWidth) / 2;
        gainSlider.setBounds(xSlider, bounds.getY(), kSliderWidth, bounds.getHeight());

        const int cellWidth = (buttonRow.getWidth() - kButtonGap) / 2;
        diceAllButton.setBounds(buttonRow.removeFromLeft(cellWidth));
        buttonRow.removeFromLeft(kButtonGap);
        auditionButton.setBounds(buttonRow);
    }
}
