#include "VoiceRenderer.h"

#include <algorithm>

namespace B33p::VoiceRenderer
{
    juce::AudioBuffer<float> renderEvent(Voice& voice,
                                         double sampleRate,
                                         double heldSeconds,
                                         double maxTailSeconds)
    {
        const int heldSamples    = std::max(0, static_cast<int>(heldSeconds    * sampleRate));
        const int maxTailSamples = std::max(0, static_cast<int>(maxTailSeconds * sampleRate));

        juce::AudioBuffer<float> buffer { 1, heldSamples + maxTailSamples };
        buffer.clear();

        voice.trigger(static_cast<float>(heldSeconds), 0.0f);

        int written = 0;
        for (; written < heldSamples; ++written)
            buffer.setSample(0, written, voice.processSample());

        voice.noteOff();

        for (; written < buffer.getNumSamples() && voice.isActive(); ++written)
            buffer.setSample(0, written, voice.processSample());

        // Trim to the exact filled length so callers don't have to
        // carry a separate sample count alongside the buffer.
        buffer.setSize(1, written, /*keepExistingContent=*/true,
                       /*clearExtraSpace=*/false, /*avoidReallocating=*/false);
        return buffer;
    }

    bool writeWavMono16(const juce::AudioBuffer<float>& buffer,
                        double sampleRate,
                        const juce::File& destination)
    {
        destination.deleteFile();

        auto stream = destination.createOutputStream();
        if (stream == nullptr)
            return false;

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer {
            wavFormat.createWriterFor(stream.release(),
                                      sampleRate,
                                      1,    // channels
                                      16,   // bits per sample
                                      {},   // metadata
                                      0)    // quality hint (unused for WAV)
        };

        if (writer == nullptr)
            return false;

        return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }
}
