#include "PatternSection.h"

#include "ExportTask.h"
#include "State/UndoableActions.h"

#include <array>

namespace B33p
{
    namespace
    {
        constexpr int  kControlsHeight  = 28;
        constexpr int  kControlsGap     = 6;
        constexpr int  kButtonWidth     = 80;
        constexpr int  kExportWidth     = 90;
        constexpr int  kRandomizeWidth  = 120;
        constexpr int  kComboWidth      = 110;
        constexpr int  kLabelWidth      = 50;
        constexpr int  kTimeWidth       = 180;
        constexpr int  kInspectorHeight = 32;
        constexpr int  kRepaintHz       = 30;

        struct LengthPreset { const char* label; double seconds; };
        constexpr std::array<LengthPreset, 6> kLengthPresets {{
            { "0.5 s", 0.5 },
            { "1.0 s", 1.0 },
            { "2.0 s", 2.0 },
            { "3.0 s", 3.0 },
            { "5.0 s", 5.0 },
            { "10.0 s", 10.0 },
        }};

        // Each preset is either a fixed-seconds value (positive
        // seconds, beatFraction = 0) or a beat-fraction value
        // (seconds = 0, positive beatFraction). computeSeconds()
        // collapses the two into a single number using the current
        // pattern BPM. seconds = 0 AND beatFraction = 0 means
        // "snap is off".
        struct GridPreset
        {
            const char* label;
            double      seconds;
            double      beatFraction;

            double computeSeconds(double bpm) const noexcept
            {
                if (beatFraction > 0.0)
                    return beatFraction * 60.0 / bpm;
                return seconds;
            }
        };

        // beatFraction is the duration in beats. A "1/4 note" lasts
        // exactly one beat in 4/4; a "1/16 note" lasts a quarter of
        // a beat; etc. Time-based and musical entries coexist so
        // users can stay in either domain.
        constexpr std::array<GridPreset, 14> kGridPresets {{
            { "10 ms",     0.010, 0.0    },
            { "25 ms",     0.025, 0.0    },
            { "50 ms",     0.050, 0.0    },
            { "100 ms",    0.100, 0.0    },
            { "250 ms",    0.250, 0.0    },
            { "500 ms",    0.500, 0.0    },
            { "1000 ms",   1.000, 0.0    },
            { "1/32 note", 0.0,   0.125  },
            { "1/16 note", 0.0,   0.25   },
            { "1/8 note",  0.0,   0.5    },
            { "1/4 note",  0.0,   1.0    },
            { "1/2 note",  0.0,   2.0    },
            { "Whole",     0.0,   4.0    },
            { "Off",       0.0,   0.0    },
        }};

        // 12 common time signatures. Each entry is {numerator,
        // denominator, label} — denominators are powers of two so
        // they map cleanly onto a beat-fraction snap grid.
        struct TimeSigPreset { int numerator; int denominator; const char* label; };
        constexpr std::array<TimeSigPreset, 9> kTimeSigPresets {{
            {  4, 4, "4/4" },
            {  3, 4, "3/4" },
            {  2, 4, "2/4" },
            {  6, 8, "6/8" },
            {  7, 8, "7/8" },
            {  9, 8, "9/8" },
            { 12, 8, "12/8"},
            {  5, 4, "5/4" },
            {  6, 4, "6/4" },
        }};

        // Item IDs are 1-based for juce::ComboBox so 0 means
        // "no selection". Map 1..N to indices.
        int idForIndex(int i) { return i + 1; }
    }

    PatternSection::PatternSection(B33pProcessor& processorRef)
        : Section("Pattern"),
          processor(processorRef),
          grid(processorRef),
          inspector(processorRef)
    {
        addAndMakeVisible(grid);
        addAndMakeVisible(inspector);

        // Live two-way wiring: the grid drives selection identity +
        // content; inspector edits push back through the pattern and
        // are picked up on the grid's next repaint.
        grid.onSelectionChanged = [this]
        {
            inspector.setSelection(grid.getPrimarySelection(),
                                    static_cast<int>(grid.getSelectedEvents().size()));
        };
        inspector.onDeleteRequested = [this]
        {
            const auto sel = grid.getPrimarySelection();
            if (! sel.valid())
                return;

            Pattern before = processor.getPattern();
            processor.getPattern().removeEvent(sel.lane, sel.index);
            processor.markDirty();
            grid.clearSelection();
            grid.repaint();

            processor.getUndoManager().beginNewTransaction("Delete event");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, &grid,
                                     std::move(before),
                                     processor.getPattern()));
        };

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
            // Pick the preset whose computed seconds best matches
            // the grid's current seconds at the pattern's BPM.
            const double current = grid.getGridSeconds();
            const double bpm     = processor.getPattern().getBpm();
            int bestId = idForIndex(0);
            double bestDist = 1e30;
            for (size_t i = 0; i < kGridPresets.size(); ++i)
            {
                const double d = std::abs(kGridPresets[i].computeSeconds(bpm) - current);
                if (d < bestDist) { bestDist = d; bestId = idForIndex(static_cast<int>(i)); }
            }
            gridCombo.setSelectedId(bestId, juce::dontSendNotification);
        }
        gridCombo.onChange = [this] { onGridChanged(); };
        addAndMakeVisible(gridCombo);

        // BPM input (numeric slider with inc/dec arrows).
        bpmLabel.setText("BPM:", juce::dontSendNotification);
        bpmLabel.setJustificationType(juce::Justification::centredRight);
        bpmLabel.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(bpmLabel);

        bpmSlider.setSliderStyle(juce::Slider::IncDecButtons);
        bpmSlider.setRange(Pattern::kMinBpm, Pattern::kMaxBpm, 1.0);
        bpmSlider.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_AutoDirection);
        bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 22);
        bpmSlider.setValue(processor.getPattern().getBpm(),
                            juce::dontSendNotification);
        bpmSlider.onValueChange = [this] { onBpmChanged(); };
        addAndMakeVisible(bpmSlider);

        // Time-signature combo
        timeSigLabel.setText("Sig:", juce::dontSendNotification);
        timeSigLabel.setJustificationType(juce::Justification::centredRight);
        timeSigLabel.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(timeSigLabel);

        for (size_t i = 0; i < kTimeSigPresets.size(); ++i)
            timeSigCombo.addItem(kTimeSigPresets[i].label,
                                  idForIndex(static_cast<int>(i)));
        timeSigCombo.onChange = [this] { onTimeSigChanged(); };
        addAndMakeVisible(timeSigCombo);
        syncBpmAndTimeSigFromPattern();

        // Randomize-all and Export buttons (right-aligned to separate
        // the action group from playback / editing controls).
        randomizeAllButton.onClick = [this]
        {
            juce::Random rng;
            processor.getRandomizer().rollAllUnlocked(rng);
        };
        addAndMakeVisible(randomizeAllButton);

        exportButton.onClick = [this] { onExportClicked(); };
        addAndMakeVisible(exportButton);

        playButton        .setTooltip("Play the pattern from the start (Space)");
        loopToggle        .setTooltip("Loop playback when the end is reached");
        lengthCombo       .setTooltip("Total length of the pattern");
        gridCombo         .setTooltip("Snap event positions to this grid (Off = free). Musical entries follow the pattern's BPM.");
        bpmSlider         .setTooltip("Pattern tempo in beats per minute. Drives the musical grid + the bars/beats time display.");
        timeSigCombo      .setTooltip("Time signature. Sets how many beats sit in a bar for the time display.");
        randomizeAllButton.setTooltip("Randomize every unlocked parameter across all 4 lanes");
        exportButton      .setTooltip("Render the pattern to a WAV file");

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
        syncBpmAndTimeSigFromPattern();
        applyGridPreset();
        loopToggle.setToggleState(processor.getLooping(),
                                   juce::dontSendNotification);
        grid.refreshLaneMetaFromPattern();
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
        applyGridPreset();
    }

    void PatternSection::applyGridPreset()
    {
        const int id = gridCombo.getSelectedId();
        if (id <= 0) return;
        const size_t idx = static_cast<size_t>(id - 1);
        if (idx >= kGridPresets.size()) return;
        const double bpm = processor.getPattern().getBpm();
        grid.setGridSeconds(kGridPresets[idx].computeSeconds(bpm));
    }

    void PatternSection::onBpmChanged()
    {
        const double newBpm = bpmSlider.getValue();
        processor.getPattern().setBpm(newBpm);
        processor.markDirty();
        // Re-evaluate the grid in case the user has a musical
        // preset selected — it follows BPM by definition.
        applyGridPreset();
        // Repaint the grid so the dual-time ruler reflects the
        // new BPM immediately.
        grid.repaint();
    }

    void PatternSection::onTimeSigChanged()
    {
        const int id = timeSigCombo.getSelectedId();
        if (id <= 0) return;
        const size_t idx = static_cast<size_t>(id - 1);
        if (idx >= kTimeSigPresets.size()) return;
        const auto& ts = kTimeSigPresets[idx];
        processor.getPattern().setTimeSignature(ts.numerator, ts.denominator);
        processor.markDirty();
        grid.repaint();
    }

    void PatternSection::syncBpmAndTimeSigFromPattern()
    {
        const auto& p = processor.getPattern();
        bpmSlider.setValue(p.getBpm(), juce::dontSendNotification);

        const int num = p.getTimeSigNumerator();
        const int den = p.getTimeSigDenominator();
        for (size_t i = 0; i < kTimeSigPresets.size(); ++i)
        {
            if (kTimeSigPresets[i].numerator == num
                && kTimeSigPresets[i].denominator == den)
            {
                timeSigCombo.setSelectedId(idForIndex(static_cast<int>(i)),
                                            juce::dontSendNotification);
                return;
            }
        }
        // Unknown time sig in file — fall back to first entry.
        timeSigCombo.setSelectedId(idForIndex(0), juce::dontSendNotification);
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

        const auto& pattern = processor.getPattern();
        const double headSec   = playing ? processor.getPlayheadSeconds() : 0.0;
        const double lengthSec = pattern.getLengthSeconds();
        const double bpm       = pattern.getBpm();
        const int    numer     = pattern.getTimeSigNumerator();
        const double secPerBeat = 60.0 / bpm;

        // Bar/beat readout. beatsTotal = total elapsed beats; from
        // there it's straight integer division to bar (1-indexed)
        // and beat (also 1-indexed).
        const double beatsTotal = headSec / secPerBeat;
        const int    barIdx     = static_cast<int>(std::floor(beatsTotal
                                                       / static_cast<double>(numer)));
        const int    beatInBar  = static_cast<int>(std::floor(beatsTotal))
                                       - barIdx * numer;

        timeLabel.setText("Bar " + juce::String(barIdx + 1)
                            + "." + juce::String(beatInBar + 1)
                            + " — " + juce::String(headSec,   2)
                            + " / " + juce::String(lengthSec, 2) + "s",
                          juce::dontSendNotification);

        if (playing)
        {
            // Republish the snapshot so any pattern edits the user
            // made since the last tick (add / remove / drag /
            // inspector edit / undo / redo) are picked up on the
            // audio thread within ~33 ms — fast enough to feel live.
            processor.refreshPatternSnapshot();
            grid.repaint();
        }
    }

    void PatternSection::resized()
    {
        auto bounds = getContentBounds();

        auto controlsRow = bounds.removeFromTop(kControlsHeight);
        bounds.removeFromTop(kControlsGap);

        // Right-align Export + Randomize All to separate the action
        // group from the play/edit controls on the left.
        exportButton      .setBounds(controlsRow.removeFromRight(kExportWidth));
        controlsRow.removeFromRight(kControlsGap);
        randomizeAllButton.setBounds(controlsRow.removeFromRight(kRandomizeWidth));
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
        controlsRow.removeFromLeft(kControlsGap);

        constexpr int kBpmWidth  = 100;
        constexpr int kSigWidth  = 70;
        bpmLabel.setBounds(controlsRow.removeFromLeft(kLabelWidth));
        bpmSlider.setBounds(controlsRow.removeFromLeft(kBpmWidth));
        controlsRow.removeFromLeft(kControlsGap);
        timeSigLabel.setBounds(controlsRow.removeFromLeft(kLabelWidth));
        timeSigCombo.setBounds(controlsRow.removeFromLeft(kSigWidth));

        // Inspector strip docks at the bottom; grid takes the rest.
        inspector.setBounds(bounds.removeFromBottom(kInspectorHeight));
        bounds.removeFromBottom(kControlsGap);
        grid.setBounds(bounds);
    }
}
