#include "B33pEditor.h"

#include "State/B33pProcessor.h"

namespace B33p
{
    B33pEditor::B33pEditor(B33pProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          mainComponent(processor)
    {
        // Install b33p's flat dark LookAndFeel as the process-wide default so
        // the editor AND the dialogs it spawns (Save/Discard, Export, preset
        // browser, About) all pick it up — those open as separate windows that
        // query the default, not mainComponent's. Scoped to this editor's
        // lifetime; relinquished in the destructor. In plugin mode this only
        // touches our own statically-linked JUCE, never the host or other
        // plugins.
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

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

    B33pEditor::~B33pEditor()
    {
        // Only relinquish the default if it's still ours — in a multi-instance
        // host another editor may have taken over since we set it.
        if (&juce::LookAndFeel::getDefaultLookAndFeel() == &lookAndFeel)
            juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
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
