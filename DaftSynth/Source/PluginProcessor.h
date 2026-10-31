#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "Synth.h"
#include "Drums.h"
#include "Effects.h"
#include "Presets.h"

class DaftSynthProcessor : public juce::AudioProcessor
{
public:
    DaftSynthProcessor();
    ~DaftSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DaftSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Données pour les animations de l'interface (écrites par l'audio, lues par l'éditeur)
    static constexpr int kScopeSize = 4096;
    void copyScope (float* dest, int n) const
    {
        const int w = scopeWrite.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
            dest[i] = scopeBuf[(size_t) ((w - n + i + kScopeSize) & (kScopeSize - 1))];
    }
    uint32_t getBlockCounter() const { return blockCounter.load(); }
    float getRms() const { return rmsOut.load(); }
    float getPeakL() const { return peakL.load(); }
    float getPeakR() const { return peakR.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float pv (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void readSynthParams (dl::SynthParams& p) const;
    void handleMidi (const juce::MidiMessage& m, int mode);
    void renderDrums (float* out, int n);
    void triggerDrum (int note, float vel);

    // Moteurs
    dl::SynthEngine synth;
    dl::DrumVoice drums[16];
    uint32_t drumCounter = 0;

    // Effets
    dl::Chorus chorus;
    dl::Phaser phaser;
    dl::StereoDelay delay;
    dl::Crusher crusher;
    juce::Reverb reverb;

    std::array<float, kScopeSize> scopeBuf {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<uint32_t> blockCounter { 0 };
    std::atomic<float> rmsOut { 0.0f }, peakL { 0.0f }, peakR { 0.0f };

    std::vector<float> mono;
    double sampleRate = 44100.0;
    int currentProgram = 0;
    int lastMode = -1;
    float modWheel = 0.0f;

    // Paramètres des drums, lus à chaque bloc
    int kitIdx = 0;
    float drumTune = 0.0f, drumDecay = 1.0f, drumTone = 0.5f, drumLevel = 0.9f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaftSynthProcessor)
};
