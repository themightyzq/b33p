#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace B33p
{
    // Production WAV writer. Pairs naturally with PatternRenderer:
    // PatternRenderer hands you a mono float buffer, this writes it
    // out at one of the three Phase 5 bit depths and either as a
    // single-channel mono file or as a stereo file with both channels
    // carrying the same mono data.
    //
    // Bit depths follow JUCE's WavAudioFormat conventions:
    //   * 8  → 8-bit unsigned PCM (the lo-fi / retro path)
    //   * 16 → 16-bit signed PCM
    //   * 24 → 24-bit signed PCM
    enum class BitDepth { Eight = 8, Sixteen = 16, TwentyFour = 24 };

    enum class ChannelMode { Mono, StereoDuplicated };

    // Writes the given mono buffer to destination, returning true on
    // success. The destination file is replaced if it already exists.
    // Returns false if the output stream can't be opened or the
    // WavAudioFormat refuses the requested format combination.
    bool writeWav(const juce::AudioBuffer<float>& monoBuffer,
                  double sampleRate,
                  BitDepth bitDepth,
                  ChannelMode channelMode,
                  const juce::File& destination);
}
