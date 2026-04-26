#include "WavWriter.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace B33p
{
    bool writeWav(const juce::AudioBuffer<float>& monoBuffer,
                  double sampleRate,
                  BitDepth bitDepth,
                  ChannelMode channelMode,
                  const juce::File& destination)
    {
        destination.deleteFile();

        auto stream = destination.createOutputStream();
        if (stream == nullptr)
            return false;

        const unsigned int numChannels =
            (channelMode == ChannelMode::StereoDuplicated) ? 2u : 1u;

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer {
            wavFormat.createWriterFor(stream.release(),
                                      sampleRate,
                                      numChannels,
                                      static_cast<int>(bitDepth),
                                      {},   // metadata
                                      0)    // quality hint (unused for WAV)
        };

        if (writer == nullptr)
            return false;

        if (channelMode == ChannelMode::Mono)
        {
            return writer->writeFromAudioSampleBuffer(
                monoBuffer, 0, monoBuffer.getNumSamples());
        }

        // Stereo-duplicated: build a real 2-channel buffer carrying the
        // same mono data on both channels. Cheaper alternatives (e.g.
        // a fake AudioBuffer view with two pointers to the same array)
        // would need const_cast gymnastics; copying is clean and the
        // export is offline so the extra alloc is irrelevant.
        juce::AudioBuffer<float> stereo { 2, monoBuffer.getNumSamples() };
        stereo.copyFrom(0, 0, monoBuffer, 0, 0, monoBuffer.getNumSamples());
        stereo.copyFrom(1, 0, monoBuffer, 0, 0, monoBuffer.getNumSamples());
        return writer->writeFromAudioSampleBuffer(
            stereo, 0, stereo.getNumSamples());
    }
}
