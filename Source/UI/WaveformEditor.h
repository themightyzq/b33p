#pragma once

#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace B33p
{
    // Drawable single-cycle waveform editor. Shows the table that
    // the lane's Oscillator::Waveform::Custom mode reads, lets the
    // user click + drag to set per-sample values, and publishes
    // updates to the processor (which forwards to the audio thread
    // via the existing custom-waveform snapshot pattern).
    //
    // setLane(int) re-points the editor at a different lane's table;
    // an empty table is seeded with one cycle of sine so the user
    // always has something to draw on rather than starting from zero.
    class WaveformEditor : public juce::Component
    {
    public:
        explicit WaveformEditor(B33pProcessor& processor);

        void setLane(int lane);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

    private:
        void   editAt(int sampleIdx, float value);
        void   strokeBetween(int fromIdx, float fromValue,
                              int toIdx,   float toValue);
        int    xToSampleIdx(float x) const;
        float  yToValue(float y) const;
        void   publish();

        B33pProcessor&     processor;
        int                currentLane    { 0 };
        std::vector<float> table;
        int                lastDragSample { -1 };
        float              lastDragValue  { 0.0f };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformEditor)
    };

    // Free-floating popup window that hosts the WaveformEditor.
    // Closes by hiding (the OscillatorSection owns it as a
    // unique_ptr and re-shows on demand).
    class WaveformEditorWindow : public juce::DocumentWindow
    {
    public:
        explicit WaveformEditorWindow(B33pProcessor& processor);

        void closeButtonPressed() override;

        WaveformEditor& getEditor() { return *editor; }

    private:
        WaveformEditor* editor { nullptr };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformEditorWindow)
    };
}
