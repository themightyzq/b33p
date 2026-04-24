#include "ParameterRandomizer.h"

namespace B33p
{
    ParameterRandomizer::ParameterRandomizer(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
    {
    }

    bool ParameterRandomizer::isLocked(const juce::String& parameterID) const
    {
        return lockedIds.count(parameterID.toStdString()) > 0;
    }

    void ParameterRandomizer::setLocked(const juce::String& parameterID, bool locked)
    {
        if (locked)
            lockedIds.insert(parameterID.toStdString());
        else
            lockedIds.erase(parameterID.toStdString());
    }

    bool ParameterRandomizer::rollOneNoTransaction(const juce::String& parameterID,
                                                   juce::Random& rng)
    {
        if (isLocked(parameterID))
            return false;

        auto* param = apvts.getParameter(parameterID);
        if (param == nullptr)
            return false;

        param->setValueNotifyingHost(rng.nextFloat());
        return true;
    }

    bool ParameterRandomizer::rollOne(const juce::String& parameterID, juce::Random& rng)
    {
        if (apvts.undoManager != nullptr)
            apvts.undoManager->beginNewTransaction();

        return rollOneNoTransaction(parameterID, rng);
    }

    void ParameterRandomizer::rollAllUnlocked(juce::Random& rng)
    {
        if (apvts.undoManager != nullptr)
            apvts.undoManager->beginNewTransaction("Dice All");

        for (juce::AudioProcessorParameter* p : apvts.processor.getParameters())
        {
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
                rollOneNoTransaction(withId->paramID, rng);
        }
    }
}
