#include "PitchEnvelope.h"

#include <algorithm>

namespace B33p
{
    void PitchEnvelope::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void PitchEnvelope::reset()
    {
        stage         = Stage::Idle;
        currentTime   = 0.0;
        timeIncrement = 0.0;
    }

    void PitchEnvelope::setCurve(const std::vector<PitchEnvelopePoint>& points)
    {
        if (points.empty())
        {
            curve = { { 0.0f, 0.0f } };
            return;
        }

        curve = points;

        for (auto& p : curve)
            p.normalizedTime = std::clamp(p.normalizedTime, 0.0f, 1.0f);

        std::stable_sort(curve.begin(), curve.end(),
            [](const PitchEnvelopePoint& a, const PitchEnvelopePoint& b)
            {
                return a.normalizedTime < b.normalizedTime;
            });
    }

    void PitchEnvelope::trigger(float durationSeconds)
    {
        if (sampleRate <= 0.0)
            return;

        // Duration <= 0 means the curve has "already finished" — first
        // processSample() should return the last point's value, not the
        // first. This avoids a one-sample glitch of the starting pitch.
        if (durationSeconds <= 0.0f)
        {
            currentTime   = 1.0;
            timeIncrement = 0.0;
            stage         = Stage::Holding;
            return;
        }

        const double totalSamples = static_cast<double>(durationSeconds) * sampleRate;

        timeIncrement = 1.0 / totalSamples;
        currentTime   = 0.0;
        stage         = Stage::Active;
    }

    bool PitchEnvelope::isActive() const
    {
        return stage == Stage::Active;
    }

    float PitchEnvelope::processSample()
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Holding:
                return interpolateAt(1.0);

            case Stage::Active:
            {
                const float out = interpolateAt(currentTime);
                currentTime += timeIncrement;
                if (currentTime >= 1.0)
                {
                    currentTime = 1.0;
                    stage       = Stage::Holding;
                }
                return out;
            }
        }

        return 0.0f;
    }

    float PitchEnvelope::interpolateAt(double normalizedTime) const
    {
        // setCurve guarantees curve is non-empty, but be defensive.
        if (curve.empty())
            return 0.0f;

        const double t = std::clamp(normalizedTime, 0.0, 1.0);

        if (t <= static_cast<double>(curve.front().normalizedTime))
            return curve.front().semitones;

        if (t >= static_cast<double>(curve.back().normalizedTime))
            return curve.back().semitones;

        // Linear search is fine: drawn curves typically have a handful of
        // points, not hundreds. Upgrade to binary search if profiling ever
        // says otherwise.
        for (size_t i = 1; i < curve.size(); ++i)
        {
            const PitchEnvelopePoint& b = curve[i];
            if (static_cast<double>(b.normalizedTime) >= t)
            {
                const PitchEnvelopePoint& a = curve[i - 1];
                const double span = static_cast<double>(b.normalizedTime)
                                  - static_cast<double>(a.normalizedTime);

                // Duplicate times encode a vertical step; take the later
                // point's value so the step is "land on the new value."
                if (span <= 0.0)
                    return b.semitones;

                const double alpha = (t - static_cast<double>(a.normalizedTime)) / span;
                return static_cast<float>(
                    static_cast<double>(a.semitones)
                    + alpha * (static_cast<double>(b.semitones)
                               - static_cast<double>(a.semitones)));
            }
        }

        return curve.back().semitones;
    }
}
