#include "ExportDialog.h"

#include <array>

namespace B33p
{
    namespace
    {
        struct SampleRatePreset { const char* label; double rate; };
        constexpr std::array<SampleRatePreset, 8> kSampleRates {{
            { "8 kHz",      8000.0  },
            { "11.025 kHz", 11025.0 },
            { "16 kHz",     16000.0 },
            { "22.05 kHz",  22050.0 },
            { "44.1 kHz",   44100.0 },
            { "48 kHz",     48000.0 },
            { "88.2 kHz",   88200.0 },
            { "96 kHz",     96000.0 },
        }};

        struct FormatPreset { const char* label; AudioFileFormat value; };
        constexpr std::array<FormatPreset, 4> kFormats {{
            { "WAV",        AudioFileFormat::Wav       },
            { "AIFF",       AudioFileFormat::Aiff      },
            { "FLAC",       AudioFileFormat::Flac      },
            { "OGG Vorbis", AudioFileFormat::OggVorbis },
        }};

        struct BitDepthPreset { const char* label; BitDepth value; };
        constexpr std::array<BitDepthPreset, 3> kBitDepths {{
            { "8-bit",  BitDepth::Eight      },
            { "16-bit", BitDepth::Sixteen    },
            { "24-bit", BitDepth::TwentyFour },
        }};

        struct ChannelPreset { const char* label; ChannelMode value; };
        constexpr std::array<ChannelPreset, 2> kChannels {{
            { "Mono",   ChannelMode::Mono             },
            { "Stereo", ChannelMode::StereoDuplicated },
        }};

        // ComboBox uses 1-based item IDs (0 == "no selection"), so
        // map 1..N onto 0-indexed presets at every boundary.
        int idForIndex(int i)   { return i + 1; }
        int indexForId(int id)  { return id - 1; }

        void styleLabel(juce::Label& label, const juce::String& text)
        {
            label.setText(text, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredRight);
            label.setFont(juce::FontOptions(12.0f));
        }
    }

    void ExportDialog::showAsync(juce::Component* parent, OnClose onClose)
    {
        auto* dialog = new ExportDialog(std::move(onClose));
        dialog->setSize(420, 296);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(dialog);
        options.dialogTitle                  = "Export WAV";
        options.dialogBackgroundColour       = juce::Colour::fromRGB(36, 36, 36);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar            = true;
        options.resizable                    = false;
        options.componentToCentreAround      = parent;
        options.launchAsync();
    }

    ExportDialog::ExportDialog(OnClose onClose)
        : onCloseCallback(std::move(onClose))
    {
        styleLabel(destinationLabel, "File:");
        addAndMakeVisible(destinationLabel);

        destinationField.setMultiLine(false);
        destinationField.setReadOnly(false);
        destinationField.setTextToShowWhenEmpty("Pick a destination...",
                                                juce::Colours::grey);
        addAndMakeVisible(destinationField);

        browseButton.onClick = [this] { browseClicked(); };
        addAndMakeVisible(browseButton);

        styleLabel(formatLabel, "Format:");
        addAndMakeVisible(formatLabel);
        for (size_t i = 0; i < kFormats.size(); ++i)
            formatCombo.addItem(kFormats[i].label,
                                idForIndex(static_cast<int>(i)));
        formatCombo.setSelectedId(idForIndex(0), juce::dontSendNotification); // WAV

        // Live-rewrite the displayed filename extension when the user
        // changes format. Without this, the dialog shows e.g.
        // "b33p_export.wav" even after picking FLAC — the actual
        // submit-time rewrite (below) eventually fixes it, but the
        // visible filename desyncs from the chosen format in the
        // meantime, which reads as a bug.
        formatCombo.onChange = [this]
        {
            const auto current = destinationField.getText().trim();
            if (current.isEmpty()) return;

            const int fmtIdx = indexForId(formatCombo.getSelectedId());
            if (fmtIdx < 0) return;

            const auto format = kFormats[static_cast<size_t>(fmtIdx)].value;
            const auto rewritten = juce::File(current).withFileExtension(extensionFor(format));
            destinationField.setText(rewritten.getFullPathName(),
                                      juce::dontSendNotification);
        };

        addAndMakeVisible(formatCombo);

        styleLabel(sampleRateLabel, "Sample Rate:");
        addAndMakeVisible(sampleRateLabel);
        for (size_t i = 0; i < kSampleRates.size(); ++i)
            sampleRateCombo.addItem(kSampleRates[i].label,
                                    idForIndex(static_cast<int>(i)));
        // Default to 44.1 kHz (fifth preset). Matches b33p's primary
        // distribution context (music + web + games), where 44.1 is
        // the de-facto standard. The 8 / 11.025 / 16 / 22.05 kHz
        // presets ahead of it are deliberate lo-fi / retro options.
        sampleRateCombo.setSelectedId(idForIndex(4),
                                      juce::dontSendNotification);
        addAndMakeVisible(sampleRateCombo);

        styleLabel(bitDepthLabel, "Bit Depth:");
        addAndMakeVisible(bitDepthLabel);
        for (size_t i = 0; i < kBitDepths.size(); ++i)
            bitDepthCombo.addItem(kBitDepths[i].label,
                                  idForIndex(static_cast<int>(i)));
        bitDepthCombo.setSelectedId(idForIndex(1), juce::dontSendNotification); // 16-bit
        addAndMakeVisible(bitDepthCombo);

        styleLabel(channelLabel, "Channels:");
        addAndMakeVisible(channelLabel);
        for (size_t i = 0; i < kChannels.size(); ++i)
            channelCombo.addItem(kChannels[i].label,
                                 idForIndex(static_cast<int>(i)));
        channelCombo.setSelectedId(idForIndex(0), juce::dontSendNotification); // Mono
        addAndMakeVisible(channelCombo);

        styleLabel(variationsLabel, "Variations:");
        addAndMakeVisible(variationsLabel);
        variationsSlider.setSliderStyle(juce::Slider::IncDecButtons);
        variationsSlider.setRange(1.0, 100.0, 1.0);
        variationsSlider.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_AutoDirection);
        variationsSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 22);
        variationsSlider.setValue(1.0, juce::dontSendNotification);
        variationsSlider.setTooltip("1 = single render. 2+ = render that many dice-rolled variations into numbered files (Filename_001.wav, etc.). The original parameter values are restored when the batch finishes.");
        addAndMakeVisible(variationsSlider);

        cancelButton.onClick = [this] { cancelClicked(); };
        addAndMakeVisible(cancelButton);

        exportButton.onClick = [this] { exportClicked(); };
        addAndMakeVisible(exportButton);
    }

    void ExportDialog::resized()
    {
        constexpr int kPadding   = 16;
        constexpr int kRowGap    = 10;
        constexpr int kRowHeight = 26;
        constexpr int kLabelWidth = 100;
        constexpr int kBrowseWidth = 90;
        constexpr int kButtonWidth = 90;
        constexpr int kButtonGap   = 8;

        auto bounds = getLocalBounds().reduced(kPadding);

        // Bottom row: Cancel / Export, right-aligned.
        auto buttonRow = bounds.removeFromBottom(kRowHeight);
        bounds.removeFromBottom(kRowGap);
        exportButton.setBounds(buttonRow.removeFromRight(kButtonWidth));
        buttonRow.removeFromRight(kButtonGap);
        cancelButton.setBounds(buttonRow.removeFromRight(kButtonWidth));

        auto layoutRow = [&](juce::Component& label, juce::Component& field,
                             juce::Component* trailing = nullptr)
        {
            auto row = bounds.removeFromTop(kRowHeight);
            bounds.removeFromTop(kRowGap);
            label.setBounds(row.removeFromLeft(kLabelWidth));
            row.removeFromLeft(kButtonGap);
            if (trailing != nullptr)
            {
                trailing->setBounds(row.removeFromRight(kBrowseWidth));
                row.removeFromRight(kButtonGap);
            }
            field.setBounds(row);
        };

        layoutRow(destinationLabel, destinationField, &browseButton);
        layoutRow(formatLabel,      formatCombo);
        layoutRow(sampleRateLabel,  sampleRateCombo);
        layoutRow(bitDepthLabel,    bitDepthCombo);
        layoutRow(channelLabel,     channelCombo);
        layoutRow(variationsLabel,  variationsSlider);
    }

    void ExportDialog::browseClicked()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save WAV...",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile("b33p_export.wav"),
            "*.wav");

        const int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting;

        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            const auto chosen = fc.getResult();
            if (chosen != juce::File())
                destinationField.setText(chosen.getFullPathName(),
                                         juce::dontSendNotification);
        });
    }

    void ExportDialog::exportClicked()
    {
        if (destinationField.getText().trim().isEmpty())
        {
            // Nudge the field so the user notices it's required;
            // an alert dialog felt heavy for an obvious miss.
            destinationField.setColour(
                juce::TextEditor::backgroundColourId,
                juce::Colour::fromRGB(60, 30, 30));
            destinationField.repaint();
            destinationField.grabKeyboardFocus();
            return;
        }

        // Pre-flight confirm for large batches so the user knows
        // they're about to start an N-file render with no in-progress
        // cancel path. Without this, dragging the variations slider
        // to 100 and clicking Export looks like the app froze. Below
        // the threshold, batches are fast enough that the confirm
        // would just be friction.
        constexpr int kBatchConfirmThreshold = 10;
        const int variations = juce::jlimit(1, 100,
            static_cast<int>(variationsSlider.getValue()));

        if (variations >= kBatchConfirmThreshold)
        {
            const juce::String msg =
                "Render " + juce::String(variations) + " files into the destination folder?\n\n"
                "Each file is one dice-rolled variation of the current patch, written as "
                "numbered files (_001, _002, ...). The UI is locked while the batch runs.";

            juce::AlertWindow::showAsync(
                juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::QuestionIcon)
                    .withTitle("Confirm batch export")
                    .withMessage(msg)
                    .withButton("Cancel")
                    .withButton("Render"),
                [this](int result)
                {
                    if (result == 2) closeDialog(true);
                });
            return;
        }

        closeDialog(true);
    }

    void ExportDialog::cancelClicked()
    {
        closeDialog(false);
    }

    ExportDialog::Result ExportDialog::gatherResult(bool accepted) const
    {
        Result r;
        r.accepted    = accepted;
        r.destination = juce::File(destinationField.getText().trim());

        const int srIdx = indexForId(sampleRateCombo.getSelectedId());
        if (srIdx >= 0 && srIdx < static_cast<int>(kSampleRates.size()))
            r.sampleRate = kSampleRates[static_cast<size_t>(srIdx)].rate;

        const int bdIdx = indexForId(bitDepthCombo.getSelectedId());
        if (bdIdx >= 0 && bdIdx < static_cast<int>(kBitDepths.size()))
            r.bitDepth = kBitDepths[static_cast<size_t>(bdIdx)].value;

        const int chIdx = indexForId(channelCombo.getSelectedId());
        if (chIdx >= 0 && chIdx < static_cast<int>(kChannels.size()))
            r.channelMode = kChannels[static_cast<size_t>(chIdx)].value;

        const int fmtIdx = indexForId(formatCombo.getSelectedId());
        if (fmtIdx >= 0 && fmtIdx < static_cast<int>(kFormats.size()))
            r.format = kFormats[static_cast<size_t>(fmtIdx)].value;

        // Force the destination's extension to match the chosen
        // format so the user can pick FLAC and forget the .wav
        // extension they typed earlier.
        if (r.destination != juce::File())
            r.destination = r.destination.withFileExtension(extensionFor(r.format));

        r.variationCount = juce::jlimit(1, 100,
            static_cast<int>(variationsSlider.getValue()));

        return r;
    }

    void ExportDialog::closeDialog(bool accepted)
    {
        const auto result = gatherResult(accepted);

        if (onCloseCallback)
            onCloseCallback(result);

        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(0);
    }
}
