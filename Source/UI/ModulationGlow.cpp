#include "ModulationGlow.h"

#include "Core/ParameterIDs.h"

#include <algorithm>
#include <cmath>

namespace B33p::ModulationGlow
{
    float computeMatrixIntensity(juce::AudioProcessorValueTreeState& apvts,
                                 int lane,
                                 ModDestination destination,
                                 float lfo1Value,
                                 float lfo2Value)
    {
        if (destination == ModDestination::None)
            return 0.0f;

        const float lfoMagnitudes[kNumLfosPerLane] = {
            std::fabs(juce::jlimit(-1.0f, 1.0f, lfo1Value)),
            std::fabs(juce::jlimit(-1.0f, 1.0f, lfo2Value)),
        };

        // Matches the live audio path's reading of `mod_slotN_source` /
        // `mod_slotN_dest` (int choices) and `mod_slotN_amount` (real
        // -1..1) in `B33pProcessor::pushParametersToLane`. We sum the
        // magnitudes so simultaneous LFOs add up to a brighter glow, then
        // cap at 1 — stacked-on-the-same-destination patches don't get
        // proportionally noisier.
        float intensity = 0.0f;
        for (int slot = 0; slot < kNumModSlots; ++slot)
        {
            const auto* destParam   = apvts.getRawParameterValue(ParameterIDs::modSlotDest  (lane, slot));
            const auto* sourceParam = apvts.getRawParameterValue(ParameterIDs::modSlotSource(lane, slot));
            const auto* amountParam = apvts.getRawParameterValue(ParameterIDs::modSlotAmount(lane, slot));
            if (destParam == nullptr || sourceParam == nullptr || amountParam == nullptr)
                continue;

            const auto slotDest = static_cast<ModDestination>(
                juce::jlimit(0, kNumModDestinations - 1,
                             static_cast<int>(destParam->load())));
            if (slotDest != destination)
                continue;

            const auto slotSource = static_cast<ModSource>(
                juce::jlimit(0, kNumModSources - 1,
                             static_cast<int>(sourceParam->load())));
            if (slotSource == ModSource::None)
                continue;

            // Source enum: None=0, LFO1=1, LFO2=2. Translate to the
            // mirror-value array index (LFO1 → 0, LFO2 → 1).
            const int lfoIndex = static_cast<int>(slotSource) - 1;
            if (lfoIndex < 0 || lfoIndex >= kNumLfosPerLane)
                continue;

            const float amountMag = std::fabs(amountParam->load());
            intensity += lfoMagnitudes[lfoIndex] * amountMag;
        }
        return std::min(1.0f, intensity);
    }
}
