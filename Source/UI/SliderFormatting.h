#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace B33p::SliderFormatting
{
    // Apply human-readable formatting to a juce::Slider's text box.
    // Each helper sets BOTH textFromValueFunction (display) and
    // valueFromTextFunction (parse user-typed input) so the box
    // round-trips. updateText() is called so the change is visible
    // immediately.
    //
    // Header-only because every helper is a thin wrapper over
    // capturing lambdas — the function-call cost would dwarf the
    // body and there's nothing here that needs a private impl.

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
}
