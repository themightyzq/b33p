#pragma once

#include "ExportDialog.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Background-thread WAV export driven through JUCE's standard
    // ThreadWithProgressWindow. Pops up a modal "Exporting..."
    // dialog with a progress bar and a Cancel button while the
    // render + write run on a background thread, so the UI stays
    // responsive even on slower machines.
    //
    // JUCE 8 disables the synchronous runThread() entry point
    // (gated on JUCE_MODAL_LOOPS_PERMITTED, which is off by
    // default), so the task is launched asynchronously and shows
    // its own success / failure alert from the threadComplete()
    // override. The task self-deletes after the alert is queued —
    // the caller does not have to track its lifetime.
    //
    // Cancellation: clicking the dialog's Cancel button signals
    // threadShouldExit(); the task respects it between render and
    // WAV write. Mid-render cancellation is intentionally not
    // supported — renders are sub-second on typical hardware so
    // the simpler step-boundary check is enough for MVP.
    class ExportTask : public juce::ThreadWithProgressWindow
    {
    public:
        // Allocates the task on the heap, launches its thread, and
        // returns immediately. The task self-deletes in
        // threadComplete() after showing its result alert.
        static void launchAsync(B33pProcessor& processor,
                                ExportDialog::Result settings);

    private:
        ExportTask(B33pProcessor& processor, ExportDialog::Result settings);

        void run() override;
        void threadComplete(bool userPressedCancel) override;

        // Generates the actual destination path for variation N.
        // settings.variationCount == 1 returns the user's chosen
        // path verbatim; > 1 inserts a 3-digit suffix before the
        // extension so a directory of variations sorts naturally.
        juce::File destinationForVariation(int variationIndex) const;

        // Snapshots / restores the APVTS state around a batch
        // render so the user's patch survives the dice rolls.
        std::unique_ptr<juce::XmlElement> snapshotApvtsState() const;
        void                              restoreApvtsState(
            const juce::XmlElement& snapshot);

        B33pProcessor&        processor;
        ExportDialog::Result  settings;

        bool         exportSucceeded { false };
        int          successfulRenders { 0 };
        juce::String errorMessage;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportTask)
    };
}
