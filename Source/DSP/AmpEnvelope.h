#pragma once

namespace B33p
{
    // Linear-ramp ADSR amplitude envelope producing a per-sample multiplier in
    // [0, 1]. Voice multiplies oscillator output by this envelope's output.
    //
    // Retrigger: calling noteOn() while non-idle restarts the attack stage
    // from the current level rather than snapping to zero. This prevents
    // clicks when a pattern event fires while a prior event is still ringing.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setAttack/Decay/Sustain/
    // Release -> noteOn() -> processSample() ... -> noteOff() -> processSample()
    // ... until isActive() returns false. Before prepare() is called,
    // processSample() returns silence (0.0f).
    class AmpEnvelope
    {
    public:
        enum class Stage
        {
            Idle,
            Attack,
            Decay,
            Sustain,
            Release
        };

        void prepare(double sampleRate);
        void reset();

        void setAttack(float seconds);
        void setDecay(float seconds);
        void setSustain(float level);
        void setRelease(float seconds);

        void noteOn();
        void noteOff();

        float processSample();

        bool isActive() const;

    private:
        void beginAttack();
        void beginDecay();
        void beginRelease();

        double sampleRate     { 0.0 };

        float  attackSeconds  { 0.0f };
        float  decaySeconds   { 0.0f };
        float  sustainLevel   { 1.0f };
        float  releaseSeconds { 0.0f };

        Stage  stage          { Stage::Idle };
        float  currentLevel   { 0.0f };
        float  stageIncrement { 0.0f };
    };
}
