#include "WaveformEditor.h"

#include <algorithm>
#include <cmath>

namespace B33p
{
    namespace
    {
        constexpr float kInset    = 8.0f;
        constexpr float kStroke   = 2.0f;
        constexpr float kCorner   = 3.0f;

        // Two pi as a double — kept local rather than pulling
        // M_PI which isn't standard.
        constexpr double kTwoPi = 6.283185307179586;

        std::vector<float> makeSineTable(int size)
        {
            std::vector<float> t(static_cast<size_t>(size));
            for (size_t i = 0; i < t.size(); ++i)
            {
                const double phase = static_cast<double>(i)
                                   / static_cast<double>(t.size());
                t[i] = static_cast<float>(std::sin(kTwoPi * phase));
            }
            return t;
        }
    }

    WaveformEditor::WaveformEditor(B33pProcessor& processorRef)
        : processor(processorRef)
    {
        setLane(processor.getSelectedLane());
    }

    void WaveformEditor::setLane(int lane)
    {
        currentLane = lane;
        table = processor.getCustomWaveformCopy(lane);
        if (table.empty())
        {
            // Seed with one cycle of sine so the user always sees
            // SOMETHING — drawing into a flat zero is much harder
            // than modifying an existing curve.
            table = makeSineTable(Oscillator::kCustomTableSize);
            publish();
        }
        repaint();
    }

    void WaveformEditor::resized() {}

    void WaveformEditor::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        g.setColour(juce::Colour::fromRGB(20, 20, 20));
        g.fillRoundedRectangle(bounds, kCorner);
        g.setColour(juce::Colour::fromRGB(60, 60, 60));
        g.drawRoundedRectangle(bounds, kCorner, 1.0f);

        const auto plot = bounds.reduced(kInset);
        const float midY = plot.getCentreY();

        // Zero line.
        g.setColour(juce::Colour::fromRGB(55, 55, 55));
        g.drawHorizontalLine(static_cast<int>(midY),
                              plot.getX(), plot.getRight());

        if (table.empty())
            return;

        // Connect each sample with a line segment for a continuous
        // wave look. The table maps left-to-right across plot.
        juce::Path wave;
        const auto sampleCount = static_cast<int>(table.size());
        const float stepX = plot.getWidth() / static_cast<float>(sampleCount - 1);

        auto sampleY = [&](float v) {
            return midY - juce::jlimit(-1.0f, 1.0f, v) * (plot.getHeight() * 0.5f);
        };

        wave.startNewSubPath(plot.getX(), sampleY(table.front()));
        for (int i = 1; i < sampleCount; ++i)
            wave.lineTo(plot.getX() + i * stepX, sampleY(table[static_cast<size_t>(i)]));

        g.setColour(juce::Colour::fromRGB(220, 140, 60));
        g.strokePath(wave, juce::PathStrokeType(kStroke));
    }

    int WaveformEditor::xToSampleIdx(float x) const
    {
        const auto plot = getLocalBounds().toFloat().reduced(1.0f).reduced(kInset);
        if (table.empty() || plot.getWidth() <= 0.0f)
            return 0;
        const float t = (x - plot.getX()) / plot.getWidth();
        const int idx = static_cast<int>(std::round(t * (table.size() - 1)));
        return juce::jlimit(0, static_cast<int>(table.size()) - 1, idx);
    }

    float WaveformEditor::yToValue(float y) const
    {
        const auto plot = getLocalBounds().toFloat().reduced(1.0f).reduced(kInset);
        if (plot.getHeight() <= 0.0f)
            return 0.0f;
        const float midY = plot.getCentreY();
        const float v = (midY - y) / (plot.getHeight() * 0.5f);
        return juce::jlimit(-1.0f, 1.0f, v);
    }

    void WaveformEditor::editAt(int sampleIdx, float value)
    {
        if (sampleIdx < 0 || sampleIdx >= static_cast<int>(table.size()))
            return;
        table[static_cast<size_t>(sampleIdx)] = value;
    }

    void WaveformEditor::strokeBetween(int fromIdx, float fromValue,
                                        int toIdx,   float toValue)
    {
        // Linear interpolation between the two endpoints so a fast
        // drag doesn't leave gaps in the table.
        if (fromIdx == toIdx)
        {
            editAt(toIdx, toValue);
            return;
        }
        const int step = (toIdx > fromIdx) ? 1 : -1;
        const int span = std::abs(toIdx - fromIdx);
        for (int s = 0; s <= span; ++s)
        {
            const int   idx  = fromIdx + s * step;
            const float frac = static_cast<float>(s) / static_cast<float>(span);
            const float v    = fromValue + frac * (toValue - fromValue);
            editAt(idx, v);
        }
    }

    void WaveformEditor::publish()
    {
        processor.setCustomWaveform(currentLane, table);
    }

    void WaveformEditor::mouseDown(const juce::MouseEvent& e)
    {
        const int   idx = xToSampleIdx(e.position.x);
        const float val = yToValue(e.position.y);
        editAt(idx, val);
        lastDragSample = idx;
        lastDragValue  = val;
        publish();
        repaint();
    }

    void WaveformEditor::mouseDrag(const juce::MouseEvent& e)
    {
        const int   idx = xToSampleIdx(e.position.x);
        const float val = yToValue(e.position.y);
        if (lastDragSample < 0)
        {
            editAt(idx, val);
        }
        else
        {
            strokeBetween(lastDragSample, lastDragValue, idx, val);
        }
        lastDragSample = idx;
        lastDragValue  = val;
        publish();
        repaint();
    }

    // ---- Window ---------------------------------------------------

    WaveformEditorWindow::WaveformEditorWindow(B33pProcessor& processor)
        : DocumentWindow("Custom Waveform Editor",
                         juce::Colour::fromRGB(22, 22, 22),
                         DocumentWindow::closeButton)
    {
        auto* ed = new WaveformEditor(processor);
        editor = ed;
        setContentOwned(ed, true);
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        centreWithSize(640, 280);
    }

    void WaveformEditorWindow::closeButtonPressed()
    {
        // Hide rather than destroy — the owner re-shows the same
        // window when the user clicks Edit again, so unsaved drag
        // state lives between visits.
        setVisible(false);
    }
}
