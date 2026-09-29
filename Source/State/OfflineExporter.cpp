#include "OfflineExporter.h"

#include "Core/ParameterIDs.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        int secondsToSamples(double seconds, double sampleRate)
        {
            return std::max(0, juce::roundToInt(seconds * sampleRate));
        }

        // Puts the private instance into "play the pattern once from 0"
        // mode. Runs after the state load because looping, follow-host
        // and bypass are all part of the saved state.
        void configureForExport(B33pProcessor& p, juce::int64 seed)
        {
            p.setLooping(false);
            // With follow-host on and no host playhead, processBlock
            // never starts playback and the export would be silent.
            p.setFollowHostTransport(false);
            if (auto* bypass = p.getApvts().getParameter(ParameterIDs::bypass()))
                bypass->setValueNotifyingHost(0.0f);
            p.stopPlayback();
            p.seedRandomSources(seed);
        }
    }

    juce::MemoryBlock OfflineExporter::captureState(B33pProcessor& live)
    {
        juce::MemoryBlock state;
        live.getStateInformation(state);
        return state;
    }

    OfflineExporter::OfflineExporter(const juce::MemoryBlock& state)
        : processor(std::make_unique<B33pProcessor>())
    {
        processor->setStateInformation(state.getData(),
                                       static_cast<int>(state.getSize()));
    }

    juce::AudioBuffer<float> OfflineExporter::render(const Settings& settings)
    {
        auto& p = *processor;
        const double sampleRate = settings.sampleRate > 0.0 ? settings.sampleRate : 48000.0;
        const int    blockSize  = std::max(1, settings.blockSize);

        configureForExport(p, settings.seed);
        p.prepareToPlay(sampleRate, blockSize);

        const int latency        = p.getLatencySamples();
        const int patternSamples = secondsToSamples(p.getPattern().getLengthSeconds(), sampleRate);
        const int maxTailSamples = secondsToSamples(settings.maxTailSeconds, sampleRate);
        const int holdSamples    = secondsToSamples(kSilenceHoldSeconds, sampleRate);

        // Positions below are in rendered-sample coordinates, which run
        // `latency` samples behind pattern time.
        const int patternEnd = latency + patternSamples;
        const int hardEnd    = patternEnd + maxTailSamples;

        // Sized once up front; the loop never grows it.
        juce::AudioBuffer<float> rendered(1, hardEnd + blockSize);
        rendered.clear();
        juce::AudioBuffer<float> block(2, blockSize);
        juce::MidiBuffer         midi;

        p.startPlayback();

        int written     = 0;
        int lastAudible = -1;
        while (written < hardEnd)
        {
            block.clear();
            midi.clear();
            p.processBlock(block, midi);

            // The engine writes the same mono mix to both channels.
            const float* src = block.getReadPointer(0);
            rendered.copyFrom(0, written, src, blockSize);
            for (int i = 0; i < blockSize; ++i)
                if (std::fabs(src[i]) >= kSilenceThreshold)
                    lastAudible = written + i;
            written += blockSize;

            if (p.isPlaying() || written < patternEnd)
                continue;
            if (written - (lastAudible + 1) >= holdSamples)
                break;
        }

        p.stopPlayback();

        const int end = std::min({ std::max(patternEnd, lastAudible + 1), hardEnd, written });
        const int outLength = std::max(0, end - latency);

        juce::AudioBuffer<float> out(1, outLength);
        if (outLength > 0)
            out.copyFrom(0, 0, rendered, 0, latency, outLength);
        return out;
    }

    juce::AudioBuffer<float> OfflineExporter::renderVariation(int variationIndex,
                                                              const Settings& settings,
                                                              juce::Random& rollRng,
                                                              juce::Thread* callingThread)
    {
        if (variationIndex > 0)
        {
            if (callingThread != nullptr)
            {
                const juce::MessageManagerLock mml(callingThread);
                if (! mml.lockWasGained())
                    return {};
                processor->getRandomizer().rollAllUnlocked(rollRng);
            }
            else
            {
                processor->getRandomizer().rollAllUnlocked(rollRng);
            }
        }

        auto s = settings;
        s.seed = settings.seed + variationIndex;
        return render(s);
    }
}
