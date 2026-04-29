#pragma once

#include "Pattern/WavWriter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace B33p
{
    // Modal export-format picker for the WAV render. Self-contained
    // Component — owns its form controls, asks the user for a
    // destination path + sample rate + bit depth + channel mode, and
    // returns the gathered choices through an onClose callback.
    //
    // Wiring to the actual render path is the next commit's job.
    // For this commit ExportDialog is just the picker.
    //
    // Use showAsync() to launch as a modal DialogWindow — the static
    // helper handles the DialogWindow lifecycle so callers don't have
    // to know about the JUCE modal-window dance.
    class ExportDialog : public juce::Component
    {
    public:
        struct Result
        {
            bool         accepted    { false };
            juce::File   destination;
            double       sampleRate  { 48000.0 };
            BitDepth     bitDepth    { BitDepth::Sixteen };
            ChannelMode  channelMode { ChannelMode::Mono };
            // 1 = single render (legacy behaviour). 2+ = render
            // that many dice-rolled variations into numbered
            // siblings (Filename_001.wav, Filename_002.wav, ...);
            // unlocked parameters get re-rolled between each
            // variation, then the original parameter values are
            // restored when the batch finishes.
            int          variationCount { 1 };
        };

        using OnClose = std::function<void(Result)>;

        // Launches the dialog modally over the given parent. The
        // callback fires exactly once — either with accepted=true
        // when the user clicks Export with a valid destination, or
        // with accepted=false on Cancel / Esc.
        static void showAsync(juce::Component* parent, OnClose onClose);

        // Public so DialogWindow can construct it; prefer showAsync.
        explicit ExportDialog(OnClose onClose);
        ~ExportDialog() override = default;

        void resized() override;

    private:
        void browseClicked();
        void exportClicked();
        void cancelClicked();
        void closeDialog(bool accepted);

        Result gatherResult(bool accepted) const;

        OnClose onCloseCallback;

        juce::Label      destinationLabel;
        juce::TextEditor destinationField;
        juce::TextButton browseButton { "Browse..." };

        juce::Label      sampleRateLabel;
        juce::ComboBox   sampleRateCombo;

        juce::Label      bitDepthLabel;
        juce::ComboBox   bitDepthCombo;

        juce::Label      channelLabel;
        juce::ComboBox   channelCombo;

        juce::Label      variationsLabel;
        juce::Slider     variationsSlider;

        juce::TextButton cancelButton { "Cancel" };
        juce::TextButton exportButton { "Export" };

        std::unique_ptr<juce::FileChooser> fileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
    };
}
