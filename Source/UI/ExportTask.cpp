#include "ExportTask.h"

#include "Core/ParameterIDs.h"
#include "DSP/Oscillator.h"
#include "Pattern/PatternRenderer.h"
#include "Pattern/AudioFileWriter.h"

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
        : juce::ThreadWithProgressWindow(
              settingsIn.variationCount > 1 ? "Batch exporting WAVs..."
                                             : "Exporting WAV...",
              /*hasProgressBar=*/ true,
              /*hasCancelButton=*/ true),
          processor(processorRef),
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

    std::unique_ptr<juce::XmlElement> ExportTask::snapshotApvtsState() const
    {
        return processor.getApvts().copyState().createXml();
    }

    void ExportTask::restoreApvtsState(const juce::XmlElement& snapshot)
    {
        processor.getApvts().replaceState(juce::ValueTree::fromXml(snapshot));
    }

    void ExportTask::run()
    {
        const int variations = juce::jmax(1, settings.variationCount);
        const bool isBatch   = variations > 1;

        // Snapshot APVTS state so a batch run can restore the
        // user's patch when it finishes. Single-render path skips
        // the snapshot to avoid the extra XML serialisation cost.
        auto snapshot = isBatch ? snapshotApvtsState()
                                : std::unique_ptr<juce::XmlElement>{};

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

            // Re-roll on every iteration except the first — the
            // first variation captures the user's current patch
            // verbatim so they always have a clean reference,
            // and subsequent variations roll outward from there.
            if (isBatch && i > 0)
            {
                juce::Random rng;
                processor.getRandomizer().rollAllUnlocked(rng);
            }

            const auto config = buildConfig(processor, settings.sampleRate);
            const auto buffer = PatternRenderer::render(processor.getPattern(), config);

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

        // Always restore the snapshot, even on cancel / failure —
        // never strand the user with mid-batch random parameters.
        if (snapshot != nullptr)
            restoreApvtsState(*snapshot);

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
