#pragma once

#include "LabeledSlider.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace B33p
{
    class MasterSection : public Section
                        , private juce::Timer
    {
    public:
        explicit MasterSection(B33pProcessor& processor);

        void resized() override;
        void paint(juce::Graphics& g) override;

        void retargetLane(int lane);

    private:
        void timerCallback() override;
        void flashAuditionButton();

        B33pProcessor& processor;

        // Gain explicitly opts out of the randomizer — too easy
        // to blast the user with a random +20 dB jump otherwise.
        LabeledSlider    gainSlider     { "Gain", /*showRandomizer=*/ false };
        juce::TextButton auditionButton { "Audition" };
        juce::TextButton diceAllButton  { "Dice All" };

        // A/B compare buttons (top-right of the section). Active
        // slot highlights via setToggleState; copy button mirrors
        // the active side's state into the other.
        juce::TextButton abButtonA       { "A" };
        juce::TextButton abButtonB       { "B" };
        juce::TextButton abCopyButton    { "Copy" };

        // Undo / Redo (top-left of the section). The plugin needs its
        // own buttons because DAWs capture Cmd+Z system-wide and the
        // Edit-menu route is a slow click-path. Enable / disable
        // state is refreshed by the existing 30 Hz timer.
        juce::TextButton undoButton      { "Undo" };
        juce::TextButton redoButton      { "Redo" };

        void refreshAbButtonStates();
        void refreshUndoButtonStates();

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

        // Output level meter — 30 Hz timer-driven update of
        // meterLevel from processor.getOutputPeak(); meterBounds is
        // computed in resized() so paint() knows where to draw.
        float                meterLevel { 0.0f };
        juce::Rectangle<int> meterBounds;

        // Meter feedback (REVIEW.md P18). Peak-hold draws a bright tick at
        // the recent maximum, decaying after ~1.5 s so transients stay
        // readable. The clip latch lights a red cap at the meter's right end
        // for ~1.5 s whenever the output reaches 0 dBFS, so a brief overload
        // can't be missed between timer frames.
        float       peakHoldLevel    { 0.0f };
        juce::int64 peakHoldUntilMs  { 0 };
        bool        clipLatched      { false };
        juce::int64 clipLatchUntilMs { 0 };

        // Audition flash deadline. The timer is now continuous (so
        // it can drive the meter); the flash uses a deadline rather
        // than start/stop so both purposes share one timer.
        juce::int64 auditionFlashUntilMs { 0 };
        bool        auditionFlashActive  { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSection)
    };
}
