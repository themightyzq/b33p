#include "PitchEnvSection.h"

namespace B33p
{
    PitchEnvSection::PitchEnvSection(B33pProcessor& processor)
        : Section("Pitch Envelope"),
          editor(processor)
    {
        addAndMakeVisible(editor);
    }

    void PitchEnvSection::resized()
    {
        editor.setBounds(getContentBounds());
    }
}
