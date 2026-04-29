#include "AudioFileWriter.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>

namespace B33p
{
    juce::String extensionFor(AudioFileFormat format)
    {
        switch (format)
        {
            case AudioFileFormat::Wav:       return "wav";
            case AudioFileFormat::Aiff:      return "aiff";
            case AudioFileFormat::Flac:      return "flac";
            case AudioFileFormat::OggVorbis: return "ogg";
        }
        return "wav";
    }

    namespace
    {
        // Heap-allocated juce::AudioFormat instance for a given
        // format. nullptr means the format wasn't compiled in
        // (FLAC / OggVorbis are conditional on JUCE_USE_FLAC /
        // JUCE_USE_OGGVORBIS) and the caller should fail the
        // write.
        std::unique_ptr<juce::AudioFormat> makeFormat(AudioFileFormat format)
        {
            switch (format)
            {
                case AudioFileFormat::Wav:
                    return std::make_unique<juce::WavAudioFormat>();
                case AudioFileFormat::Aiff:
                    return std::make_unique<juce::AiffAudioFormat>();
                case AudioFileFormat::Flac:
                   #if JUCE_USE_FLAC
                    return std::make_unique<juce::FlacAudioFormat>();
                   #else
                    return nullptr;
                   #endif
                case AudioFileFormat::OggVorbis:
                   #if JUCE_USE_OGGVORBIS
                    return std::make_unique<juce::OggVorbisAudioFormat>();
                   #else
                    return nullptr;
                   #endif
            }
            return nullptr;
        }
    }

    bool writeAudioFile(AudioFileFormat format,
                        const juce::AudioBuffer<float>& monoBuffer,
                        double sampleRate,
                        BitDepth bitDepth,
                        ChannelMode channelMode,
                        const juce::File& destination)
    {
        auto fmt = makeFormat(format);
        if (fmt == nullptr)
            return false;

        destination.deleteFile();

        auto stream = destination.createOutputStream();
        if (stream == nullptr)
            return false;

        const unsigned int numChannels =
            (channelMode == ChannelMode::StereoDuplicated) ? 2u : 1u;

        // OggVorbis ignores the bit-depth and pegs to its internal
        // format; pass 0 so JUCE doesn't reject our call. WAV /
        // AIFF / FLAC use the requested PCM bit-depth.
        const int bitsForFormat =
            (format == AudioFileFormat::OggVorbis) ? 0
                                                     : static_cast<int>(bitDepth);

        std::unique_ptr<juce::AudioFormatWriter> writer {
            fmt->createWriterFor(stream.release(),
                                  sampleRate,
                                  numChannels,
                                  bitsForFormat,
                                  {},   // metadata
                                  0)    // quality hint (unused except for OggVorbis)
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
