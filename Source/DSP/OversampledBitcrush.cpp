#include "OversampledBitcrush.h"

namespace B33p
{
    namespace
    {
        // juce::dsp::Oversampling's constructor takes a stage count N
        // and gives 2^N times oversampling; kOversamplingFactor is the
        // literal multiplier, so derive N from it (it's a power of two).
        size_t oversamplingStageCount()
        {
            size_t stages = 0;
            for (int factor = kOversamplingFactor; factor > 1; factor >>= 1)
                ++stages;
            return stages;
        }
    }

    OversampledBitcrush::OversampledBitcrush()
        : oversampler(1,   // mono -- Voice is a monophonic signal path
                       oversamplingStageCount(),
                       juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                       true,    // isMaxQuality
                       true)    // useIntegerLatency -- getLatencySamples()
                                // below needs a whole-sample value to
                                // hand straight to AudioProcessor::
                                // setLatencySamples (which only accepts
                                // int), with no rounding drift stage vs.
                                // stage.
    {
    }

    void OversampledBitcrush::prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        bitcrush.prepare(newSampleRate);
        // Voice calls processSample() one sample at a time (see the
        // class comment), so every processSamplesUp/Down call this
        // wrapper makes is always a 1-sample block -- initProcessing(1)
        // sizes the internal buffers for exactly that, not the host's
        // block size (which this class never sees or needs).
        oversampler.initProcessing(1);
        oversampler.reset();
        prepared = true;
    }

    void OversampledBitcrush::reset()
    {
        bitcrush.reset();
        oversampler.reset();
    }

    void OversampledBitcrush::setBitDepth(float bits)       { bitcrush.setBitDepth(bits); }
    void OversampledBitcrush::setTargetSampleRate(float hz) { bitcrush.setTargetSampleRate(hz); }

    void OversampledBitcrush::setOversamplingEnabledForTests(bool enabled)
    {
        oversamplingEnabled = enabled;
    }

    int OversampledBitcrush::getLatencySamples() const
    {
        if (! prepared || ! oversamplingEnabled)
            return 0;
        return juce::roundToInt(oversampler.getLatencyInSamples());
    }

    float OversampledBitcrush::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        if (! oversamplingEnabled)
            return bitcrush.processSample(input);

        // Advance the bit-depth/rate smoothers exactly once per external
        // (host-rate) sample -- their 30 ms ramps were set up in
        // Bitcrush::prepare() against the host sample rate -- and
        // recompute the phase increment against the OVERSAMPLED tick
        // rate (host rate * kOversamplingFactor). step() below then runs
        // once per oversampled tick, so the "hold" still only captures a
        // new sample targetHz times per real second: the audible
        // reduction ratio for a given Rate setting is unchanged by the
        // oversampling factor, per the CLAUDE.md/task requirement that a
        // preset saved at 48 kHz must sound the same.
        bitcrush.beginOversampledTick(sampleRate * static_cast<double>(kOversamplingFactor));

        osIn.setSample(0, 0, input);
        const juce::dsp::AudioBlock<const float> inBlock(osIn);
        auto upBlock = oversampler.processSamplesUp(inBlock);

        for (size_t i = 0; i < upBlock.getNumSamples(); ++i)
        {
            const int idx = static_cast<int>(i);
            upBlock.setSample(0, idx, bitcrush.step(upBlock.getSample(0, idx)));
        }

        juce::dsp::AudioBlock<float> outBlock(osOut);
        oversampler.processSamplesDown(outBlock);
        return osOut.getSample(0, 0);
    }
}
