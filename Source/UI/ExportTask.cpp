#include "ExportTask.h"

#include "Pattern/AudioFileWriter.h"

namespace B33p
{
    void ExportTask::launchAsync(B33pProcessor& processor,
                                  ExportDialog::Result settings)
    {
        // Capture here, on the message thread, so the export sees one
        // consistent snapshot even if the user keeps editing while it
        // renders.
        auto* task = new ExportTask(OfflineExporter::captureState(processor),
                                    std::move(settings));
        task->launchThread();
        // task is now owned by itself — self-deletes in threadComplete().
    }

    ExportTask::ExportTask(const juce::MemoryBlock& liveState,
                           ExportDialog::Result settingsIn)
        : juce::ThreadWithProgressWindow(
              settingsIn.variationCount > 1 ? "Batch exporting WAVs..."
                                             : "Exporting WAV...",
              /*hasProgressBar=*/ true,
              /*hasCancelButton=*/ true),
          exporter(std::make_unique<OfflineExporter>(liveState)),
          settings(std::move(settingsIn))
    {
    }

    juce::File ExportTask::destinationForVariation(int variationIndex) const
    {
        if (settings.variationCount <= 1)
            return settings.destination;

        // Three-digit zero-padded suffix so a directory of 100
        // variations sorts naturally even when listed
        // alphabetically.
        const auto stem    = settings.destination.getFileNameWithoutExtension();
        const auto ext     = settings.destination.getFileExtension();
        const auto parent  = settings.destination.getParentDirectory();
        const juce::String suffix = "_" + juce::String(variationIndex + 1).paddedLeft('0', 3);
        return parent.getChildFile(stem + suffix + ext);
    }

    void ExportTask::run()
    {
        const int variations = juce::jmax(1, settings.variationCount);
        const bool isBatch   = variations > 1;

        OfflineExporter::Settings renderSettings;
        renderSettings.sampleRate = settings.sampleRate;

        // Batch dice rolls land on the exporter's private processor, so
        // the user's patch is never modified and needs no restore.
        juce::Random rollRng;

        setProgress(0.0);
        successfulRenders = 0;

        for (int i = 0; i < variations; ++i)
        {
            if (threadShouldExit())
                break;

            setStatusMessage(isBatch
                ? "Rendering variation " + juce::String(i + 1)
                  + " of " + juce::String(variations) + "..."
                : juce::String("Rendering..."));

            // Variation 0 is the user's current patch verbatim, so
            // they always have a clean reference; later variations
            // roll outward from there.
            const auto buffer = exporter->renderVariation(i, renderSettings, rollRng, this);

            if (threadShouldExit())
                break;

            const auto destination = destinationForVariation(i);
            const bool ok = writeAudioFile(settings.format,
                                            buffer,
                                            settings.sampleRate,
                                            settings.bitDepth,
                                            settings.channelMode,
                                            destination);
            if (! ok)
            {
                errorMessage = "Could not write \""
                                + destination.getFileName()
                                + "\". Check destination is writable.";
                break;
            }
            ++successfulRenders;

            setProgress(static_cast<double>(i + 1)
                            / static_cast<double>(variations));
        }

        exportSucceeded = (successfulRenders == variations);
    }

    void ExportTask::threadComplete(bool userPressedCancel)
    {
        if (! userPressedCancel)
        {
            const bool isBatch = settings.variationCount > 1;
            if (exportSucceeded)
            {
                const juce::String msg = isBatch
                    ? juce::String(settings.variationCount)
                          + " variations written next to:\n"
                          + settings.destination.getFullPathName()
                    : juce::String("WAV written to:\n")
                          + settings.destination.getFullPathName();
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::InfoIcon)
                        .withTitle("Export complete")
                        .withMessage(msg)
                        .withButton("OK"),
                    nullptr);
            }
            else
            {
                juce::String msg = errorMessage.isEmpty()
                    ? juce::String("Unknown error during export.")
                    : errorMessage;
                if (isBatch && successfulRenders > 0)
                    msg = juce::String(successfulRenders)
                          + " of "
                          + juce::String(settings.variationCount)
                          + " variations succeeded before the error:\n\n" + msg;
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::WarningIcon)
                        .withTitle("Export failed")
                        .withMessage(msg)
                        .withButton("OK"),
                    nullptr);
            }
        }

        delete this;
    }
}
