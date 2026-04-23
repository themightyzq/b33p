#include "AmpEnvelope.h"

#include <algorithm>

namespace B33p
{
    void AmpEnvelope::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void AmpEnvelope::reset()
    {
        stage          = Stage::Idle;
        currentLevel   = 0.0f;
        stageIncrement = 0.0f;
    }

    void AmpEnvelope::setAttack(float seconds)
    {
        attackSeconds = std::max(0.0f, seconds);
    }

    void AmpEnvelope::setDecay(float seconds)
    {
        decaySeconds = std::max(0.0f, seconds);
    }

    void AmpEnvelope::setSustain(float level)
    {
        sustainLevel = std::clamp(level, 0.0f, 1.0f);
    }

    void AmpEnvelope::setRelease(float seconds)
    {
        releaseSeconds = std::max(0.0f, seconds);
    }

    void AmpEnvelope::noteOn()
    {
        if (sampleRate <= 0.0)
            return;

        beginAttack();
    }

    void AmpEnvelope::noteOff()
    {
        if (sampleRate <= 0.0 || stage == Stage::Idle)
            return;

        beginRelease();
    }

    bool AmpEnvelope::isActive() const
    {
        return stage != Stage::Idle;
    }

    void AmpEnvelope::beginAttack()
    {
        stage = Stage::Attack;

        const double samples = static_cast<double>(attackSeconds) * sampleRate;

        // A zero- or sub-sample attack forces an instantaneous jump to peak on
        // the next processSample(). 1.0 is guaranteed to exceed currentLevel
        // (which is clamped to [0, 1]), so the Attack branch will clamp to 1.0
        // and transition straight into Decay.
        stageIncrement = samples <= 0.0
            ? 1.0f
            : static_cast<float>((1.0 - static_cast<double>(currentLevel)) / samples);
    }

    void AmpEnvelope::beginDecay()
    {
        // If decay would be a no-op (already at or below sustain), skip it so
        // the envelope doesn't spend a sample in a stage with zero slope.
        if (currentLevel <= sustainLevel)
        {
            currentLevel   = sustainLevel;
            stage          = Stage::Sustain;
            stageIncrement = 0.0f;
            return;
        }

        stage = Stage::Decay;

        const double samples = static_cast<double>(decaySeconds) * sampleRate;

        stageIncrement = samples <= 0.0
            ? -1.0f
            : static_cast<float>((static_cast<double>(sustainLevel) - 1.0) / samples);
    }

    void AmpEnvelope::beginRelease()
    {
        stage = Stage::Release;

        const double samples = static_cast<double>(releaseSeconds) * sampleRate;

        stageIncrement = samples <= 0.0
            ? -1.0f
            : static_cast<float>(-static_cast<double>(currentLevel) / samples);
    }

    float AmpEnvelope::processSample()
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
                currentLevel += stageIncrement;
                if (currentLevel >= 1.0f)
                {
                    currentLevel = 1.0f;
                    beginDecay();
                }
                return currentLevel;

            case Stage::Decay:
                currentLevel += stageIncrement;
                if (currentLevel <= sustainLevel)
                {
                    currentLevel   = sustainLevel;
                    stage          = Stage::Sustain;
                    stageIncrement = 0.0f;
                }
                return currentLevel;

            case Stage::Sustain:
                return currentLevel;

            case Stage::Release:
                currentLevel += stageIncrement;
                if (currentLevel <= 0.0f)
                {
                    currentLevel   = 0.0f;
                    stage          = Stage::Idle;
                    stageIncrement = 0.0f;
                }
                return currentLevel;
        }

        return 0.0f;
    }
}
