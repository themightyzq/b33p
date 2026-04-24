#include "VoiceRenderer.h"

#include "DSP/Voice.h"

#include <cstdlib>
#include <iostream>
#include <string>

using B33p::Oscillator;
using B33p::Voice;

namespace
{
    struct Options
    {
        double               duration   = 0.5;
        double               sampleRate = 48000.0;
        float                basePitch  = 440.0f;
        Oscillator::Waveform waveform   = Oscillator::Waveform::Sine;
        juce::String         outputPath = "voice.wav";
    };

    bool parseWaveform(const std::string& s, Oscillator::Waveform& out)
    {
        if (s == "sine")     { out = Oscillator::Waveform::Sine;     return true; }
        if (s == "square")   { out = Oscillator::Waveform::Square;   return true; }
        if (s == "triangle") { out = Oscillator::Waveform::Triangle; return true; }
        if (s == "saw")      { out = Oscillator::Waveform::Saw;      return true; }
        if (s == "noise")    { out = Oscillator::Waveform::Noise;    return true; }
        return false;
    }

    void printUsage(const char* argv0)
    {
        std::cerr
            << "Usage: " << argv0 << " [options]\n"
            << "  --duration     SECONDS   note-on time, also pitch-env playback duration (default 0.5)\n"
            << "  --pitch        HZ        base pitch (default 440)\n"
            << "  --waveform     NAME      sine|square|triangle|saw|noise (default sine)\n"
            << "  --sample-rate  HZ        render sample rate (default 48000)\n"
            << "  --output       PATH      output WAV path (default voice.wav)\n";
    }

    // Returns 0 on success, 1 on argument error, 2 on help request.
    int parseArgs(int argc, char** argv, Options& opts)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];

            auto takeValue = [&](const char* name) -> const char*
            {
                if (i + 1 < argc)
                    return argv[++i];
                std::cerr << "Missing value for " << name << "\n";
                return nullptr;
            };

            if (arg == "--help" || arg == "-h")
            {
                printUsage(argv[0]);
                return 2;
            }
            if (arg == "--duration")
            {
                const char* v = takeValue("--duration");
                if (v == nullptr) return 1;
                opts.duration = std::atof(v);
            }
            else if (arg == "--pitch")
            {
                const char* v = takeValue("--pitch");
                if (v == nullptr) return 1;
                opts.basePitch = static_cast<float>(std::atof(v));
            }
            else if (arg == "--waveform")
            {
                const char* v = takeValue("--waveform");
                if (v == nullptr) return 1;
                if (! parseWaveform(v, opts.waveform))
                {
                    std::cerr << "Unknown waveform: " << v << "\n";
                    return 1;
                }
            }
            else if (arg == "--sample-rate")
            {
                const char* v = takeValue("--sample-rate");
                if (v == nullptr) return 1;
                opts.sampleRate = std::atof(v);
            }
            else if (arg == "--output")
            {
                const char* v = takeValue("--output");
                if (v == nullptr) return 1;
                opts.outputPath = juce::String(v);
            }
            else
            {
                std::cerr << "Unknown argument: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }
        return 0;
    }

    void configureDemoVoice(Voice& voice, const Options& opts)
    {
        voice.setWaveform(opts.waveform);
        voice.setBasePitchHz(opts.basePitch);

        voice.setAmpAttack(0.005f);
        voice.setAmpDecay(0.1f);
        voice.setAmpSustain(0.7f);
        voice.setAmpRelease(0.15f);

        voice.setFilterCutoff(12000.0f);
        voice.setFilterResonance(0.707f);

        voice.setBitcrushBitDepth(16.0f);
        voice.setBitcrushSampleRate(static_cast<float>(opts.sampleRate));

        voice.setDistortionDrive(1.0f);
        voice.setGain(0.5f);
    }
}

int main(int argc, char** argv)
{
    Options opts;
    const int parseRc = parseArgs(argc, argv, opts);
    if (parseRc == 2)
        return 0;      // help printed
    if (parseRc != 0)
        return parseRc;

    Voice voice;
    voice.prepare(opts.sampleRate);
    configureDemoVoice(voice, opts);

    const auto buffer = B33p::VoiceRenderer::renderEvent(
        voice, opts.sampleRate, opts.duration, /*maxTailSeconds=*/2.0);

    const juce::File outFile
        = juce::File::getCurrentWorkingDirectory().getChildFile(opts.outputPath);

    if (! B33p::VoiceRenderer::writeWavMono16(buffer, opts.sampleRate, outFile))
    {
        std::cerr << "Failed to write WAV to " << outFile.getFullPathName() << "\n";
        return 3;
    }

    std::cout << "Wrote " << buffer.getNumSamples() << " samples ("
              << (static_cast<double>(buffer.getNumSamples()) / opts.sampleRate)
              << " s) to " << outFile.getFullPathName() << "\n";
    return 0;
}
