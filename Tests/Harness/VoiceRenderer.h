#pragma once

#include "DSP/Voice.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace B33p::VoiceRenderer
{
    // Render a single event: trigger the voice, hold for heldSeconds,
    // call noteOff, then keep rendering until the voice's amp envelope
    // finishes its release tail or maxTailSeconds elapses (whichever
    // comes first). Returns a mono buffer trimmed to the number of
    // samples actually rendered.
    //
    // heldSeconds is also passed to voice.trigger() as the pitch
    // envelope's playback duration, so the curve reaches its final
    // point right when noteOff fires.
    juce::AudioBuffer<float> renderEvent(Voice& voice,
                                         double sampleRate,
                                         double heldSeconds,
                                         double maxTailSeconds);

    // Write a mono 16-bit PCM WAV to destination. Returns true on
    // success. The writer's destructor flushes before the function
    // returns, so the file is complete by the time the caller sees
    // the result.
    bool writeWavMono16(const juce::AudioBuffer<float>& buffer,
                        double sampleRate,
                        const juce::File& destination);
}
