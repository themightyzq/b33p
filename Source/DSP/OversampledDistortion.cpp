#include "OversampledDistortion.h"

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
        // Voice calls processSample() one sample at a time (see the
        // class comment), so every processSamplesUp/Down call this
        // wrapper makes is always a 1-sample block -- initProcessing(1)
        // sizes the internal buffers for exactly that, not the host's
        // block size (which this class never sees or needs).
        oversampler.initProcessing(1);
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

    float OversampledDistortion::processSample(float input)
    {
        if (! prepared)
            return 0.0f;

        if (! oversamplingEnabled)
            return distortion.processSample(input);

        osIn.setSample(0, 0, input);
        const juce::dsp::AudioBlock<const float> inBlock(osIn);
        auto upBlock = oversampler.processSamplesUp(inBlock);

        // Advance the drive smoother exactly once per external (host-
        // rate) sample -- its 20 ms ramp was set up in Distortion::
        // prepare() against the host sample rate -- and hold that one
        // smoothed value across every oversampled tick in this block.
        // Calling Distortion::processSample() per tick instead would
        // advance the smoother 4 steps for every 1 real sample and
        // finish the ramp 4x faster than intended.
        upBlock.setSample(0, 0, distortion.processSample(upBlock.getSample(0, 0)));
        const float heldDrive = distortion.getCurrentDrive();
        for (size_t i = 1; i < upBlock.getNumSamples(); ++i)
        {
            const int idx = static_cast<int>(i);
            upBlock.setSample(0, idx, Distortion::shape(upBlock.getSample(0, idx), heldDrive));
        }

        juce::dsp::AudioBlock<float> outBlock(osOut);
        oversampler.processSamplesDown(outBlock);
        return osOut.getSample(0, 0);
    }
}
