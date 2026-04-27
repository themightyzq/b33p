#pragma once

#include "Pattern/Pattern.h"
#include "PatternGrid.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace B33p
{
    // Thin horizontal strip below the pattern grid that surfaces the
    // selected event's properties (start, duration, pitch offset,
    // velocity, lane) for direct numeric editing, plus a Delete
    // button. When nothing is selected it shows a placeholder hint
    // and disables the controls.
    //
    // Edits are committed through the UndoManager via SetPatternAction
    // so each change collapses into one undo entry. Dragging an event
    // in the grid above also updates the inspector live — the parent
    // calls refresh() whenever the selected event mutates.
    class InspectorStrip : public juce::Component
    {
    public:
        explicit InspectorStrip(B33pProcessor& processor);

        // Identity change — the selection now points at a different
        // event (or to nothing). Re-reads everything and rebuilds the
        // lane combo's selected item, etc.
        void setSelection(const PatternGrid::Selection& selection);

        // Same selection, content changed — typical mid-drag refresh
        // from the grid. Cheaper than setSelection.
        void refresh();

        // User clicked the Delete button. Parent is responsible for
        // removing the event and calling setSelection({}).
        std::function<void()> onDeleteRequested;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        // Mutator runs against a copy of the selected event; the
        // resulting Pattern is pushed as a SetPatternAction so the
        // edit is undoable. No-op if the selection is invalid.
        void pushEdit(std::function<void(Event&)> mutator,
                       const juce::String& transactionName);

        // Pulls the currently-selected event out of the pattern.
        // Returns the default Event{} if the selection is invalid or
        // out-of-range (shouldn't happen in practice — the grid
        // always points at a real event when selection is valid).
        Event currentEvent() const;

        void writeFieldsFromEvent(const Event& event);
        void setControlsEnabled(bool enabled);

        B33pProcessor&         processor;
        PatternGrid::Selection currentSelection;

        juce::Label    placeholder;

        juce::Label    laneLabel,     startLabel,    durationLabel,
                        pitchLabel,    velocityLabel;
        juce::ComboBox laneCombo;
        juce::Slider   startSlider,   durationSlider, pitchSlider, velocitySlider;
        juce::TextButton deleteButton { "Delete" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorStrip)
    };
}
