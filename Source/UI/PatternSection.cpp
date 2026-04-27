#include "PatternSection.h"

#include "ExportTask.h"

#include <array>

namespace B33p
{
    namespace
    {
        constexpr int  kControlsHeight = 28;
        constexpr int  kControlsGap    = 6;
        constexpr int  kButtonWidth    = 80;
        constexpr int  kExportWidth    = 90;
        constexpr int  kComboWidth     = 110;
        constexpr int  kLabelWidth     = 50;
        constexpr int  kTimeWidth      = 90;
        constexpr int  kRepaintHz      = 30;

        struct LengthPreset { const char* label; double seconds; };
        constexpr std::array<LengthPreset, 6> kLengthPresets {{
            { "0.5 s", 0.5 },
            { "1.0 s", 1.0 },
            { "2.0 s", 2.0 },
            { "3.0 s", 3.0 },
            { "5.0 s", 5.0 },
            { "10.0 s", 10.0 },
        }};

        struct GridPreset { const char* label; double seconds; };
        constexpr std::array<GridPreset, 8> kGridPresets {{
            { "10 ms",   0.010 },
            { "25 ms",   0.025 },
            { "50 ms",   0.050 },
            { "100 ms",  0.100 },
            { "250 ms",  0.250 },
            { "500 ms",  0.500 },
            { "1000 ms", 1.000 },
            { "Off",     0.000 },
        }};

        // Item IDs are 1-based for juce::ComboBox so 0 means
        // "no selection". Map 1..N to indices.
        int idForIndex(int i) { return i + 1; }
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
        // Initial colour matches the "ready to play" state so the
        // button doesn't flash default-grey for the first 33 ms
        // before the timer's first tick.
        playButton.setColour(juce::TextButton::buttonColourId,
                             juce::Colour::fromRGB(60, 140, 70));
        addAndMakeVisible(playButton);

        loopToggle.setClickingTogglesState(true);
        loopToggle.setToggleState(processor.getLooping(), juce::dontSendNotification);
        loopToggle.onClick = [this]
        {
            processor.setLooping(loopToggle.getToggleState());
        };
        addAndMakeVisible(loopToggle);

        // Playhead readout. Updates from the same 30 Hz timer as
        // the grid; shows "0.00 / 5.00s" at rest so the user can
        // see the pattern's total length without playing it.
        timeLabel.setJustificationType(juce::Justification::centredLeft);
        timeLabel.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        timeLabel.setColour(juce::Label::textColourId,
                             juce::Colour::fromRGB(170, 170, 170));
        addAndMakeVisible(timeLabel);

        // Length combo
        lengthLabel.setText("Length:", juce::dontSendNotification);
        lengthLabel.setJustificationType(juce::Justification::centredRight);
        lengthLabel.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(lengthLabel);

        for (size_t i = 0; i < kLengthPresets.size(); ++i)
            lengthCombo.addItem(kLengthPresets[i].label, idForIndex(static_cast<int>(i)));
        syncLengthComboToPattern();
        lengthCombo.onChange = [this] { onLengthChanged(); };
        addAndMakeVisible(lengthCombo);

        // Grid combo
        gridLabel.setText("Grid:", juce::dontSendNotification);
        gridLabel.setJustificationType(juce::Justification::centredRight);
        gridLabel.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(gridLabel);

        for (size_t i = 0; i < kGridPresets.size(); ++i)
            gridCombo.addItem(kGridPresets[i].label, idForIndex(static_cast<int>(i)));
        {
            const double current = grid.getGridSeconds();
            int bestId = idForIndex(0);
            double bestDist = 1e30;
            for (size_t i = 0; i < kGridPresets.size(); ++i)
            {
                const double d = std::abs(kGridPresets[i].seconds - current);
                if (d < bestDist) { bestDist = d; bestId = idForIndex(static_cast<int>(i)); }
            }
            gridCombo.setSelectedId(bestId, juce::dontSendNotification);
        }
        gridCombo.onChange = [this] { onGridChanged(); };
        addAndMakeVisible(gridCombo);

        // Export button (right-aligned to separate the action from
        // playback / editing controls).
        exportButton.onClick = [this] { onExportClicked(); };
        addAndMakeVisible(exportButton);

        playButton  .setTooltip("Play the pattern from the start");
        loopToggle  .setTooltip("Loop playback when the end is reached");
        lengthCombo .setTooltip("Total length of the pattern");
        gridCombo   .setTooltip("Snap event positions to this grid (Off = free)");
        exportButton.setTooltip("Render the pattern to a WAV file");

        startTimerHz(kRepaintHz);
    }

    PatternSection::~PatternSection()
    {
        stopTimer();
    }

    void PatternSection::syncLengthComboToPattern()
    {
        // Pick the preset closest to the current pattern length so the
        // dropdown reflects reality. Used at construction and after a
        // bulk state replacement (Open / New).
        const double current = processor.getPattern().getLengthSeconds();
        int    bestId   = idForIndex(0);
        double bestDist = 1e30;
        for (size_t i = 0; i < kLengthPresets.size(); ++i)
        {
            const double d = std::abs(kLengthPresets[i].seconds - current);
            if (d < bestDist) { bestDist = d; bestId = idForIndex(static_cast<int>(i)); }
        }
        lengthCombo.setSelectedId(bestId, juce::dontSendNotification);
    }

    void PatternSection::refreshFromState()
    {
        syncLengthComboToPattern();
        loopToggle.setToggleState(processor.getLooping(),
                                   juce::dontSendNotification);
        grid.repaint();
    }

    void PatternSection::onLengthChanged()
    {
        const int id = lengthCombo.getSelectedId();
        if (id <= 0) return;
        const size_t idx = static_cast<size_t>(id - 1);
        if (idx >= kLengthPresets.size()) return;
        processor.getPattern().setLengthSeconds(kLengthPresets[idx].seconds);
        processor.markDirty();
        grid.repaint();
    }

    void PatternSection::onGridChanged()
    {
        const int id = gridCombo.getSelectedId();
        if (id <= 0) return;
        const size_t idx = static_cast<size_t>(id - 1);
        if (idx >= kGridPresets.size()) return;
        grid.setGridSeconds(kGridPresets[idx].seconds);
    }

    void PatternSection::onExportClicked()
    {
        ExportDialog::showAsync(this, [this](ExportDialog::Result result)
        {
            if (! result.accepted)
                return;

            // Defer the actual launch to the message thread so the
            // dialog has fully closed before the export-progress
            // window opens — avoids overlapping modal stacks.
            juce::MessageManager::callAsync([this, r = std::move(result)]
            {
                runExport(r);
            });
        });
    }

    void PatternSection::runExport(ExportDialog::Result settings)
    {
        // ExportTask is heap-allocated and self-deletes in
        // threadComplete after showing its own success/failure
        // alert — fire-and-forget from this side.
        ExportTask::launchAsync(processor, std::move(settings));
    }

    void PatternSection::timerCallback()
    {
        const bool playing = processor.isPlaying();

        // Recolour even when the text is unchanged — JUCE's
        // setButtonText skips the repaint if the string matches,
        // and the colour change is the visual cue we care about.
        const auto stopRed   = juce::Colour::fromRGB(190,  60,  60);
        const auto playGreen = juce::Colour::fromRGB( 60, 140,  70);

        playButton.setButtonText(playing ? "Stop" : "Play");
        playButton.setColour(juce::TextButton::buttonColourId,
                             playing ? stopRed : playGreen);

        const double headSec   = playing ? processor.getPlayheadSeconds() : 0.0;
        const double lengthSec = processor.getPattern().getLengthSeconds();
        timeLabel.setText(juce::String(headSec,   2) + " / "
                        + juce::String(lengthSec, 2) + "s",
                          juce::dontSendNotification);

        if (playing)
            grid.repaint();
    }

    void PatternSection::resized()
    {
        auto bounds = getContentBounds();

        auto controlsRow = bounds.removeFromTop(kControlsHeight);
        bounds.removeFromTop(kControlsGap);

        // Right-align Export to separate the action from the
        // play/edit controls on the left.
        exportButton.setBounds(controlsRow.removeFromRight(kExportWidth));
        controlsRow.removeFromRight(kControlsGap);

        playButton.setBounds(controlsRow.removeFromLeft(kButtonWidth));
        controlsRow.removeFromLeft(kControlsGap);
        loopToggle.setBounds(controlsRow.removeFromLeft(kButtonWidth));
        controlsRow.removeFromLeft(kControlsGap);

        timeLabel.setBounds(controlsRow.removeFromLeft(kTimeWidth));
        controlsRow.removeFromLeft(kControlsGap);

        lengthLabel.setBounds(controlsRow.removeFromLeft(kLabelWidth));
        lengthCombo.setBounds(controlsRow.removeFromLeft(kComboWidth));
        controlsRow.removeFromLeft(kControlsGap);

        gridLabel.setBounds(controlsRow.removeFromLeft(kLabelWidth));
        gridCombo.setBounds(controlsRow.removeFromLeft(kComboWidth));

        grid.setBounds(bounds);
    }
}
