#pragma once

#include <vector>

namespace B33p
{
    // A single breakpoint in a drawn pitch curve. normalizedTime is a
    // position along the curve in [0, 1], not an absolute time — the
    // curve is stretched to fit the per-trigger duration at playback.
    struct PitchEnvelopePoint
    {
        float normalizedTime;
        float semitones;
    };

    // User-drawn arbitrary pitch curve, played back over a per-trigger
    // duration. Output is a semitone offset; Voice combines it with
    // base pitch and event pitch offset to produce the final frequency.
    //
    // The curve is stored in normalized time so the same shape can play
    // over any duration. Linear interpolation between points; curve
    // types beyond linear are post-MVP.
    //
    // Post-duration: once playback reaches t=1 the envelope holds at the
    // last point's semitone value and isActive() returns false. The amp
    // envelope decides when the voice ends — snapping pitch here would
    // re-articulate a still-ringing tail.
    //
    // setCurve is tolerant: empty input becomes a constant-zero curve;
    // points outside [0, 1] are clamped; out-of-order points are sorted;
    // duplicate times are preserved (producing an instantaneous step).
    // First / last points are not required to land on the endpoints —
    // time before the first point holds the first value, time after the
    // last holds the last.
    //
    // Lifecycle: construct -> prepare(sampleRate) -> setCurve -> trigger
    // -> processSample ... . Before prepare(), processSample() returns
    // silence (0.0f).
    class PitchEnvelope
    {
    public:
        enum class Stage
        {
            Idle,
            Active,
            Holding
        };

        void prepare(double sampleRate);
        void reset();

        void setCurve(const std::vector<PitchEnvelopePoint>& points);

        void trigger(float durationSeconds);

        float processSample();

        bool isActive() const;

    private:
        float interpolateAt(double normalizedTime) const;

        double                          sampleRate    { 0.0 };
        std::vector<PitchEnvelopePoint> curve         { { 0.0f, 0.0f } };
        Stage                           stage         { Stage::Idle };
        double                          currentTime   { 0.0 };
        double                          timeIncrement { 0.0 };
    };
}
