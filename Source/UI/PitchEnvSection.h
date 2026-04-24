#pragma once

#include "PitchEnvelopeEditor.h"
#include "Section.h"
#include "State/B33pProcessor.h"

namespace B33p
{
    class PitchEnvSection : public Section
    {
    public:
        explicit PitchEnvSection(B33pProcessor& processor);

        void resized() override;

    private:
        PitchEnvelopeEditor editor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchEnvSection)
    };
}
