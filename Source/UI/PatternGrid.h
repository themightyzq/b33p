#pragma once

#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstddef>

namespace B33p
{
    // Interactive sequencer canvas: four lanes stacked vertically, a
    // time ruler across the top, a narrow label column on the left,
    // and events drawn as filled rectangles that can be clicked to
    // create, dragged to move, edge-dragged to resize, and
    // delete-keyed to remove.
    //
    // Dragging is horizontal-only for MVP — the selected event stays
    // in its lane. Moving events between lanes is a post-MVP polish
    // item (delete-and-re-add is the workaround).
    //
    // Grid size is hardcoded at 100 ms for this commit; the next
    // Phase 4 commit introduces the grid-size / pattern-length
    // dropdowns and surfaces them here.
    class PatternGrid : public juce::Component
    {
    public:
        explicit PatternGrid(B33pProcessor& processor);

        // 0 (or negative) means "no snap" — clicks land at the exact
        // cursor position and grid lines are not drawn.
        void   setGridSeconds(double seconds);
        double getGridSeconds() const { return gridSeconds; }

        void paint(juce::Graphics& g) override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp  (const juce::MouseEvent& e) override;

        bool keyPressed(const juce::KeyPress& key) override;

    private:
        struct Selection
        {
            int         lane  { -1 };
            std::size_t index {  0 };

            bool valid() const { return lane >= 0; }
        };

        enum class DragMode { None, Move, Resize };

        struct HitResult
        {
            enum class Kind { None, Body, RightEdge } kind { Kind::None };
            int         lane  { -1 };
            std::size_t index {  0 };
        };

        juce::Rectangle<float> plotArea() const;
        juce::Rectangle<float> laneArea(int lane) const;
        juce::Rectangle<float> eventRect(int lane, const Event& e) const;

        float  secondsToX(double seconds) const;
        double xToSeconds(float x) const;
        int    yToLane(float y) const;
        double snapSeconds(double seconds) const;

        HitResult hitTestEvent(juce::Point<float> p) const;

        B33pProcessor& processor;
        double         gridSeconds { 0.1 };

        Selection selection;

        DragMode dragMode { DragMode::None };
        double   dragStartSeconds { 0.0 };
        Event    dragOriginalEvent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGrid)
    };
}
