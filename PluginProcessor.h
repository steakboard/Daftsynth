#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "Synth.h"
#include "Drums.h"
#include "Effects.h"
#include "Modulation.h"
#include "SampleEngine.h"
#include "Arp.h"
#include <unordered_map>
#include <string>
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

    static constexpr int kNumSlots = 12;
    static juce::StringArray modDestIds, modDestNames;   // paramètres modulables (remplis à la création des paramètres)
    static juce::StringArray modSrcNames;                // noms des sources de modulation
    static juce::StringArray syncChoices();              // divisions de synchro tempo
    void randomizeSound (bool mutate);
    // ---- Sampler
    bool loadSampleFile (const juce::File& f);
    juce::String getSampleName() const { auto d = sampler.getData(); return d ? d->name : juce::String(); }
    void startRecording();                  // resampling interne : enregistre la sortie du plugin
    void stopRecording();
    bool isRecording() const { return recState.load() == 1; }
    bool pollRecording();                   // à appeler régulièrement : finalise l'enregistrement terminé
    dl::SampleEngine sampler;
    juce::AudioFormatManager formats;

    // ---- Presets utilisateur, texte (copier / coller, pour l'IA), snapshots Morph
    static juce::File userPresetDir();
    juce::Array<juce::File> listUserPresets() const;
    juce::String exportPresetText (const juce::String& name) const;
    bool importPresetText (const juce::String& text, juce::String* nameOut = nullptr);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::String describeParamsForAI() const;
    void storeSnapshot (int k);
    bool snapshotValid (int k) const { return snapValid[k]; }
    void clearSnapshots() { for (int k = 0; k < 4; ++k) snapValid[k] = false; }

    void applyFmAlgo (int index);                       // charge un algorithme FM (matrice + ratios + sorties)
    static juce::StringArray fmAlgoNames();                   // DICE (tout) / MUT (petite variation)

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
    float pv (const std::string& id) const { return effVals[(size_t) paramIndex.at (id)]; }   // valeur effective (avec modulation)
    float bv (const std::string& id) const { return baseVals[(size_t) paramIndex.at (id)]; }  // valeur de base du potard
    void processSegment (int start, int len, int mode, double ppq, bool havePpq, double bpm, double beatsPerSample);
    void applyPresetMods (const Preset& preset);
    void readSynthParams (dl::SynthParams& p) const;
    void handleMidi (const juce::MidiMessage& m, int mode);
    void renderDrums (float* out, int n);
    void triggerDrum (int note, float vel);

    // Moteurs
    dl::SynthEngine synth;
    dl::DrumVoice drums[16];
    uint32_t drumCounter = 0;

    // Effets
    // enregistrement (resampling)
    juce::AudioBuffer<float> recBuf;
    std::atomic<int> recState { 0 };        // 0 inactif, 1 en cours, 2 arrêt demandé
    std::atomic<int> recPos { 0 };
    int recStopTicks = 0;
    juce::String samplePath;
    // arpégiateur, séquenceur de pas, macros intelligentes, morph
    dl::Arpeggiator arp;
public:
    std::atomic<int> arpLastNote { -1 };
private:
    double arpFreeBeat = 0.0;
    bool arpActive = false;
    dl::StepConfig stepCfg;
    int pmIdx[8] = {}, morphXIdx = 0, morphYIdx = 0, morphOnIdx = 0;
    struct MacroMod { int idx; float off; };
    std::vector<MacroMod> macroMods;
    std::vector<int> morphable;
    std::vector<float> snap[4];
    bool snapValid[4] = { false, false, false, false };
    void applyMorphToBase();
    void computeMacroMods();
    void releaseArp();
    dl::Distortion distortion;
    dl::WowFilter wow;
    float wowPhase = 0.0f, wowRand = 0.0f;
    juce::Random rnd;
    dl::Flanger flanger;
    dl::Equalizer eq;
    dl::MultiBand ott;
    dl::Limiter limiter;
    dl::ModDelay revPre[2];
    dl::Chorus chorus;
    dl::Phaser phaser;
    dl::StereoDelay delay;
    dl::Crusher crusher;
    juce::Reverb reverb;

    std::array<float, kScopeSize> scopeBuf {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<uint32_t> blockCounter { 0 };
    std::atomic<float> rmsOut { 0.0f }, peakL { 0.0f }, peakR { 0.0f };

    // Modulation
    dl::ModEngine modEngine;
    float modSrcValues[dl::NumModSrc] = {};
    struct Slot { int src; int dstIdx; float amt; };
    Slot slots[kNumSlots] = {};
    int numSlots = 0;
    int slotSrcIdx[kNumSlots] = {}, slotDstIdx[kNumSlots] = {}, slotAmtIdx[kNumSlots] = {};
    std::vector<int> destParamIdx;
    int fmRIdx[6] = {}, fmOIdx[6] = {}, fmEIdx[6] = {}, fmMIdx[6][6] = {};

    // Accès rapide aux paramètres
    std::vector<juce::RangedAudioParameter*> allParams;
    std::vector<std::atomic<float>*> rawPtrs;
    std::unordered_map<std::string, int> paramIndex;
    std::vector<float> baseVals, effVals, modNorm;
    std::vector<int> touched;
    std::vector<char> modFlag;
    dl::LfoConfig lfoCfg[4];
    dl::EnvConfig envCfg[2];
    float macroVals[4] = {};
    double curBpm = 120.0;
    float* outL = nullptr;
    float* outR = nullptr;

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
