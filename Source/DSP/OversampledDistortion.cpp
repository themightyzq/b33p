#include "OversampledDistortion.h"

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

    OversampledDistortion::OversampledDistortion()
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

    void OversampledDistortion::prepare(double sampleRate)
    {
        distortion.prepare(sampleRate);
        // processBlock() chunks at kMaxOversampledBlockSize, so size the
        // oversampler's internal buffers for that -- one initProcessing()
        // call here at prepare() time, never resized from the audio
        // thread afterward.
        oversampler.initProcessing(static_cast<size_t>(kMaxOversampledBlockSize));
        oversampler.reset();
        prepared = true;
    }

    void OversampledDistortion::reset()
    {
        distortion.reset();
        oversampler.reset();
    }

    void OversampledDistortion::setDrive(float drive)
    {
        distortion.setDrive(drive);
    }

    void OversampledDistortion::setOversamplingEnabledForTests(bool enabled)
    {
        oversamplingEnabled = enabled;
    }

    int OversampledDistortion::getLatencySamples() const
    {
        if (! prepared || ! oversamplingEnabled)
            return 0;
        return juce::roundToInt(oversampler.getLatencyInSamples());
    }

    void OversampledDistortion::processBlock(float* data, int numSamples)
    {
        if (! prepared)
        {
            std::fill(data, data + numSamples, 0.0f);
            return;
        }

        if (! oversamplingEnabled)
        {
            for (int i = 0; i < numSamples; ++i)
                data[i] = distortion.processSample(data[i]);
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

            // Advance the drive smoother exactly once per external (host-
            // rate) sample -- its 20 ms ramp was set up in Distortion::
            // prepare() against the host sample rate -- and hold that one
            // smoothed value across every oversampled tick belonging to
            // that sample. Calling Distortion::processSample() per tick
            // instead would advance the smoother kOversamplingFactor
            // steps for every 1 real sample and finish the ramp that
            // many times faster than intended.
            for (int j = 0; j < chunk; ++j)
            {
                const int idx0 = j * kOversamplingFactor;
                upBlock.setSample(0, idx0, distortion.processSample(upBlock.getSample(0, idx0)));
                const float heldDrive = distortion.getCurrentDrive();
                for (int k = 1; k < kOversamplingFactor; ++k)
                {
                    const int idx = idx0 + k;
                    upBlock.setSample(0, idx, Distortion::shape(upBlock.getSample(0, idx), heldDrive));
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

    float OversampledDistortion::processSample(float input)
    {
        processBlock(&input, 1);
        return input;
    }
}
