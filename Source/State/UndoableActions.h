#pragma once

#include "B33pProcessor.h"
#include "ProjectState.h"
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
                         const juce::Component::SafePointer<juce::Component>& editorToRefresh,
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
            // Rebuild the snapshot immediately so the audio thread
            // sees the edit on its next block (within ~5 ms),
            // instead of waiting up to 33 ms for the next timer
            // tick — which is what made duplicated / dragged
            // events sometimes go silent for one loop iteration.
            processor.refreshPatternSnapshot();
            // notifyFullStateLoaded fires the message-thread callback
            // that re-syncs widgets that don't auto-track the pattern
            // — length combo, loop toggle, lane name labels, mute
            // buttons. Cheap (callAsync coalesces duplicates).
            processor.notifyFullStateLoaded();
            if (auto* c = editor.getComponent())
                c->repaint();
            return true;
        }

        bool undo() override
        {
            processor.getPattern() = beforeState;
            processor.markDirty();
            processor.refreshPatternSnapshot();
            processor.notifyFullStateLoaded();
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
                            const juce::Component::SafePointer<juce::Component>& editorToRefresh,
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

    // Whole-project state swap as one undoable step (REVIEW.md P6). Loading
    // a preset replaces the entire project (parameters, pattern, pitch curve,
    // wavetables, locks), so undo restores the full pre-load snapshot rather
    // than just one field. Both snapshots are raw ProjectState ValueTrees;
    // each apply runs them through ProjectState::load on a fresh copy (load
    // calls migrate, which mutates, so the stored snapshots must not be
    // consumed).
    class LoadProjectStateAction : public juce::UndoableAction
    {
    public:
        LoadProjectStateAction(B33pProcessor& p,
                               const juce::Component::SafePointer<juce::Component>& editorToRefresh,
                               juce::ValueTree beforeTree,
                               juce::ValueTree afterTree)
            : processor(p),
              editor(editorToRefresh),
              before(std::move(beforeTree)),
              after(std::move(afterTree))
        {}

        bool perform() override { return apply(after); }
        bool undo()    override { return apply(before); }

        int getSizeInUnits() override { return 2048; }

    private:
        bool apply(const juce::ValueTree& tree)
        {
            const bool ok = ProjectState::load(processor, tree.createCopy());
            if (ok)
                if (auto* c = editor.getComponent())
                    c->repaint();
            return ok;
        }

        B33pProcessor& processor;
        juce::Component::SafePointer<juce::Component> editor;
        juce::ValueTree before;
        juce::ValueTree after;
    };
}
