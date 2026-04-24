#pragma once

#include "UI/Section.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    class MainComponent : public juce::Component
    {
    public:
        MainComponent();
        ~MainComponent() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        Section oscillatorSection  { "Oscillator"   };
        Section ampEnvelopeSection { "Amp Envelope" };
        Section filterSection      { "Filter"       };
        Section effectsSection     { "Effects"      };
        Section masterSection      { "Master"       };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
    };
}
