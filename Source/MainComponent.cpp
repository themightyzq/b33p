#include "MainComponent.h"

namespace B33p
{
    namespace
    {
        constexpr int kOuterPadding   = 12;
        constexpr int kGap            = 8;
        constexpr int kTopRowHeight   = 260;
        constexpr int kMidRowHeight   = 180;
        constexpr int kPitchRowHeight = 180;
    }

    MainComponent::MainComponent(B33pProcessor& processorRef)
        : processor(processorRef),
          fileManager(processorRef),
          oscillatorSection   (processor),
          ampEnvelopeSection  (processor),
          filterSection       (processor),
          effectsSection      (processor),
          masterSection       (processor),
          pitchEnvelopeSection(processor),
          patternSection      (processor)
    {
        addAndMakeVisible(oscillatorSection);
        addAndMakeVisible(ampEnvelopeSection);
        addAndMakeVisible(filterSection);
        addAndMakeVisible(effectsSection);
        addAndMakeVisible(masterSection);
        addAndMakeVisible(pitchEnvelopeSection);
        addAndMakeVisible(patternSection);

        fileManager.setOnStateChanged([this] { updateWindowTitle(); });
        processor.setOnDirtyChanged ([this] { updateWindowTitle(); });

        setWantsKeyboardFocus(true);

        setSize(900, 920);
    }

    void MainComponent::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour::fromRGB(22, 22, 22));
    }

    void MainComponent::resized()
    {
        auto bounds = getLocalBounds().reduced(kOuterPadding);

        auto topRow = bounds.removeFromTop(kTopRowHeight);
        bounds.removeFromTop(kGap);
        auto midRow = bounds.removeFromTop(kMidRowHeight);
        bounds.removeFromTop(kGap);
        auto pitchRow = bounds.removeFromTop(kPitchRowHeight);
        bounds.removeFromTop(kGap);
        auto patternRow = bounds;

        const int topCellWidth = (topRow.getWidth() - 2 * kGap) / 3;
        oscillatorSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        ampEnvelopeSection.setBounds(topRow.removeFromLeft(topCellWidth));
        topRow.removeFromLeft(kGap);
        filterSection.setBounds(topRow);

        const int midCellWidth = (midRow.getWidth() - kGap) / 2;
        effectsSection.setBounds(midRow.removeFromLeft(midCellWidth));
        midRow.removeFromLeft(kGap);
        masterSection.setBounds(midRow);

        pitchEnvelopeSection.setBounds(pitchRow);
        patternSection.setBounds(patternRow);
    }

    void MainComponent::openProjectFile(const juce::File& file)
    {
        fileManager.openFile(file);
    }

    bool MainComponent::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::spaceKey)
        {
            processor.triggerAudition();
            return true;
        }

        const auto& mods = key.getModifiers();
        const bool cmd   = mods.isCommandDown();
        const bool shift = mods.isShiftDown();
        const int  code  = key.getKeyCode();

        if (cmd && shift && code == 'S')   { fileManager.saveAs(this); return true; }
        if (cmd && code == 'S')            { fileManager.save  (this); return true; }
        if (cmd && code == 'O')            { fileManager.open  (this); return true; }

        return false;
    }

    void MainComponent::parentHierarchyChanged()
    {
        // The window doesn't exist while the constructor runs;
        // this fires once after setContentOwned attaches us to the
        // DocumentWindow, which is the right moment to sync the
        // initial title.
        updateWindowTitle();
    }

    void MainComponent::updateWindowTitle()
    {
        auto* dw = findParentComponentOfClass<juce::DocumentWindow>();
        if (dw == nullptr)
            return;

        const auto& file   = fileManager.getCurrentFile();
        const auto  name   = file == juce::File() ? juce::String("Untitled")
                                                     : file.getFileName();
        const auto  suffix = processor.isDirty() ? juce::String(" *")
                                                  : juce::String();
        dw->setName("b33p - " + name + suffix);
    }
}
