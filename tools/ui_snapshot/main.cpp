// b33p_ui_snapshot: render the real plugin editor headlessly to a PNG, or audit its
// hit-target sizes.
//
//   b33p_ui_snapshot <out.png> [scale] [width height]   (scale defaults to 2.0; width/height
//                                                         default to the editor's own preferred
//                                                         size — pass 1000 600 to check the
//                                                         minimum resize floor, setResizeLimits
//                                                         (1000, 600, 3200, 2200) in B33pEditor)
//
//   b33p_ui_snapshot --hit-audit [width height]          walks the constructed editor and
//                                                         prints every visible Button /
//                                                         ComboBox / Slider (including custom
//                                                         clickables built on top of them, e.g.
//                                                         IconButton, zqsfx::ui::LogoMark) whose
//                                                         on-screen hit box is under the house
//                                                         22 px floor (../CLAUDE.md section 6 /
//                                                         ../../CLAUDE.md section 6). Exit code
//                                                         is 0 when nothing is under the floor,
//                                                         1 otherwise — wired into ctest below.
//
// The look-and-feel regression gate for the ZQ SFX house-UI migration (see
// docs/ZQSFX_UI_STYLE_GUIDE.md): render before a UI change,
// render after, compare. Follows the same "pamplejuce pattern" as LFlOw's lflow_ui_snapshot and
// Broken's ts_ui_snapshot: links against b33p's own shared-code CMake target (B33p, produced by
// juce_add_plugin) instead of recompiling the plugin sources a second time — see CMakeLists.txt
// for the target wiring.
//
// Determinism: nothing here ever pumps JUCE's message loop (no runDispatchLoop), so any
// juce::Timer owned by the editor or its children never actually fires before the snapshot is
// taken — JUCE dispatches timer callbacks through the message queue, not directly from a
// background timer thread. The snapshot is taken immediately after construction, which is what
// makes two successive renders of unchanged code byte-identical. The hit-audit walk runs under
// the same guarantee, so its output is deterministic run to run.

#include "State/B33pProcessor.h"
#include "UI/B33pEditor.h"

#include <functional>
#include <iostream>

namespace
{
    // House accessibility floor (../CLAUDE.md section 6, restated in b33p's own
    // Source/UI/B33pEditor.cpp / house-UI docs): every interactive control needs a hit area
    // of at least this many pixels in both dimensions, at any editor size down to the
    // declared minimum.
    constexpr int kMinHitPx = 22;

    // Recursively walks the component tree rooted at `component`, printing (and counting)
    // every clickable descendant whose hit box is under kMinHitPx in either dimension.
    // `ancestorsVisible` tracks the isVisible() chain manually rather than relying on
    // Component::isShowing() — isShowing() also requires a live peer/desktop attachment
    // (see juce_Component.cpp), which this headless tool deliberately never creates (no
    // runDispatchLoop, no addToDesktop — see the determinism note above). A component with
    // an invisible ancestor is skipped exactly as isShowing() would skip it; only the
    // peer/minimised check is omitted since it can never be true here.
    //
    // "Clickable" follows the spec's own list: Button, ComboBox, Slider. Every custom
    // clickable in this codebase (IconButton, B33pSlider, zqsfx::ui::LogoMark) is a
    // subclass of one of those three, so the dynamic_cast chain covers them without
    // needing a fourth, project-specific base class.
    // Total clickable controls the walk considered, regardless of pass/fail — printed
    // alongside the violation count so a 0-violation run is legible as "audited N
    // controls, all clear" rather than indistinguishable from a walk that silently
    // found nothing (e.g. because the editor never became visible — see setVisible
    // below).
    int auditedControlCount = 0;

    int auditRecursive (juce::Component& component, bool ancestorsVisible, int depth)
    {
        const bool showing = ancestorsVisible && component.isVisible();
        int violations = 0;

        if (showing)
        {
            const bool isClickable = dynamic_cast<juce::Button*>   (&component) != nullptr
                                   || dynamic_cast<juce::ComboBox*> (&component) != nullptr
                                   || dynamic_cast<juce::Slider*>   (&component) != nullptr;

            if (isClickable)
            {
                ++auditedControlCount;
                const int w = component.getWidth();
                const int h = component.getHeight();

                if (w < kMinHitPx || h < kMinHitPx)
                {
                    juce::String label = component.getTitle();
                    if (label.isEmpty()) label = component.getName();
                    if (label.isEmpty()) label = component.getComponentID();
                    if (label.isEmpty()) label = "(unnamed " + juce::String (typeid (component).name()) + ")";

                    std::cout << "  UNDER-FLOOR  " << label.toStdString()
                              << "  " << w << "x" << h
                              << "  (need >= " << kMinHitPx << " in both dims)\n";
                    ++violations;
                }
            }
        }

        for (int i = 0; i < component.getNumChildComponents(); ++i)
            if (auto* child = component.getChildComponent (i))
                violations += auditRecursive (*child, showing, depth + 1);

        return violations;
    }

    int runHitAudit (int argc, char** argv)
    {
        int w = 0, h = 0;
        if (argc > 3)
        {
            w = juce::String (argv[2]).getIntValue();
            h = juce::String (argv[3]).getIntValue();
        }

        juce::ScopedJuceInitialiser_GUI gui;

        // Same declaration-order / teardown-order contract as the snapshot path below:
        // processor outlives the editor.
        B33p::B33pProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        if (editor == nullptr)
        {
            std::cerr << "createEditor returned null\n";
            return 1;
        }

        if (w > 0 && h > 0)
            editor->setSize (w, h); // clamped to setResizeLimits(1000, 600, 3200, 2200) in B33pEditor

        // Components default to non-visible until shown (Component::setVisible doc comment) —
        // a real host or the standalone window makes the editor visible before display, which
        // is the only reason isVisible()/isShowing() ever return true for its descendants.
        // This headless tool never creates a peer, so it must set that top-level flag itself;
        // everything below the editor keeps whatever visibility Section/LabeledSlider/etc. set
        // during layout (e.g. a hidden customEditButton or filter-mode-specific slider stays
        // correctly excluded from the walk).
        editor->setVisible (true);

        std::cout << "hit-audit @ " << editor->getWidth() << "x" << editor->getHeight() << "\n";
        const int violations = auditRecursive (*editor, true, 0);
        std::cout << violations << " control(s) under " << kMinHitPx << "px at "
                  << editor->getWidth() << "x" << editor->getHeight()
                  << "  (" << auditedControlCount << " clickable control(s) audited)\n";

        editor.reset();
        return violations > 0 ? 1 : 0;
    }

    int runSnapshot (int argc, char** argv)
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]));
        const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 2.0f;

        // processor declared before editor: C++ destroys locals in reverse declaration order, so
        // the editor is always torn down before the processor it references (spec requirement:
        // "delete the editor before the processor").
        B33p::B33pProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        if (editor == nullptr)
        {
            std::cerr << "createEditor returned null\n";
            return 1;
        }

        if (argc > 4)
        {
            const int w = juce::String (argv[3]).getIntValue();
            const int h = juce::String (argv[4]).getIntValue();
            editor->setSize (w, h); // clamped to setResizeLimits(1000, 600, 3200, 2200) in B33pEditor
        }

        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);

        out.getParentDirectory().createDirectory();
        out.deleteFile();
        juce::FileOutputStream stream (out);
        juce::PNGImageFormat png;
        if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
        {
            std::cerr << "could not write " << out.getFullPathName() << "\n";
            return 1;
        }

        std::cout << out.getFullPathName() << "  " << image.getWidth() << "x" << image.getHeight() << "\n";

        // editor is destroyed here (end of scope), before processor — matches the spec's ordering
        // requirement and avoids the editor's destructor touching a half-torn-down processor.
        editor.reset();
        return 0;
    }
}

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: b33p_ui_snapshot <out.png> [scale] [width height]\n";
        std::cerr << "       b33p_ui_snapshot --hit-audit [width height]\n";
        return 2;
    }

    if (juce::String (argv[1]) == "--hit-audit")
        return runHitAudit (argc, argv);

    return runSnapshot (argc, argv);
}
