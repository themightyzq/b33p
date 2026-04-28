#include "ParameterRandomizer.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    namespace
    {
        // Per-parameter ceiling for "what fraction of the full
        // normalised range can a roll touch?". 1.0 = no cap.
        // Manual editing always has the full parameter range; only
        // rolls are clamped.
        //
        // amp_release is capped at 100 ms because longer random
        // releases bleed into following beeps and made the user's
        // patterns sound smeared / unintentional.
        float randomizationCeiling(const juce::String& id,
                                    juce::AudioProcessorValueTreeState& apvts)
        {
            if (id.endsWith("_amp_release"))
                if (auto* p = apvts.getParameter(id))
                    return p->getNormalisableRange().convertTo0to1(0.1f);
            return 1.0f;
        }
    }

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

        const float ceiling = randomizationCeiling(parameterID, apvts);
        param->setValueNotifyingHost(rng.nextFloat() * ceiling);
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
            apvts.undoManager->beginNewTransaction("Randomize All Lanes");

        for (juce::AudioProcessorParameter* p : apvts.processor.getParameters())
        {
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
            {
                if (! ParameterIDs::isRandomizable(withId->paramID))
                    continue;   // e.g. voiceGain — don't blast ears
                rollOneNoTransaction(withId->paramID, rng);
            }
        }
    }

    void ParameterRandomizer::rollMany(const std::vector<juce::String>& parameterIDs,
                                        juce::Random& rng,
                                        const juce::String& transactionName)
    {
        if (apvts.undoManager != nullptr)
            apvts.undoManager->beginNewTransaction(transactionName);

        for (const auto& id : parameterIDs)
        {
            if (! ParameterIDs::isRandomizable(id))
                continue;
            rollOneNoTransaction(id, rng);
        }
    }
}
