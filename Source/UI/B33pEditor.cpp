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

        // Resize limits (REVIEW.md P12). Without these a user could shrink the
        // window until the section headers are unreadable or balloon it past
        // any sane size. The minimum width keeps the Pattern toolbar's wrapped
        // (two-row) layout legible; the minimum height is kept low enough that
        // the standalone fit-to-screen clamp can still shrink the window onto a
        // small display without fighting the constrainer. The standalone window
        // and plugin hosts both honour the editor's constrainer.
        setResizeLimits(1000, 600, 3200, 2200);
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
