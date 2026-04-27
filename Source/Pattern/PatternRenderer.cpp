#include "PatternRenderer.h"

#include "DSP/Voice.h"
#include "PatternSnapshot.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        void applyConfig(Voice& voice, const PatternRenderer::Config& config)
        {
            voice.setWaveform           (config.waveform);
            voice.setBasePitchHz        (config.basePitchHz);
            voice.setAmpAttack          (config.ampAttack);
            voice.setAmpDecay           (config.ampDecay);
            voice.setAmpSustain         (config.ampSustain);
            voice.setAmpRelease         (config.ampRelease);
            voice.setFilterCutoff       (config.filterCutoffHz);
            voice.setFilterResonance    (config.filterResonanceQ);
            voice.setBitcrushBitDepth   (config.bitcrushBitDepth);
            voice.setBitcrushSampleRate (config.bitcrushSampleRateHz);
            voice.setDistortionDrive    (config.distortionDrive);
            voice.setGain               (config.gain);
            voice.setPitchCurve         (config.pitchCurve);
        }
    }

    juce::AudioBuffer<float> PatternRenderer::render(const Pattern& pattern,
                                                     const Config& config)
    {
        Voice voice;
        voice.prepare(config.sampleRate);
        applyConfig(voice, config);

        const auto snapshot = makeSnapshot(pattern);

        const int patternSamples = std::max(0,
            static_cast<int>(snapshot.lengthSeconds * config.sampleRate));
        const int maxTailSamples = std::max(0,
            static_cast<int>(config.maxTailSeconds * config.sampleRate));

        juce::AudioBuffer<float> buffer { 1, patternSamples + maxTailSamples };
        buffer.clear();

        int eventIndex          = 0;
        int samplesUntilNoteOff = 0;
        int sampleIndex         = 0;

        // ---- Pattern playback ----------------------------------
        for (; sampleIndex < patternSamples; ++sampleIndex)
        {
            const double t = static_cast<double>(sampleIndex) / config.sampleRate;

            while (eventIndex < static_cast<int>(snapshot.events.size())
                   && snapshot.events[static_cast<size_t>(eventIndex)].startSeconds <= t)
            {
                const auto& e = snapshot.events[static_cast<size_t>(eventIndex)];
                voice.trigger(static_cast<float>(e.durationSeconds),
                              e.pitchOffsetSemitones,
                              e.velocity);
                samplesUntilNoteOff = static_cast<int>(
                    e.durationSeconds * config.sampleRate);
                ++eventIndex;
            }

            if (samplesUntilNoteOff > 0)
            {
                if (--samplesUntilNoteOff == 0)
                    voice.noteOff();
            }

            buffer.setSample(0, sampleIndex, voice.processSample());
        }

        // ---- Tail ----------------------------------------------
        // Match the live engine's non-looped behaviour: at pattern
        // end the voice receives noteOff, then we keep rendering
        // the release tail until amp envelope goes idle.
        if (voice.isActive())
            voice.noteOff();

        for (; sampleIndex < buffer.getNumSamples() && voice.isActive(); ++sampleIndex)
            buffer.setSample(0, sampleIndex, voice.processSample());

        // Trim to whatever we actually filled.
        buffer.setSize(1, sampleIndex, /*keepExistingContent=*/true,
                       /*clearExtraSpace=*/false, /*avoidReallocating=*/false);
        return buffer;
    }
}
