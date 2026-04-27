#pragma once

#include "B33pProcessor.h"
#include "Pattern/Pattern.h"
#include "DSP/PitchEnvelope.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include <vector>

namespace B33p
{
    // Snapshot-based undo for the pattern. mouseDown captures the
    // before-state, the UI mutates the live pattern directly during
    // the drag, then mouseUp wraps the gesture in one of these and
    // hands it to the UndoManager. The whole gesture collapses to
    // a single undo entry instead of one entry per drag frame.
    //
    // The repaint SafePointer is so undo / redo from the menu (or a
    // keyboard shortcut) refreshes the editor — the grid's timer
    // only repaints while playback is running.
    class SetPatternAction : public juce::UndoableAction
    {
    public:
        SetPatternAction(B33pProcessor& p,
                         juce::Component::SafePointer<juce::Component> editorToRefresh,
                         Pattern beforeSnapshot,
                         Pattern afterSnapshot)
            : processor(p),
              editor(editorToRefresh),
              beforeState(std::move(beforeSnapshot)),
              afterState(std::move(afterSnapshot))
        {}

        bool perform() override
        {
            processor.getPattern() = afterState;
            processor.markDirty();
            if (auto* c = editor.getComponent())
                c->repaint();
            return true;
        }

        bool undo() override
        {
            processor.getPattern() = beforeState;
            processor.markDirty();
            if (auto* c = editor.getComponent())
                c->repaint();
            return true;
        }

        int getSizeInUnits() override { return static_cast<int>(sizeof(*this)); }

    private:
        B33pProcessor& processor;
        juce::Component::SafePointer<juce::Component> editor;
        Pattern beforeState;
        Pattern afterState;
    };

    // Snapshot-based undo for the drawn pitch curve. setPitchCurve
    // already marks dirty internally, so this action only swaps the
    // vector and asks the editor to repaint.
    class SetPitchCurveAction : public juce::UndoableAction
    {
    public:
        SetPitchCurveAction(B33pProcessor& p,
                            juce::Component::SafePointer<juce::Component> editorToRefresh,
                            std::vector<PitchEnvelopePoint> beforeSnapshot,
                            std::vector<PitchEnvelopePoint> afterSnapshot)
            : processor(p),
              editor(editorToRefresh),
              beforeState(std::move(beforeSnapshot)),
              afterState(std::move(afterSnapshot))
        {}

        bool perform() override
        {
            processor.setPitchCurve(afterState);
            if (auto* c = editor.getComponent())
                c->repaint();
            return true;
        }

        bool undo() override
        {
            processor.setPitchCurve(beforeState);
            if (auto* c = editor.getComponent())
                c->repaint();
            return true;
        }

        int getSizeInUnits() override { return static_cast<int>(sizeof(*this)); }

    private:
        B33pProcessor& processor;
        juce::Component::SafePointer<juce::Component> editor;
        std::vector<PitchEnvelopePoint> beforeState;
        std::vector<PitchEnvelopePoint> afterState;
    };
}
