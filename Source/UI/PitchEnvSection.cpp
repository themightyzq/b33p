#include "PitchEnvSection.h"

namespace B33p
{
    PitchEnvSection::PitchEnvSection(B33pProcessor& processor)
        : Section("Pitch Envelope"),
          editor(processor)
    {
        // Suffix mirrors the "(Lane N)" tag the voice sections use,
        // so the user understands the asymmetry: the curve isn't
        // per-lane like the voice params are.
        setTitleSuffix(" (shared across all lanes)");
        addAndMakeVisible(editor);
    }

    void PitchEnvSection::resized()
    {
        editor.setBounds(getContentBounds());
    }

    void PitchEnvSection::refreshFromState()
    {
        editor.repaint();
    }
}
