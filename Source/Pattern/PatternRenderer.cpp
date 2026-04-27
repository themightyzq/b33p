#include "PatternRenderer.h"

#include "DSP/Voice.h"
#include "PatternSnapshot.h"

#include <algorithm>
#include <array>

namespace B33p
{
    namespace
    {
        void applyLane(Voice& voice, const PatternRenderer::LaneConfig& lc,
                       const std::vector<PitchEnvelopePoint>& pitchCurve)
        {
            voice.setWaveform           (lc.waveform);
            voice.setBasePitchHz        (lc.basePitchHz);
            voice.setAmpAttack          (lc.ampAttack);
            voice.setAmpDecay           (lc.ampDecay);
            voice.setAmpSustain         (lc.ampSustain);
            voice.setAmpRelease         (lc.ampRelease);
            voice.setFilterCutoff       (lc.filterCutoffHz);
            voice.setFilterResonance    (lc.filterResonanceQ);
            voice.setBitcrushBitDepth   (lc.bitcrushBitDepth);
            voice.setBitcrushSampleRate (lc.bitcrushSampleRateHz);
            voice.setDistortionDrive    (lc.distortionDrive);
            voice.setGain               (lc.gain);
            voice.setPitchCurve         (pitchCurve);
        }

        bool anyActive(const std::array<Voice, Pattern::kNumLanes>& voices)
        {
            for (const auto& v : voices)
                if (v.isActive())
                    return true;
            return false;
        }
    }

    juce::AudioBuffer<float> PatternRenderer::render(const Pattern& pattern,
                                                     const Config& config)
    {
        std::array<Voice, Pattern::kNumLanes> voices;
        for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
        {
            voices[static_cast<size_t>(lane)].prepare(config.sampleRate);
            applyLane(voices[static_cast<size_t>(lane)],
                       config.lanes[static_cast<size_t>(lane)],
                       config.pitchCurve);
        }

        const auto snapshot = makeSnapshot(pattern);

        const int patternSamples = std::max(0,
            static_cast<int>(snapshot.lengthSeconds * config.sampleRate));
        const int maxTailSamples = std::max(0,
            static_cast<int>(config.maxTailSeconds * config.sampleRate));

        juce::AudioBuffer<float> buffer { 1, patternSamples + maxTailSamples };
        buffer.clear();

        int eventIndex = 0;
        std::array<int, Pattern::kNumLanes> samplesUntilNoteOff { { 0, 0, 0, 0 } };
        int sampleIndex = 0;

        // ---- Pattern playback ----------------------------------
        for (; sampleIndex < patternSamples; ++sampleIndex)
        {
            const double t = static_cast<double>(sampleIndex) / config.sampleRate;

            while (eventIndex < static_cast<int>(snapshot.events.size())
                   && snapshot.events[static_cast<size_t>(eventIndex)].event.startSeconds <= t)
            {
                const auto& sched = snapshot.events[static_cast<size_t>(eventIndex)];
                const auto& e = sched.event;
                voices[static_cast<size_t>(sched.lane)].trigger(
                    static_cast<float>(e.durationSeconds),
                    e.pitchOffsetSemitones,
                    e.velocity);
                samplesUntilNoteOff[static_cast<size_t>(sched.lane)]
                    = static_cast<int>(e.durationSeconds * config.sampleRate);
                ++eventIndex;
            }

            for (int lane = 0; lane < Pattern::kNumLanes; ++lane)
            {
                int& count = samplesUntilNoteOff[static_cast<size_t>(lane)];
                if (count > 0 && --count == 0)
                    voices[static_cast<size_t>(lane)].noteOff();
            }

            float s = 0.0f;
            for (auto& v : voices)
                s += v.processSample();
            buffer.setSample(0, sampleIndex, s);
        }

        // ---- Tail ----------------------------------------------
        // Match the live engine's non-looped behaviour: at pattern
        // end every voice receives noteOff, then we keep rendering
        // the release tails until none of the four are active.
        for (auto& v : voices)
            if (v.isActive())
                v.noteOff();

        for (; sampleIndex < buffer.getNumSamples() && anyActive(voices); ++sampleIndex)
        {
            float s = 0.0f;
            for (auto& v : voices)
                s += v.processSample();
            buffer.setSample(0, sampleIndex, s);
        }

        // Trim to whatever we actually filled.
        buffer.setSize(1, sampleIndex, /*keepExistingContent=*/true,
                       /*clearExtraSpace=*/false, /*avoidReallocating=*/false);
        return buffer;
    }
}
