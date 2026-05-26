#include <catch2/catch_test_macros.hpp>

#include "Core/CommandLineFiles.h"

#include <juce_core/juce_core.h>

using B33p::findBeepFileInCommandLine;

namespace
{
    // Unique scratch directory under the system temp dir, cleaned up on
    // destruction so each test leaves nothing behind.
    struct ScratchDir
    {
        ScratchDir()
        {
            dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                      .getChildFile("b33p_cmdline_test_"
                                    + juce::String(juce::Random::getSystemRandom().nextInt()));
            dir.createDirectory();
        }

        ~ScratchDir() { dir.deleteRecursively(); }

        juce::File makeFile(const juce::String& name) const
        {
            auto f = dir.getChildFile(name);
            f.replaceWithText("dummy");   // existsAsFile() must be true
            return f;
        }

        juce::File dir;
    };
}

TEST_CASE("CommandLine: empty input yields an invalid file")
{
    REQUIRE_FALSE(findBeepFileInCommandLine("").existsAsFile());
    REQUIRE_FALSE(findBeepFileInCommandLine("   ").existsAsFile());
}

TEST_CASE("CommandLine: a bare absolute .beep path is found")
{
    ScratchDir scratch;
    const auto beep = scratch.makeFile("song.beep");

    REQUIRE(findBeepFileInCommandLine(beep.getFullPathName()) == beep);
}

TEST_CASE("CommandLine: a quoted path with spaces stays intact")
{
    ScratchDir scratch;
    const auto beep = scratch.makeFile("my droid chatter.beep");

    const auto quoted = "\"" + beep.getFullPathName() + "\"";
    REQUIRE(findBeepFileInCommandLine(quoted) == beep);
}

TEST_CASE("CommandLine: the .beep is picked out of a multi-token line")
{
    ScratchDir scratch;
    const auto beep = scratch.makeFile("alarm.beep");

    // Mimics a launch line: leading executable path, a flag, then the file.
    const auto line = "/Applications/b33p.app/Contents/MacOS/b33p --foo "
                    + beep.getFullPathName();
    REQUIRE(findBeepFileInCommandLine(line) == beep);
}

TEST_CASE("CommandLine: a non-.beep existing file is ignored")
{
    ScratchDir scratch;
    const auto wav = scratch.makeFile("render.wav");

    REQUIRE_FALSE(findBeepFileInCommandLine(wav.getFullPathName()).existsAsFile());
}

TEST_CASE("CommandLine: a .beep path that does not exist is rejected")
{
    ScratchDir scratch;
    const auto missing = scratch.dir.getChildFile("ghost.beep");   // never created
    REQUIRE_FALSE(missing.existsAsFile());

    REQUIRE_FALSE(findBeepFileInCommandLine(missing.getFullPathName()).existsAsFile());
}

TEST_CASE("CommandLine: a relative path resolves against the working directory")
{
    const auto name = "b33p_cmdline_rel_"
                    + juce::String(juce::Random::getSystemRandom().nextInt()) + ".beep";
    auto relFile = juce::File::getCurrentWorkingDirectory().getChildFile(name);
    relFile.replaceWithText("dummy");

    REQUIRE(findBeepFileInCommandLine(name) == relFile);

    relFile.deleteFile();
}
