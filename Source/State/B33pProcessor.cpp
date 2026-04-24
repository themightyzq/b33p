#include "B33pProcessor.h"

#include "ParameterLayout.h"

namespace B33p
{
    B33pProcessor::B33pProcessor()
        : juce::AudioProcessor(BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::mono(), true))
        , apvts(*this, &undoManager, "B33pParameters", createParameterLayout())
    {
    }

    void B33pProcessor::prepareToPlay(double sampleRate, int /*blockSize*/)
    {
        voice.prepare(sampleRate);
    }

    void B33pProcessor::releaseResources()
    {
    }

    void B33pProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
    {
        // No audio device, no audition path yet — silent output.
        // The Voice is held but not driven; later tasks will copy
        // APVTS values into Voice and render its samples here.
        buffer.clear();
    }
}
