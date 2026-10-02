#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Pattern/Pattern.h"
#include "State/B33pProcessor.h"
#include "UI/B33pEditor.h"
#include "UI/EventOverridesDialog.h"
#include "UI/ExportDialog.h"

#include <zqsfx_ui/zqsfx_ui.h>

#include <algorithm>
#include <memory>
#include <vector>

// House control behaviour for every knob / slider in b33p's UI: a zqsfx::ui::Dial (keyboard
// focus, focus ring from the LookAndFeel, Shift+arrow fine step) whose double-click returns it
// to the parameter default. Like b33p_ui_snapshot --hit-audit this builds the real editor
// without ever creating a window peer, so it needs no display and runs on every CI OS.

namespace
{
    void collectSliders(juce::Component& root, std::vector<juce::Slider*>& out)
    {
        if (auto* s = dynamic_cast<juce::Slider*>(&root))
            out.push_back(s);
        for (auto* child : root.getChildren())
            if (child != nullptr)
                collectSliders(*child, out);
    }

    std::vector<juce::Slider*> slidersUnder(juce::Component& root)
    {
        std::vector<juce::Slider*> sliders;
        collectSliders(root, sliders);
        return sliders;
    }

    // Controls that are legitimately disabled right now (mod-effect knobs while the effect is
    // "None") report no keyboard focus and ignore double-click, whatever their class. Judge
    // them as they will behave once enabled, then put the enabled state back.
    struct ScopedEnabled
    {
        explicit ScopedEnabled(juce::Component& c) : component(c), wasEnabled(c.isEnabled())
        {
            component.setEnabled(true);
        }
        ~ScopedEnabled() { component.setEnabled(wasEnabled); }
        ScopedEnabled(const ScopedEnabled&) = delete;
        ScopedEnabled& operator=(const ScopedEnabled&) = delete;

        juce::Component& component;
        bool wasEnabled;
    };

    juce::String describe(const juce::Slider& s)
    {
        return s.getTitle().isNotEmpty() ? s.getTitle() : juce::String("(untitled slider)");
    }

    std::vector<float> snapshotParameters(juce::AudioProcessor& p)
    {
        std::vector<float> values;
        for (auto* param : p.getParameters())
            values.push_back(param->getValue());
        return values;
    }

    // The parameter a slider is attached to, found the way the user sees it: move the slider
    // and see which parameter follows. nullptr when the slider is not bound to a parameter.
    // The slider and the parameter are left at the moved value; callers reset them.
    juce::RangedAudioParameter* findBoundParameter(juce::AudioProcessor& p, juce::Slider& s)
    {
        const auto before = snapshotParameters(p);
        const double target = s.getValue() < s.getMaximum() ? s.getMaximum() : s.getMinimum();
        s.setValue(target, juce::sendNotificationSync);

        juce::RangedAudioParameter* found = nullptr;
        int changed = 0;
        for (int i = 0; i < p.getParameters().size(); ++i)
        {
            auto* param = p.getParameters()[i];
            if (std::abs(param->getValue() - before[static_cast<size_t>(i)]) > 1.0e-6f)
            {
                ++changed;
                found = dynamic_cast<juce::RangedAudioParameter*>(param);
            }
        }
        REQUIRE(changed <= 1);
        return found;
    }

    juce::MouseEvent doubleClickOn(juce::Slider& s)
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),
                                s.getLocalBounds().getCentre().toFloat(), juce::ModifierKeys(),
                                juce::MouseInputSource::defaultPressure,
                                juce::MouseInputSource::defaultOrientation,
                                juce::MouseInputSource::defaultRotation,
                                juce::MouseInputSource::defaultTiltX,
                                juce::MouseInputSource::defaultTiltY,
                                &s, &s, now, s.getLocalBounds().getCentre().toFloat(), now, 2, false);
    }

    // The checks every slider must pass, bound to a parameter or not.
    void requireHouseBehaviour(juce::Slider& s)
    {
        const ScopedEnabled enabled(s);
        INFO("slider: " << describe(s).toStdString());

        CHECK(dynamic_cast<zqsfx::ui::Dial*>(&s) != nullptr);
        CHECK(s.getWantsKeyboardFocus());
        CHECK(s.hasFocusOutline());

        // The focus ring comes from the LookAndFeel the slider actually resolves to: with
        // the editor alive that is b33p's, a zqsfx::ui::LookAndFeel subclass.
        auto& laf = s.getLookAndFeel();
        CHECK(dynamic_cast<zqsfx::ui::LookAndFeel*>(&laf) != nullptr);
        CHECK(laf.createFocusOutlineForComponent(s) != nullptr);

        // juce::Slider never double-click-resets the +/- button style, and the inspector's
        // "Event ..." fields edit one event's data (no meaningful default), so only the
        // others owe a reset value.
        if (s.getSliderStyle() != juce::Slider::IncDecButtons && ! s.getTitle().startsWith("Event "))
            CHECK(s.isDoubleClickReturnEnabled());
    }

    // A slider bound to a parameter must reset to that parameter's default, end to end.
    void requireDoubleClickResetsToDefault(juce::Slider& s, juce::RangedAudioParameter& param)
    {
        INFO("slider: " << describe(s).toStdString() << "  parameter: "
             << param.getParameterID().toStdString());

        const double defaultValue = param.convertFrom0to1(param.getDefaultValue());
        REQUIRE(s.isDoubleClickReturnEnabled());
        CHECK(s.getDoubleClickReturnValue() == Catch::Approx(defaultValue).margin(1.0e-4));

        // Move away from the default, then double-click.
        const double away = defaultValue < s.getMaximum() ? s.getMaximum() : s.getMinimum();
        s.setValue(away, juce::sendNotificationSync);
        REQUIRE(param.getValue() != Catch::Approx(param.getDefaultValue()).margin(1.0e-4));

        s.mouseDoubleClick(doubleClickOn(s));
        CHECK(param.getValue() == Catch::Approx(param.getDefaultValue()).margin(1.0e-4));
    }
}

TEST_CASE("Every slider in the editor is a house Dial", "[ui][knobs]")
{
    juce::ScopedJuceInitialiser_GUI gui;

    // The processor outlives the editor, as everywhere else the editor is built.
    B33p::B33pProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);

    const auto sliders = slidersUnder(*editor);
    REQUIRE(! sliders.empty());

    int boundToParameter = 0;
    for (auto* s : sliders)
    {
        const ScopedEnabled enabled(*s);
        requireHouseBehaviour(*s);

        if (auto* param = findBoundParameter(processor, *s))
        {
            ++boundToParameter;
            requireDoubleClickResetsToDefault(*s, *param);
        }
    }

    // Guard against a walk that silently found nothing: the selected lane's knobs are
    // oscillator 6, amp env 4, filter 3, effects 3, mod FX 3, master 1 and LFO rates 2, plus
    // four modulation amounts and the randomisation scope.
    CHECK(boundToParameter >= 27);

    editor.reset();
}

TEST_CASE("Event overrides dialog and export dialog sliders are house Dials", "[ui][knobs]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    B33p::B33pLookAndFeel lookAndFeel;
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

    {
        B33p::Event event;
        B33p::EventOverridesDialog overrides(event, [](const B33p::EventDialogEdits&) {}, [] {});
        const auto sliders = slidersUnder(overrides);
        CHECK(sliders.size() >= 7); // four override amounts, probability, ratchets, humanize
        for (auto* s : sliders)
            requireHouseBehaviour(*s);
    }

    {
        B33p::ExportDialog exportDialog([](B33p::ExportDialog::Result) {});
        const auto sliders = slidersUnder(exportDialog);
        CHECK(! sliders.empty());
        for (auto* s : sliders)
            requireHouseBehaviour(*s);
    }

    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

TEST_CASE("Arrow keys step a knob and Shift+arrow steps a tenth as far", "[ui][knobs]")
{
    juce::ScopedJuceInitialiser_GUI gui;

    B33p::B33pProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);

    zqsfx::ui::Dial* dial = nullptr;
    for (auto* s : slidersUnder(*editor))
        if (s->getSliderStyle() != juce::Slider::IncDecButtons)
            if (auto* d = dynamic_cast<zqsfx::ui::Dial*>(s); d != nullptr && d->getTitle() == "Attack")
                dial = d;
    REQUIRE(dial != nullptr);

    // Mid-range so neither direction clamps.
    dial->setValue(dial->getMinimum() + 0.5 * (dial->getMaximum() - dial->getMinimum()),
                   juce::sendNotificationSync);

    const auto right      = juce::KeyPress(juce::KeyPress::rightKey);
    const auto shiftRight = juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier, 0);

    const double start = dial->getValue();
    REQUIRE(dial->keyPressed(right));
    const double coarseStep = dial->getValue() - start;
    CHECK(coarseStep > 0.0);

    const double mid = dial->getValue();
    REQUIRE(dial->keyPressed(shiftRight));
    const double fineStep = dial->getValue() - mid;
    CHECK(fineStep > 0.0);
    CHECK(fineStep == Catch::Approx(coarseStep * 0.1).epsilon(0.05));

    editor.reset();
}
