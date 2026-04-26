#include "ExportTask.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "Pattern/PatternRenderer.h"
#include "Pattern/WavWriter.h"

namespace B33p
{
    namespace
    {
        // Pulls every voice parameter out of APVTS into a plain
        // PatternRenderer::Config — keeps the renderer ignorant of
        // APVTS and gives the export thread a value-type snapshot
        // that is decoupled from later UI edits.
        PatternRenderer::Config buildConfig(const B33pProcessor& processor,
                                            double sampleRate)
        {
            PatternRenderer::Config c;
            c.sampleRate     = sampleRate;
            c.maxTailSeconds = 5.0;

            const auto& apvts = processor.getApvts();

            c.waveform = static_cast<Oscillator::Waveform>(
                juce::jlimit(0, 4,
                    static_cast<int>(apvts.getRawParameterValue(
                        ParameterIDs::oscWaveform)->load())));

            c.basePitchHz         = apvts.getRawParameterValue(ParameterIDs::basePitchHz)->load();
            c.ampAttack           = apvts.getRawParameterValue(ParameterIDs::ampAttack)->load();
            c.ampDecay            = apvts.getRawParameterValue(ParameterIDs::ampDecay)->load();
            c.ampSustain          = apvts.getRawParameterValue(ParameterIDs::ampSustain)->load();
            c.ampRelease          = apvts.getRawParameterValue(ParameterIDs::ampRelease)->load();
            c.filterCutoffHz      = apvts.getRawParameterValue(ParameterIDs::filterCutoffHz)->load();
            c.filterResonanceQ    = apvts.getRawParameterValue(ParameterIDs::filterResonanceQ)->load();
            c.bitcrushBitDepth    = apvts.getRawParameterValue(ParameterIDs::bitcrushBitDepth)->load();
            c.bitcrushSampleRateHz= apvts.getRawParameterValue(ParameterIDs::bitcrushSampleRateHz)->load();
            c.distortionDrive     = apvts.getRawParameterValue(ParameterIDs::distortionDrive)->load();
            c.gain                = apvts.getRawParameterValue(ParameterIDs::voiceGain)->load();
            c.pitchCurve          = processor.getPitchCurveCopy();

            return c;
        }
    }

    void ExportTask::launchAsync(B33pProcessor& processor,
                                  ExportDialog::Result settings)
    {
        auto* task = new ExportTask(processor, std::move(settings));
        task->launchThread();
        // task is now owned by itself — self-deletes in threadComplete().
    }

    ExportTask::ExportTask(B33pProcessor& processorRef,
                           ExportDialog::Result settingsIn)
        : juce::ThreadWithProgressWindow("Exporting WAV...",
                                         /*hasProgressBar=*/ true,
                                         /*hasCancelButton=*/ true),
          processor(processorRef),
          settings(std::move(settingsIn))
    {
    }

    void ExportTask::run()
    {
        setProgress(0.0);
        setStatusMessage("Rendering...");

        const auto config = buildConfig(processor, settings.sampleRate);

        if (threadShouldExit())
            return;

        const auto buffer = PatternRenderer::render(processor.getPattern(), config);

        setProgress(0.5);
        setStatusMessage("Writing WAV...");

        if (threadShouldExit())
            return;

        exportSucceeded = writeWav(buffer,
                                   settings.sampleRate,
                                   settings.bitDepth,
                                   settings.channelMode,
                                   settings.destination);

        if (! exportSucceeded)
            errorMessage = "Could not write WAV — check that the destination path is writable.";

        setProgress(1.0);
    }

    void ExportTask::threadComplete(bool userPressedCancel)
    {
        if (! userPressedCancel)
        {
            if (exportSucceeded)
            {
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::InfoIcon)
                        .withTitle("Export complete")
                        .withMessage("WAV written to:\n" + settings.destination.getFullPathName())
                        .withButton("OK"),
                    nullptr);
            }
            else
            {
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::WarningIcon)
                        .withTitle("Export failed")
                        .withMessage(errorMessage.isEmpty()
                                         ? juce::String("Unknown error during export.")
                                         : errorMessage)
                        .withButton("OK"),
                    nullptr);
            }
        }

        delete this;
    }
}
