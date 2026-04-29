#include "ParameterRandomizer.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    namespace
    {
        // Per-parameter ceiling for "what fraction of the full
        // normalised range can a roll touch?". 1.0 = no cap.
        // Manual editing always has the full parameter range; only
        // rolls are clamped. Caps:
        //   amp_release       -> 100 ms (longer tails bleed into
        //                        the next beep)
        //   amp_attack        -> 500 ms (longer attacks make short
        //                        events nearly inaudible)
        //   filter_resonance  -> Q 5     (above ~10 the filter
        //                        self-oscillates into painful
        //                        whistles)
        //   distortion_drive  -> 20      (drive > 50 produces
        //                        ear-shredding hard-clip fuzz)
        struct Cap { const char* suffix; float maxRealValue; };
        constexpr Cap kCaps[] = {
            { "_amp_release",       0.1f  },
            { "_amp_attack",        0.5f  },
            { "_filter_resonance_q", 5.0f },
            { "_distortion_drive",  20.0f },
        };

        float randomizationCeiling(const juce::String& id,
                                    juce::AudioProcessorValueTreeState& apvts)
        {
            for (const auto& cap : kCaps)
            {
                if (! id.endsWith(cap.suffix))
                    continue;
                if (auto* p = apvts.getParameter(id))
                    return p->getNormalisableRange().convertTo0to1(cap.maxRealValue);
                break;
            }
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

        // Per-roll randomization scope: 1.0 = full random across
        // [0, ceiling] (legacy behaviour); smaller values constrain
        // the roll to a window centred on the parameter's current
        // normalised value, so the user can dial how "wild" the
        // dice get without locking parameters individually.
        float scope = 1.0f;
        if (auto* scopeParam = apvts.getRawParameterValue(
                ParameterIDs::randomizationScope()))
            scope = juce::jlimit(0.05f, 1.0f, scopeParam->load());

        if (scope >= 0.999f)
        {
            // Fast path — preserves the original "uniform across
            // the full range" behaviour exactly so existing tests
            // and rolls don't shift their distribution.
            param->setValueNotifyingHost(rng.nextFloat() * ceiling);
            return true;
        }

        const float current    = param->getValue();   // already in 0..1
        const float halfWindow = scope * ceiling * 0.5f;
        const float lo = juce::jmax(0.0f,     current - halfWindow);
        const float hi = juce::jmin(ceiling,  current + halfWindow);
        const float rolled = lo + rng.nextFloat() * (hi - lo);
        param->setValueNotifyingHost(rolled);
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
