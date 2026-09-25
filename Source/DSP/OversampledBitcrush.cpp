#include "OversampledBitcrush.h"

#include <algorithm>

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
        // processBlock() chunks at kMaxOversampledBlockSize, so size the
        // oversampler's internal buffers for that -- one initProcessing()
        // call here at prepare() time, never resized from the audio
        // thread afterward.
        oversampler.initProcessing(static_cast<size_t>(kMaxOversampledBlockSize));
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

    void OversampledBitcrush::processBlock(float* data, int numSamples)
    {
        if (! prepared)
        {
            std::fill(data, data + numSamples, 0.0f);
            return;
        }

        if (! oversamplingEnabled)
        {
            for (int i = 0; i < numSamples; ++i)
                data[i] = bitcrush.processSample(data[i]);
            return;
        }

        int offset = 0;
        while (offset < numSamples)
        {
            const int chunk = std::min(numSamples - offset, kMaxOversampledBlockSize);

            for (int i = 0; i < chunk; ++i)
                osIn.setSample(0, i, data[offset + i]);

            const juce::dsp::AudioBlock<const float> inBlock(
                juce::dsp::AudioBlock<const float>(osIn).getSubBlock(0, static_cast<size_t>(chunk)));
            auto upBlock = oversampler.processSamplesUp(inBlock);

            // Advance the bit-depth/rate smoothers exactly once per
            // external (host-rate) sample -- their 30 ms ramps were set
            // up in Bitcrush::prepare() against the host sample rate --
            // and recompute the phase increment against the OVERSAMPLED
            // tick rate (host rate * kOversamplingFactor). step() below
            // then runs once per oversampled tick, so the "hold" still
            // only captures a new sample targetHz times per real second:
            // the audible reduction ratio for a given Rate setting is
            // unchanged by the oversampling factor, per the CLAUDE.md/
            // task requirement that a preset saved at 48 kHz must sound
            // the same.
            for (int j = 0; j < chunk; ++j)
            {
                bitcrush.beginOversampledTick(sampleRate * static_cast<double>(kOversamplingFactor));
                const int idx0 = j * kOversamplingFactor;
                for (int k = 0; k < kOversamplingFactor; ++k)
                {
                    const int idx = idx0 + k;
                    upBlock.setSample(0, idx, bitcrush.step(upBlock.getSample(0, idx)));
                }
            }

            juce::dsp::AudioBlock<float> outBlock(
                juce::dsp::AudioBlock<float>(osOut).getSubBlock(0, static_cast<size_t>(chunk)));
            oversampler.processSamplesDown(outBlock);

            for (int i = 0; i < chunk; ++i)
                data[offset + i] = osOut.getSample(0, i);

            offset += chunk;
        }
    }

    float OversampledBitcrush::processSample(float input)
    {
        processBlock(&input, 1);
        return input;
    }
}
