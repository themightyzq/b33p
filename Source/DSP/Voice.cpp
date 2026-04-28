#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    void Voice::prepare(double sampleRate)
    {
        oscillator.prepare(sampleRate);
        ampEnvelope.prepare(sampleRate);
        pitchEnvelope.prepare(sampleRate);
        filter.prepare(sampleRate);
        bitcrush.prepare(sampleRate);
        distortion.prepare(sampleRate);
        prepared = true;
    }

    void Voice::reset()
    {
        oscillator.reset();
        ampEnvelope.reset();
        pitchEnvelope.reset();
        filter.reset();
        bitcrush.reset();
        distortion.reset();
    }

    void Voice::setWaveform(Oscillator::Waveform waveform)
    {
        oscillator.setWaveform(waveform);
    }

    void Voice::setBasePitchHz(float hz)
    {
        basePitchHz = std::clamp(hz, 20.0f, 20000.0f);
    }

    void Voice::setCustomWaveformTable(const std::vector<float>& samples)
    {
        oscillator.setCustomTable(samples);
    }

    void Voice::setAmpAttack(float seconds)   { ampEnvelope.setAttack(seconds);   }
    void Voice::setAmpDecay(float seconds)    { ampEnvelope.setDecay(seconds);    }
    void Voice::setAmpSustain(float level)    { ampEnvelope.setSustain(level);    }
    void Voice::setAmpRelease(float seconds)  { ampEnvelope.setRelease(seconds);  }

    void Voice::setPitchCurve(const std::vector<PitchEnvelopePoint>& points)
    {
        pitchEnvelope.setCurve(points);
    }

    void Voice::setFilterCutoff(float hz)     { filter.setCutoff(hz);     }
    void Voice::setFilterResonance(float q)   { filter.setResonance(q);   }

    void Voice::setBitcrushBitDepth(float bits)        { bitcrush.setBitDepth(bits);         }
    void Voice::setBitcrushSampleRate(float targetHz)  { bitcrush.setTargetSampleRate(targetHz); }

    void Voice::setDistortionDrive(float drive)        { distortion.setDrive(drive);         }

    void Voice::setGain(float linearGain)
    {
        gain = std::clamp(linearGain, 0.0f, 10.0f);
    }

    void Voice::trigger(float durationSeconds, float pitchOffsetSt, float velocity)
    {
        pitchOffsetSemitones = pitchOffsetSt;
        triggerVelocity      = std::clamp(velocity, 0.0f, 1.0f);
        pitchEnvelope.trigger(durationSeconds);
        ampEnvelope.noteOn();
    }

    void Voice::noteOff()
    {
        ampEnvelope.noteOff();
    }

    bool Voice::isActive() const
    {
        return ampEnvelope.isActive();
    }

    float Voice::processSample()
    {
        if (! prepared)
            return 0.0f;

        const float envSemitones   = pitchEnvelope.processSample();
        const float totalSemitones = pitchOffsetSemitones + envSemitones;
        const float frequency      = basePitchHz * std::exp2(totalSemitones / 12.0f);
        oscillator.setFrequency(frequency);

        float sample = oscillator.processSample();
        sample *= ampEnvelope.processSample();
        sample  = filter.processSample(sample);
        sample  = bitcrush.processSample(sample);
        sample  = distortion.processSample(sample);
        sample *= gain * triggerVelocity;

        return sample;
    }
}
