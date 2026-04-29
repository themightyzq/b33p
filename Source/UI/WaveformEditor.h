#pragma once

#include "DSP/Oscillator.h"
#include "State/B33pProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <vector>

namespace B33p
{
    // Drawable single-cycle waveform editor. Shows the table that
    // the lane's currently-selected Wavetable slot holds, lets the
    // user click + drag to set per-sample values, and publishes
    // updates to the processor (which forwards to the audio thread
    // via the existing per-slot snapshot pattern).
    //
    // A row of slot tabs across the top selects which of the four
    // wavetable slots is being edited. Slot 0 is the Custom-mode
    // table — switching slots is a no-op when the lane's waveform
    // is Custom, since that mode only plays slot 0.
    //
    // setLane(int) re-points the editor at a different lane's
    // wavetable; an empty slot is seeded locally with one cycle of
    // sine so the user always has something to draw on rather than
    // starting from zero.
    class WaveformEditor : public juce::Component
    {
    public:
        explicit WaveformEditor(B33pProcessor& processor);

        void setLane(int lane);
        void setSlot(int slot);
        int  getSlot() const noexcept { return currentSlot; }

        void paint(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

    private:
        void   reloadFromProcessor();
        void   editAt(int sampleIdx, float value);
        void   strokeBetween(int fromIdx, float fromValue,
                              int toIdx,   float toValue);
        juce::Rectangle<float> plotRect() const;
        int    xToSampleIdx(float x) const;
        float  yToValue(float y) const;
        void   publish();

        B33pProcessor&     processor;
        int                currentLane    { 0 };
        int                currentSlot    { 0 };
        std::vector<float> table;
        int                lastDragSample { -1 };
        float              lastDragValue  { 0.0f };

        // Slot tabs across the top — clicking one calls setSlot.
        // Made a TextButton with radio behaviour so JUCE manages
        // the "selected" toggle state for us.
        std::array<juce::TextButton, Oscillator::kNumWavetableSlots> slotButtons;

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
