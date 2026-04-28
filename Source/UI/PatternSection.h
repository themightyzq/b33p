#pragma once

#include "ExportDialog.h"
#include "InspectorStrip.h"
#include "PatternGrid.h"
#include "Section.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    class PatternSection : public Section
                          , private juce::Timer
    {
    public:
        explicit PatternSection(B33pProcessor& processor);
        ~PatternSection() override;

        void resized() override;

        // Called after Open / New so the length combo, loop toggle,
        // and grid all match the freshly-replaced pattern state.
        void refreshFromState();

        // Used by MainComponent's Edit menu to drive the grid's
        // public copy / paste / select-all / deselect entry points.
        PatternGrid& getGrid() { return grid; }

    private:
        void timerCallback() override;
        void onLengthChanged();
        void onGridChanged();
        void onExportClicked();
        void runExport(ExportDialog::Result settings);
        void syncLengthComboToPattern();

        B33pProcessor&   processor;
        PatternGrid      grid;
        InspectorStrip   inspector;

        juce::TextButton playButton   { "Play" };
        juce::TextButton loopToggle   { "Loop" };

        juce::Label      timeLabel;

        juce::Label      lengthLabel;
        juce::ComboBox   lengthCombo;

        juce::Label      gridLabel;
        juce::ComboBox   gridCombo;

        juce::TextButton randomizeAllButton { "Randomize All" };
        juce::TextButton exportButton       { "Export..." };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSection)
    };
}
