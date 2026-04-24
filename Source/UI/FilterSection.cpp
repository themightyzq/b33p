#include "FilterSection.h"

#include "Core/ParameterIDs.h"

namespace B33p
{
    FilterSection::FilterSection(juce::AudioProcessorValueTreeState& apvts)
        : Section("Filter"),
          cutoffAttachment   (apvts, ParameterIDs::filterCutoffHz,     cutoffSlider.getSlider()),
          resonanceAttachment(apvts, ParameterIDs::filterResonanceQ,   resonanceSlider.getSlider())
    {
        addAndMakeVisible(cutoffSlider);
        addAndMakeVisible(resonanceSlider);
    }

    void FilterSection::resized()
    {
        auto bounds = getContentBounds();

        constexpr int kGap = 8;
        const int cellWidth = (bounds.getWidth() - kGap) / 2;

        cutoffSlider.setBounds(bounds.removeFromLeft(cellWidth));
        bounds.removeFromLeft(kGap);
        resonanceSlider.setBounds(bounds);
    }
}
