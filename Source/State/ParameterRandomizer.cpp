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

    std::vector<juce::String> ParameterRandomizer::getLockedParameterIDs() const
    {
        std::vector<juce::String> out;
        out.reserve(lockedIds.size());
        for (const auto& id : lockedIds)
            out.emplace_back(id);
        return out;
    }

    void ParameterRandomizer::clearAllLocks()
    {
        lockedIds.clear();
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
            apvts.undoManager->beginNewTransaction("Dice All Lanes");

        for (juce::AudioProcessorParameter* p : apvts.processor.getParameters())
        {
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
                rollOneNoTransaction(withId->paramID, rng);
        }
    }

    void ParameterRandomizer::rollMany(const std::vector<juce::String>& parameterIDs,
                                        juce::Random& rng,
                                        const juce::String& transactionName)
    {
        if (apvts.undoManager != nullptr)
            apvts.undoManager->beginNewTransaction(transactionName);

        for (const auto& id : parameterIDs)
            rollOneNoTransaction(id, rng);
    }
}
