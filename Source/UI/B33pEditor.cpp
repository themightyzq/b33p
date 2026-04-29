#include "B33pEditor.h"

#include "State/B33pProcessor.h"

namespace B33p
{
    B33pEditor::B33pEditor(B33pProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          mainComponent(processor)
    {
        addAndMakeVisible(mainComponent);

        // MainComponent computes its preferred size in its own
        // constructor; mirror that as the editor's initial size so
        // the host opens the editor at the right dimensions on
        // first load. Hosts persist this value if the user resizes,
        // so re-opening picks up the user's last layout.
        setSize(mainComponent.getWidth(), mainComponent.getHeight());
        setResizable(true, true);
    }

    void B33pEditor::resized()
    {
        mainComponent.setBounds(getLocalBounds());
    }

    // Factory hook called by B33pProcessor::createEditor when
    // B33P_HAS_EDITOR is set on the build target. Keeps the
    // processor TU free of the UI dependency graph so the test
    // target can link without dragging in MainComponent.
    juce::AudioProcessorEditor* createB33pEditor(B33pProcessor& processor)
    {
        return new B33pEditor(processor);
    }
}
