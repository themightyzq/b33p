#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace B33p
{
    // Container format the export will write. Each one maps to a
    // built-in juce::AudioFormat instance; FLAC and OggVorbis are
    // gated on JUCE_USE_FLAC / JUCE_USE_OGGVORBIS at JUCE module
    // configuration time. Both default to enabled in juce_audio_formats.
    enum class AudioFileFormat
    {
        Wav,
        Aiff,
        Flac,
        OggVorbis,
    };

    // Bit depths follow JUCE's WavAudioFormat conventions:
    //   * 8  → 8-bit unsigned PCM (the lo-fi / retro path)
    //   * 16 → 16-bit signed PCM
    //   * 24 → 24-bit signed PCM
    // FLAC accepts 16/24; AIFF accepts 8/16/24; OggVorbis is lossy
    // and ignores the bit-depth (it picks its own internal format).
    // Writers refuse the call (returning false) if the chosen
    // format / bit-depth pair isn't supported.
    enum class BitDepth { Eight = 8, Sixteen = 16, TwentyFour = 24 };

    enum class ChannelMode { Mono, StereoDuplicated };

    // Returns the canonical filename extension ("wav", "aiff",
    // "flac", "ogg") for a given format. ExportTask uses this to
    // ensure the destination filename matches the chosen format.
    juce::String extensionFor(AudioFileFormat format);

    // Writes the given mono buffer to destination, returning true on
    // success. The destination file is replaced if it already exists.
    // Returns false if the output stream can't be opened, the
    // requested format isn't compiled in, or the chosen format
    // refuses the bit-depth + channel-count combination.
    bool writeAudioFile(AudioFileFormat format,
                        const juce::AudioBuffer<float>& monoBuffer,
                        double sampleRate,
                        BitDepth bitDepth,
                        ChannelMode channelMode,
                        const juce::File& destination);

    // Backwards-compatible alias for the WAV-only path used by the
    // existing tests. Wraps writeAudioFile with format = Wav.
    inline bool writeWav(const juce::AudioBuffer<float>& monoBuffer,
                         double sampleRate,
                         BitDepth bitDepth,
                         ChannelMode channelMode,
                         const juce::File& destination)
    {
        return writeAudioFile(AudioFileFormat::Wav,
                              monoBuffer, sampleRate, bitDepth,
                              channelMode, destination);
    }
}
