#include "InspectorStrip.h"

#include "State/UndoableActions.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        constexpr int kLabelWidth      = 50;
        constexpr int kFieldWidth      = 78;
        constexpr int kLaneFieldWidth  = 56;
        constexpr int kFieldGap        = 4;
        constexpr int kGroupGap        = 12;
        constexpr int kDeleteWidth     = 70;
        constexpr int kRowHeight       = 24;
        constexpr int kTextBoxHeight   = kRowHeight;
        constexpr int kTextBoxWidth    = kFieldWidth;

        void styleField(juce::Slider& s)
        {
            s.setSliderStyle(juce::Slider::LinearBar);
            s.setTextBoxStyle(juce::Slider::TextBoxLeft, false,
                              kTextBoxWidth, kTextBoxHeight);
            // LinearBar paints a coloured bar over the whole control;
            // dim it so it reads as a number field with a subtle
            // value indicator rather than a loud horizontal bar.
            s.setColour(juce::Slider::trackColourId,
                         juce::Colour::fromRGB(60, 90, 120));
            s.setColour(juce::Slider::backgroundColourId,
                         juce::Colour::fromRGB(34, 34, 34));
        }

        void styleLabel(juce::Label& l, const juce::String& text)
        {
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centredRight);
            l.setFont(juce::FontOptions(11.0f));
            l.setColour(juce::Label::textColourId,
                         juce::Colour::fromRGB(170, 170, 170));
        }
    }

    InspectorStrip::InspectorStrip(B33pProcessor& processorRef)
        : processor(processorRef)
    {
        placeholder.setText("Click an event to edit it.",
                             juce::dontSendNotification);
        placeholder.setJustificationType(juce::Justification::centred);
        placeholder.setFont(juce::FontOptions(12.0f));
        placeholder.setColour(juce::Label::textColourId,
                               juce::Colour::fromRGB(110, 110, 110));
        addAndMakeVisible(placeholder);

        styleLabel(laneLabel,     "Lane");
        styleLabel(startLabel,    "Start");
        styleLabel(durationLabel, "Length");
        styleLabel(pitchLabel,    "Pitch");
        styleLabel(velocityLabel, "Vel");

        for (auto* l : { &laneLabel, &startLabel, &durationLabel,
                          &pitchLabel, &velocityLabel })
            addAndMakeVisible(*l);

        for (int i = 0; i < Pattern::kNumLanes; ++i)
            laneCombo.addItem(juce::String(i + 1), i + 1);
        laneCombo.onChange = [this]
        {
            const int newLane = laneCombo.getSelectedId() - 1;
            if (newLane < 0 || newLane == currentSelection.lane)
                return;

            // Lane change is a move: snapshot, remove from old lane,
            // append to new, then push as a single undo entry.
            if (! currentSelection.valid())
                return;

            Pattern before = processor.getPattern();
            const Event ev = currentEvent();
            processor.getPattern().removeEvent(currentSelection.lane,
                                                currentSelection.index);
            processor.getPattern().addEvent(newLane, ev);
            processor.markDirty();

            const std::size_t newIndex
                = processor.getPattern().getEvents(newLane).size() - 1;

            processor.getUndoManager().beginNewTransaction("Move event lane");
            processor.getUndoManager().perform(
                new SetPatternAction(processor, getParentComponent(),
                                     std::move(before),
                                     processor.getPattern()));

            currentSelection = { newLane, newIndex };
            refresh();
            if (auto* parent = getParentComponent())
                parent->repaint();
        };
        addAndMakeVisible(laneCombo);

        // Ranges chosen to match the editing limits enforced by the
        // grid: pattern length is 0.1..10s, min event duration is
        // 0.02s, pitch offset is the same +/-12 st as the curve view.
        startSlider   .setRange(0.0,  Pattern::kMaxLengthSeconds, 0.001);
        durationSlider.setRange(0.02, Pattern::kMaxLengthSeconds, 0.001);
        pitchSlider   .setRange(-24.0,   24.0,  1.0);
        velocitySlider.setRange(0.0,    1.0,   0.01);

        startSlider   .textFromValueFunction = [](double v) { return juce::String(v, 3) + " s"; };
        durationSlider.textFromValueFunction = [](double v) { return juce::String(v, 3) + " s"; };
        pitchSlider   .textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v)) + " st"; };
        velocitySlider.textFromValueFunction = [](double v) { return juce::String(v, 2); };

        for (auto* s : { &startSlider, &durationSlider, &pitchSlider, &velocitySlider })
            styleField(*s);

        startSlider.onValueChange = [this]
        {
            pushEdit([v = startSlider.getValue()](Event& e) { e.startSeconds = v; },
                      "Edit event start");
        };
        durationSlider.onValueChange = [this]
        {
            pushEdit([v = durationSlider.getValue()](Event& e) { e.durationSeconds = v; },
                      "Edit event length");
        };
        pitchSlider.onValueChange = [this]
        {
            pushEdit([v = static_cast<float>(pitchSlider.getValue())](Event& e)
                      { e.pitchOffsetSemitones = v; },
                      "Edit event pitch");
        };
        velocitySlider.onValueChange = [this]
        {
            pushEdit([v = static_cast<float>(velocitySlider.getValue())](Event& e)
                      { e.velocity = v; },
                      "Edit event velocity");
        };

        for (auto* s : { &startSlider, &durationSlider, &pitchSlider, &velocitySlider })
            addAndMakeVisible(*s);

        deleteButton.onClick = [this]
        {
            if (onDeleteRequested) onDeleteRequested();
        };
        addAndMakeVisible(deleteButton);

        setSelection({});
    }

    void InspectorStrip::setSelection(const PatternGrid::Selection& selection)
    {
        currentSelection = selection;
        const bool show = currentSelection.valid();

        placeholder.setVisible(! show);
        for (auto* c : { (juce::Component*) &laneLabel,
                          (juce::Component*) &startLabel,
                          (juce::Component*) &durationLabel,
                          (juce::Component*) &pitchLabel,
                          (juce::Component*) &velocityLabel,
                          (juce::Component*) &laneCombo,
                          (juce::Component*) &startSlider,
                          (juce::Component*) &durationSlider,
                          (juce::Component*) &pitchSlider,
                          (juce::Component*) &velocitySlider,
                          (juce::Component*) &deleteButton })
            c->setVisible(show);

        if (show)
        {
            laneCombo.setSelectedId(currentSelection.lane + 1,
                                     juce::dontSendNotification);
            writeFieldsFromEvent(currentEvent());
            setControlsEnabled(true);
        }
    }

    void InspectorStrip::refresh()
    {
        if (! currentSelection.valid())
            return;
        writeFieldsFromEvent(currentEvent());
    }

    Event InspectorStrip::currentEvent() const
    {
        if (! currentSelection.valid())
            return {};

        const auto& events = processor.getPattern().getEvents(currentSelection.lane);
        if (currentSelection.index >= events.size())
            return {};

        return events[currentSelection.index];
    }

    void InspectorStrip::writeFieldsFromEvent(const Event& e)
    {
        // dontSendNotification: we're echoing the model into the
        // controls; firing onValueChange would push the same value
        // back through the undo system.
        startSlider   .setValue(e.startSeconds,                       juce::dontSendNotification);
        durationSlider.setValue(e.durationSeconds,                    juce::dontSendNotification);
        pitchSlider   .setValue(static_cast<double>(e.pitchOffsetSemitones), juce::dontSendNotification);
        velocitySlider.setValue(static_cast<double>(e.velocity),      juce::dontSendNotification);
    }

    void InspectorStrip::setControlsEnabled(bool enabled)
    {
        for (auto* c : { (juce::Component*) &laneCombo,
                          (juce::Component*) &startSlider,
                          (juce::Component*) &durationSlider,
                          (juce::Component*) &pitchSlider,
                          (juce::Component*) &velocitySlider,
                          (juce::Component*) &deleteButton })
            c->setEnabled(enabled);
    }

    void InspectorStrip::pushEdit(std::function<void(Event&)> mutator,
                                   const juce::String& transactionName)
    {
        if (! currentSelection.valid() || ! mutator)
            return;

        auto& pattern = processor.getPattern();
        if (currentSelection.index
                >= pattern.getEvents(currentSelection.lane).size())
            return;

        Event after = pattern.getEvents(currentSelection.lane)[currentSelection.index];
        Event before = after;
        mutator(after);

        if (after == before)
            return;

        Pattern beforePattern = pattern;
        pattern.updateEvent(currentSelection.lane,
                            currentSelection.index,
                            after);
        processor.markDirty();

        processor.getUndoManager().beginNewTransaction(transactionName);
        processor.getUndoManager().perform(
            new SetPatternAction(processor, getParentComponent(),
                                 std::move(beforePattern),
                                 pattern));

        if (auto* parent = getParentComponent())
            parent->repaint();
    }

    void InspectorStrip::paint(juce::Graphics& g)
    {
        // Subtle background so the strip is visually distinct from
        // both the pattern grid above and the section frame.
        g.setColour(juce::Colour::fromRGB(28, 28, 28));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
    }

    void InspectorStrip::resized()
    {
        auto bounds = getLocalBounds().reduced(6, 4);
        placeholder.setBounds(bounds);

        if (! currentSelection.valid())
            return;

        // Layout left -> right; Delete is right-aligned.
        deleteButton.setBounds(bounds.removeFromRight(kDeleteWidth));
        bounds.removeFromRight(kGroupGap);

        auto layoutField = [&bounds](juce::Label& label, juce::Component& field, int fieldW)
        {
            label.setBounds(bounds.removeFromLeft(kLabelWidth));
            bounds.removeFromLeft(kFieldGap);
            field.setBounds(bounds.removeFromLeft(fieldW));
            bounds.removeFromLeft(kGroupGap);
        };

        layoutField(laneLabel,     laneCombo,      kLaneFieldWidth);
        layoutField(startLabel,    startSlider,    kFieldWidth);
        layoutField(durationLabel, durationSlider, kFieldWidth);
        layoutField(pitchLabel,    pitchSlider,    kFieldWidth);
        layoutField(velocityLabel, velocitySlider, kFieldWidth);
    }
}
