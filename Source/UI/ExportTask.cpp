#include "ExportTask.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "Pattern/PatternRenderer.h"
#include "Pattern/WavWriter.h"

namespace B33p
{
    namespace
    {
        // Pulls every voice parameter for one lane out of APVTS into
        // a plain LaneConfig.
        PatternRenderer::LaneConfig buildLaneConfig(
            const juce::AudioProcessorValueTreeState& apvts, int lane)
        {
            PatternRenderer::LaneConfig lc;
            lc.waveform = static_cast<Oscillator::Waveform>(
                juce::jlimit(0, 4,
                    static_cast<int>(apvts.getRawParameterValue(
                        ParameterIDs::oscWaveform(lane))->load())));
            lc.basePitchHz          = apvts.getRawParameterValue(ParameterIDs::basePitchHz(lane))->load();
            lc.ampAttack            = apvts.getRawParameterValue(ParameterIDs::ampAttack(lane))->load();
            lc.ampDecay             = apvts.getRawParameterValue(ParameterIDs::ampDecay(lane))->load();
            lc.ampSustain           = apvts.getRawParameterValue(ParameterIDs::ampSustain(lane))->load();
            lc.ampRelease           = apvts.getRawParameterValue(ParameterIDs::ampRelease(lane))->load();
            lc.filterCutoffHz       = apvts.getRawParameterValue(ParameterIDs::filterCutoffHz(lane))->load();
            lc.filterResonanceQ     = apvts.getRawParameterValue(ParameterIDs::filterResonanceQ(lane))->load();
            lc.bitcrushBitDepth     = apvts.getRawParameterValue(ParameterIDs::bitcrushBitDepth(lane))->load();
            lc.bitcrushSampleRateHz = apvts.getRawParameterValue(ParameterIDs::bitcrushSampleRateHz(lane))->load();
            lc.distortionDrive      = apvts.getRawParameterValue(ParameterIDs::distortionDrive(lane))->load();
            lc.gain                 = apvts.getRawParameterValue(ParameterIDs::voiceGain(lane))->load();
            return lc;
        }

        // Pulls every lane's voice parameters out of APVTS into a
        // plain PatternRenderer::Config — keeps the renderer ignorant
        // of APVTS and gives the export thread a value-type snapshot
        // decoupled from later UI edits.
        PatternRenderer::Config buildConfig(const B33pProcessor& processor,
                                            double sampleRate)
        {
            PatternRenderer::Config c;
            c.sampleRate     = sampleRate;
            c.maxTailSeconds = 5.0;
            c.pitchCurve     = processor.getPitchCurveCopy();

            const auto& apvts = processor.getApvts();
            for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
                c.lanes[static_cast<size_t>(lane)] = buildLaneConfig(apvts, lane);

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
            errorMessage = "Could not write WAV. Check that the destination path is writable.";

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
