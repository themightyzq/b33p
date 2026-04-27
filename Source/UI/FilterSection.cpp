#include "FilterSection.h"

#include "Core/ParameterIDs.h"
#include "SliderFormatting.h"

namespace B33p
{
    FilterSection::FilterSection(B33pProcessor& processor)
        : Section("Filter"),
          cutoffAttachment   (processor.getApvts(), ParameterIDs::filterCutoffHz,   cutoffSlider.getSlider()),
          resonanceAttachment(processor.getApvts(), ParameterIDs::filterResonanceQ, resonanceSlider.getSlider())
    {
        addAndMakeVisible(cutoffSlider);
        addAndMakeVisible(resonanceSlider);

        cutoffSlider   .attachRandomizer(processor, ParameterIDs::filterCutoffHz);
        resonanceSlider.attachRandomizer(processor, ParameterIDs::filterResonanceQ);

        SliderFormatting::applyHz     (cutoffSlider   .getSlider());
        SliderFormatting::applyDecimal(resonanceSlider.getSlider(), 2);
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
