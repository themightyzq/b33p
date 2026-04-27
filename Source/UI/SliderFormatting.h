#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p::SliderFormatting
{
    // Slider configuration helpers — value-text formatting plus a
    // double-click-to-reset helper. Each formatting helper sets BOTH
    // textFromValueFunction (display) and valueFromTextFunction
    // (parse user-typed input) so the box round-trips. updateText()
    // is called so the change is visible immediately.
    //
    // Header-only because every helper is a thin wrapper — the
    // function-call cost would dwarf the body and there's nothing
    // here that needs a private impl.

    inline void applyHz(juce::Slider& slider)
    {
        slider.textFromValueFunction = [](double v)
        {
            if (v >= 1000.0)
                return juce::String(v / 1000.0, 1) + " kHz";
            return juce::String(juce::roundToInt(v)) + " Hz";
        };
        slider.valueFromTextFunction = [](const juce::String& s)
        {
            const auto trimmed = s.trim();
            const double n = trimmed.getDoubleValue();
            return trimmed.containsIgnoreCase("k") ? n * 1000.0 : n;
        };
        slider.updateText();
    }

    inline void applySeconds(juce::Slider& slider)
    {
        slider.textFromValueFunction = [](double v)
        {
            if (v < 1.0)
                return juce::String(juce::roundToInt(v * 1000.0)) + " ms";
            return juce::String(v, 2) + " s";
        };
        slider.valueFromTextFunction = [](const juce::String& s)
        {
            const auto trimmed = s.trim();
            const double n = trimmed.getDoubleValue();
            return trimmed.containsIgnoreCase("ms") ? n / 1000.0 : n;
        };
        slider.updateText();
    }

    inline void applyPercent(juce::Slider& slider)
    {
        slider.textFromValueFunction = [](double v)
        {
            return juce::String(juce::roundToInt(v * 100.0)) + " %";
        };
        slider.valueFromTextFunction = [](const juce::String& s)
        {
            return s.trim().getDoubleValue() / 100.0;
        };
        slider.updateText();
    }

    inline void applyInteger(juce::Slider& slider, const juce::String& suffix)
    {
        slider.textFromValueFunction = [suffix](double v)
        {
            return juce::String(juce::roundToInt(v)) + suffix;
        };
        slider.valueFromTextFunction = [](const juce::String& s)
        {
            return s.trim().getDoubleValue();
        };
        slider.updateText();
    }

    inline void applyDecimal(juce::Slider& slider, int decimalPlaces)
    {
        slider.textFromValueFunction = [decimalPlaces](double v)
        {
            return juce::String(v, decimalPlaces);
        };
        slider.valueFromTextFunction = [](const juce::String& s)
        {
            return s.trim().getDoubleValue();
        };
        slider.updateText();
    }

    // Wires double-click on the knob to snap back to the parameter's
    // default value (DAW convention). Pulls the default from the
    // APVTS parameter's normalised range so this stays in sync if
    // the parameter layout changes.
    inline void applyDoubleClickReset(juce::Slider& slider,
                                       juce::AudioProcessorValueTreeState& apvts,
                                       const juce::String& parameterID)
    {
        if (auto* param = apvts.getParameter(parameterID))
        {
            const auto& range = apvts.getParameterRange(parameterID);
            slider.setDoubleClickReturnValue(true,
                range.convertFrom0to1(param->getDefaultValue()));
        }
    }
}
