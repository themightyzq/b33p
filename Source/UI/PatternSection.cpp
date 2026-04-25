#include "PatternSection.h"

namespace B33p
{
    namespace
    {
        constexpr int  kControlsHeight = 28;
        constexpr int  kControlsGap    = 6;
        constexpr int  kButtonWidth    = 80;
        constexpr int  kRepaintHz      = 30;
    }

    PatternSection::PatternSection(B33pProcessor& processorRef)
        : Section("Pattern"),
          processor(processorRef),
          grid(processorRef)
    {
        addAndMakeVisible(grid);

        playButton.onClick = [this]
        {
            if (processor.isPlaying())
                processor.stopPlayback();
            else
                processor.startPlayback();
        };
        addAndMakeVisible(playButton);

        loopToggle.setClickingTogglesState(true);
        loopToggle.setToggleState(processor.getLooping(), juce::dontSendNotification);
        loopToggle.onClick = [this]
        {
            processor.setLooping(loopToggle.getToggleState());
        };
        addAndMakeVisible(loopToggle);

        startTimerHz(kRepaintHz);
    }

    PatternSection::~PatternSection()
    {
        stopTimer();
    }

    void PatternSection::timerCallback()
    {
        // Sync button text in case playback stopped on its own
        // (non-looped pattern reached the end).
        const bool playing = processor.isPlaying();
        playButton.setButtonText(playing ? "Stop" : "Play");

        if (playing)
            grid.repaint();
    }

    void PatternSection::resized()
    {
        auto bounds = getContentBounds();

        auto controlsRow = bounds.removeFromTop(kControlsHeight);
        bounds.removeFromTop(kControlsGap);

        playButton.setBounds(controlsRow.removeFromLeft(kButtonWidth));
        controlsRow.removeFromLeft(kControlsGap);
        loopToggle.setBounds(controlsRow.removeFromLeft(kButtonWidth));

        grid.setBounds(bounds);
    }
}
