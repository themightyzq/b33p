#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Harness/VoiceRenderer.h"

#include "DSP/Voice.h"

using B33p::Oscillator;
using B33p::Voice;
using B33p::VoiceRenderer::renderEvent;
using B33p::VoiceRenderer::writeWavMono16;
using Catch::Approx;

namespace
{
    constexpr double kSampleRate = 48000.0;

    void configureSmokeVoice(Voice& voice)
    {
        voice.setWaveform(Oscillator::Waveform::Sine);
        voice.setBasePitchHz(440.0f);
        voice.setAmpAttack(0.0f);
        voice.setAmpDecay(0.0f);
        voice.setAmpSustain(1.0f);
        voice.setAmpRelease(0.02f);
        voice.setFilterCutoff(20000.0f);
        voice.setFilterResonance(0.707f);
        voice.setBitcrushBitDepth(16.0f);
        voice.setBitcrushSampleRate(static_cast<float>(kSampleRate));
        voice.setDistortionDrive(0.5f);
        voice.setGain(0.5f);
    }
}

TEST_CASE("VoiceRenderer: renderEvent fills held samples and trims the tail", "[harness][renderer]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureSmokeVoice(voice);

    const double heldSeconds = 0.1;
    const auto buffer = renderEvent(voice, kSampleRate, heldSeconds, /*maxTailSeconds=*/0.2);

    const int heldSamples = static_cast<int>(heldSeconds * kSampleRate);

    // Buffer must be long enough to hold the held section plus the
    // release tail — and short enough to confirm the trim happened.
    REQUIRE(buffer.getNumSamples() >= heldSamples);
    REQUIRE(buffer.getNumSamples() < heldSamples + static_cast<int>(0.2 * kSampleRate));

    // Non-silent somewhere in the held region (voice actually rendered).
    float peak = 0.0f;
    for (int i = 0; i < heldSamples; ++i)
        peak = std::max(peak, std::fabs(buffer.getSample(0, i)));
    REQUIRE(peak > 0.05f);
}

TEST_CASE("VoiceRenderer: zero-duration event produces an empty buffer, not a crash", "[harness][renderer]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureSmokeVoice(voice);

    const auto buffer = renderEvent(voice, kSampleRate, 0.0, 0.0);
    REQUIRE(buffer.getNumSamples() == 0);
}

TEST_CASE("VoiceRenderer: writeWavMono16 writes a readable WAV that round-trips", "[harness][renderer]")
{
    Voice voice;
    voice.prepare(kSampleRate);
    configureSmokeVoice(voice);

    const auto buffer = renderEvent(voice, kSampleRate, 0.05, 0.1);
    REQUIRE(buffer.getNumSamples() > 0);

    const juce::File tempFile = juce::File::createTempFile(".wav");
    const bool wrote = writeWavMono16(buffer, kSampleRate, tempFile);
    REQUIRE(wrote);
    REQUIRE(tempFile.existsAsFile());
    REQUIRE(tempFile.getSize() > 44);  // bigger than a bare WAV header

    // Round-trip: a fresh WavAudioFormat must parse the file back and
    // report the same sample count and sample rate we wrote.
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader {
        formatManager.createReaderFor(tempFile) };
    REQUIRE(reader != nullptr);
    REQUIRE(reader->sampleRate   == Approx(kSampleRate).margin(1e-6));
    REQUIRE(reader->numChannels  == 1u);
    REQUIRE(reader->bitsPerSample == 16u);
    REQUIRE(static_cast<int>(reader->lengthInSamples) == buffer.getNumSamples());

    reader.reset();
    tempFile.deleteFile();
}
