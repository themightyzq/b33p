#pragma once

#include "DSP/PitchEnvelope.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p
{
    // Direct-manipulation editor for the pitch envelope breakpoint
    // curve. Reads and writes the processor's pitch curve.
    //
    // Interactions:
    //   * click empty space   -> add a new point at the cursor, drag it
    //   * click on a point    -> begin dragging that point
    //   * right-click a point -> delete it (ctrl-click on macOS is also
    //                            treated as a popup gesture)
    //
    // The horizontal axis is normalized time [0, 1]; the vertical axis
    // is semitones clamped to ±12 for display. Points outside that
    // range visually saturate at the edges but are stored as-is.
    //
    // Each gesture (click + drag + release, or right-click delete)
    // commits as a single UndoManager transaction by snapshotting
    // the curve in mouseDown and pushing one SetPitchCurveAction in
    // mouseUp if the result differs.
    class PitchEnvelopeEditor : public juce::Component
    {
    public:
        explicit PitchEnvelopeEditor(B33pProcessor& processor);

        void paint(juce::Graphics& g) override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;

    private:
        juce::Rectangle<float> plotArea() const;
        juce::Point<float>     toScreen(const PitchEnvelopePoint& p) const;
        PitchEnvelopePoint     fromScreen(juce::Point<float> screen) const;
        int                    hitTestPoint(juce::Point<float> screen) const;

        B33pProcessor& processor;
        int            draggingIndex { -1 };

        std::vector<PitchEnvelopePoint> gestureSnapshot;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchEnvelopeEditor)
    };
}
