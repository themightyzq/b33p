#include "Core/CommandLineFiles.h"

namespace B33p
{
    juce::File findBeepFileInCommandLine(const juce::String& commandLine)
    {
        // fromTokens(..., true) keeps quoted runs together, so a path with
        // spaces survives as one token. The leading executable path (when
        // present) and any flags simply fail the .beep test below and are
        // skipped.
        for (auto token : juce::StringArray::fromTokens(commandLine, true))
        {
            token = token.unquoted().trim();
            if (token.isEmpty())
                continue;

            const auto file = juce::File::isAbsolutePath(token)
                                ? juce::File(token)
                                : juce::File::getCurrentWorkingDirectory().getChildFile(token);

            if (file.existsAsFile() && file.hasFileExtension("beep"))
                return file;
        }

        return {};
    }
}
