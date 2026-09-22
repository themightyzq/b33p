// b33p_ui_snapshot: render the real plugin editor headlessly to a PNG.
//
//   b33p_ui_snapshot <out.png> [scale] [width height]   (scale defaults to 2.0; width/height
//                                                         default to the editor's own preferred
//                                                         size — pass 1000 600 to check the
//                                                         minimum resize floor, setResizeLimits
//                                                         (1000, 600, 3200, 2200) in B33pEditor)
//
// The look-and-feel regression gate for the ZQ SFX house-UI migration (see
// docs/ZQSFX_UI_STYLE_GUIDE.md and docs/ui_migration_report.md): render before a UI change,
// render after, compare. Follows the same "pamplejuce pattern" as LFlOw's lflow_ui_snapshot and
// Broken's ts_ui_snapshot: links against b33p's own shared-code CMake target (B33p, produced by
// juce_add_plugin) instead of recompiling the plugin sources a second time — see CMakeLists.txt
// for the target wiring.
//
// Determinism: nothing here ever pumps JUCE's message loop (no runDispatchLoop), so any
// juce::Timer owned by the editor or its children never actually fires before the snapshot is
// taken — JUCE dispatches timer callbacks through the message queue, not directly from a
// background timer thread. The snapshot is taken immediately after construction, which is what
// makes two successive renders of unchanged code byte-identical (verified in
// docs/ui_migration_report.md).

#include "State/B33pProcessor.h"
#include "UI/B33pEditor.h"

#include <iostream>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: b33p_ui_snapshot <out.png> [scale] [width height]\n";
        return 2;
    }

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
