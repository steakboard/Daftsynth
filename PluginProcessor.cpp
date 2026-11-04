#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Définition des paramètres
//==============================================================================
juce::StringArray DaftSynthProcessor::modDestIds, DaftSynthProcessor::modDestNames;
juce::StringArray DaftSynthProcessor::modSrcNames { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Env 1", "Env 2",
                                                    "Velocity", "Mod Wheel", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Random", "Step Seq" };

juce::StringArray DaftSynthProcessor::syncChoices()
{
    return { "Free", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T", "1/4.", "1/8.", "1/16." };
}

juce::AudioProcessorValueTreeState::ParameterLayout DaftSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    bool recordDest = true;
    modDestIds.clear();
    modDestNames.clear();

    auto addF = [&layout, &recordDest] (const juce::String& id, const juce::String& name, float lo, float hi, float def,
                                        float centre = 0.0f, const char* unit = "", float interval = 0.0f)
    {
        if (recordDest)
        {
            modDestIds.add (id);
            modDestNames.add (name);
        }

        juce::NormalisableRange<float> range (lo, hi, interval);
        if (centre > 0.0f) range.setSkewForCentre (centre);

        const juce::String u (unit);
        const int decimals = (hi - lo) < 10.0f ? 2 : ((hi - lo) < 100.0f ? 1 : 0);
        auto attrs = juce::AudioParameterFloatAttributes()
                         .withStringFromValueFunction ([u, decimals] (float v, int) { return juce::String (v, decimals) + u; });

        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def, attrs));
    };

    auto addC = [&layout] (const juce::String& id, const juce::String& name, juce::StringArray choices, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, choices, def));
    };

    addC ("mode", "Mode", { "Poly (Juno)", "Mono (Moog)", "Acid (303)", "Drums", "Sample" }, 0);

    // Oscillateurs
    addF ("octave", "Octave", -2, 2, 0, 0, "", 1);
    addF ("saw", "Saw Level", 0, 1, 0.8f);
    addF ("pulse", "Pulse Level", 0, 1, 0);
    addF ("pw", "Pulse Width", 0.05f, 0.95f, 0.5f);
    addF ("sub", "Sub Level", 0, 1, 0);
    addF ("osc2", "Osc2 Level", 0, 1, 0);
    addF ("osc2Semi", "Osc2 Semitones", -24, 24, 0, 0, " st", 1);
    addF ("osc2Detune", "Osc2 Detune", -50, 50, 0, 0, " ct");
    addF ("noise", "Noise Level", 0, 1, 0);
    addF ("drift", "Analog Drift", 0, 1, 0);
    addF ("ringMix", "Ring Mod Mix", 0, 1, 0);
    addF ("ringRatio", "Ring Mod Ratio", 0.25f, 8, 2.01f, 2, "x");
    addF ("fmLevel", "FM Level", 0, 1, 0);
    addF ("fmAmt", "FM Amount", 0, 12, 3, 3);
    addF ("fmRatio", "FM Ratio", 0.25f, 8, 1, 0, "x", 0.25f);
    addF ("fmFb", "FM Feedback", 0, 1, 0);
    addF ("fmEnv", "FM Env Amount", 0, 1, 0);
    addF ("fm6Level", "FM 6-Op Level", 0, 1, 0);
    addF ("fm6Index", "FM 6-Op Index", 0, 8, 2, 2);
    for (int i = 1; i <= 6; ++i)
        addF ("fmO" + juce::String (i), "FM Op" + juce::String (i) + " Output", 0, 1, i == 1 ? 1.0f : 0.0f);
    addC ("wtTable", "Wavetable", { "Basic", "Growl Sweep", "Digital", "PWM", "Vowels", "FM Sweep", "Sync Sweep" }, 0);
    addF ("wtLevel", "Wavetable Level", 0, 1, 0);
    addF ("wtPos", "Wavetable Position", 0, 1, 0);
    addF ("wtOct", "Wavetable Octave", -2, 2, 0, 0, "", 1);
    addF ("uniVoices", "Unison Voices", 1, 7, 1, 0, "", 1);
    addF ("uniDetune", "Unison Detune", 0, 100, 12, 0, " ct");

    // Filtre
    addF ("cutoff", "Filter Cutoff", 20, 20000, 8000, 1000, " Hz");
    addF ("res", "Filter Resonance", 0, 1, 0.1f);
    addF ("drive", "Filter Drive", 0, 1, 0);
    addF ("fenv", "Filter Env Amount", -1, 1, 0.3f);
    addF ("keytrack", "Filter Key Track", 0, 1, 0.5f);
    addF ("hpf", "High-Pass", 0, 1, 0);

    // Enveloppe amplitude
    addF ("aA", "Amp Attack", 0.001f, 5, 0.005f, 0.2f, " s");
    addF ("aD", "Amp Decay", 0.005f, 5, 0.3f, 0.5f, " s");
    addF ("aS", "Amp Sustain", 0, 1, 0.8f);
    addF ("aR", "Amp Release", 0.005f, 8, 0.2f, 0.5f, " s");

    // Enveloppe filtre
    addF ("fA", "Filter Attack", 0.001f, 5, 0.005f, 0.2f, " s");
    addF ("fD", "Filter Decay", 0.005f, 5, 0.4f, 0.5f, " s");
    addF ("fS", "Filter Sustain", 0, 1, 0.2f);
    addF ("fR", "Filter Release", 0.005f, 8, 0.3f, 0.5f, " s");

    // LFO
    addC ("lfoShape", "LFO Shape", { "Sine", "Tri", "Sqr", "S&H" }, 0);
    addC ("lfoSync", "LFO Sync", syncChoices(), 0);
    addF ("lfoRate", "LFO Rate", 0.05f, 30, 4, 3, " Hz");
    addF ("lfoPitch", "LFO > Pitch", 0, 1, 0);
    addF ("lfoCutoff", "LFO > Cutoff", 0, 1, 0);
    addF ("lfoPwm", "LFO > PWM", 0, 1, 0);

    // Jeu
    addF ("glide", "Glide", 0, 2, 0, 0.2f, " s");
    addF ("accent", "Acid Accent", 0, 1, 0.6f);
    addF ("volume", "Master Volume", 0, 1, 0.7f);

    // Effets
    addC ("distType", "Dist Type", { "Soft", "Tube", "Hard Clip", "Wavefold", "Fuzz", "Rectify", "Tape" }, 0);
    addF ("dist", "Distortion Drive", 0, 1, 0);
    addF ("distBias", "Dist Bias", -1, 1, 0);
    addF ("distTone", "Dist Tone", 0, 1, 1);
    addF ("distMix", "Dist Mix", 0, 1, 1);
    addF ("distOut", "Dist Output", -24, 12, 0, 0, " dB");
    addF ("crushBits", "Crush Bits", 4, 16, 16, 0, " bit", 1);
    addF ("crushRate", "Crush Rate", 2000, 48000, 48000, 12000, " Hz");
    addC ("chorus", "Chorus", { "Off", "I", "II", "I+II" }, 0);
    addF ("phaserMix", "Phaser Mix", 0, 1, 0);
    addC ("phaserSync", "Phaser Sync", syncChoices(), 0);
    addF ("phaserRate", "Phaser Rate", 0.05f, 8, 0.4f, 0.5f, " Hz");
    addF ("phaserDepth", "Phaser Depth", 0, 1, 0.7f);
    addF ("phaserFb", "Phaser Feedback", 0, 0.95f, 0.5f);
    addF ("delayMix", "Delay Mix", 0, 1, 0);
    addC ("delaySync", "Delay Sync", syncChoices(), 0);
    addF ("delayTime", "Delay Time", 10, 1000, 375, 300, " ms");
    addF ("delayFb", "Delay Feedback", 0, 0.95f, 0.35f);
    addF ("reverbMix", "Reverb Mix", 0, 1, 0);
    addF ("reverbSize", "Reverb Size", 0, 1, 0.5f);
    addF ("pump", "Pump Amount", 0, 1, 0);
    addC ("pumpRate", "Pump Rate", { "1/2", "1/4", "1/8", "1/16" }, 1);
    addF ("pumpRel", "Pump Release", 0.1f, 1, 0.5f);

    // Effets détaillés
    addF ("chorusMix", "Chorus Mix", 0, 1, 1);
    addC ("phaserStages", "Phaser Stages", { "4", "6", "8" }, 1);
    addF ("flMix", "Flanger Mix", 0, 1, 0);
    addF ("flRate", "Flanger Rate", 0.05f, 5, 0.3f, 0.5f, " Hz");
    addF ("flDepth", "Flanger Depth", 0, 1, 0.5f);
    addF ("flFb", "Flanger Feedback", -0.95f, 0.95f, 0.4f);
    addF ("flDelay", "Flanger Delay", 0.5f, 8, 2, 2, " ms");
    addC ("delayPing", "Delay Ping-Pong", { "Off", "On" }, 0);
    addF ("delayTone", "Delay Tone", 500, 16000, 4000, 4000, " Hz");
    addF ("reverbPre", "Reverb Pre-Delay", 0, 200, 0, 0, " ms");
    addF ("reverbDamp", "Reverb Damping", 0, 1, 0.4f);
    addF ("reverbWidth", "Reverb Width", 0, 1, 1);

    // Égaliseur, compresseur multibande (OTT) et sortie
    addF ("eqLowGain", "EQ Low Gain", -15, 15, 0, 0, " dB");
    addF ("eqLowFreq", "EQ Low Freq", 40, 400, 120, 120, " Hz");
    addF ("eqMidGain", "EQ Mid Gain", -15, 15, 0, 0, " dB");
    addF ("eqMidFreq", "EQ Mid Freq", 200, 8000, 1000, 1000, " Hz");
    addF ("eqMidQ", "EQ Mid Q", 0.3f, 4, 1, 1);
    addF ("eqHighGain", "EQ High Gain", -15, 15, 0, 0, " dB");
    addF ("eqHighFreq", "EQ High Freq", 2000, 16000, 6000, 6000, " Hz");
    addF ("ottMix", "OTT Mix", 0, 1, 0);
    addF ("ottDepth", "OTT Depth", 0, 1, 0.7f);
    addF ("ottTime", "OTT Time", 0.2f, 3, 1, 1);
    addF ("ottLow", "OTT Low Gain", -12, 12, 0, 0, " dB");
    addF ("ottMid", "OTT Mid Gain", -12, 12, 0, 0, " dB");
    addF ("ottHigh", "OTT High Gain", -12, 12, 0, 0, " dB");
    addF ("width", "Stereo Width", 0, 2, 1);
    addF ("boost", "Output Boost", 0, 24, 0, 0, " dB");
    addF ("limCeil", "Limiter Ceiling", -12, 0, -0.3f, 0, " dB");
    addF ("limRel", "Limiter Release", 10, 500, 80, 80, " ms");

    // Drums
    addC ("kit", "Drum Kit", { "TR-808", "TR-909", "LinnDrum", "Oberheim DMX" }, 0);
    addF ("drumTune", "Drum Tune", -12, 12, 0, 0, " st");
    addF ("drumDecay", "Drum Decay", 0.3f, 2, 1, 1, "x");
    addF ("drumTone", "Drum Tone", 0, 1, 0.5f);
    addF ("drumLevel", "Drum Level", 0, 1.5f, 0.9f);

    // Sampler
    addF ("sampleLevel", "Sample Level", 0, 2, 1);
    addF ("sampleTune", "Sample Tune", -24, 24, 0, 0, " st", 0.01f);
    addF ("sampleAttack", "Sample Attack", 0.0005f, 2, 0.002f, 0.1f, " s");
    addF ("sampleRelease", "Sample Release", 0.005f, 4, 0.1f, 0.3f, " s");

    // WOW : filtre multi-modes / voyelles + wobble
    addC ("wowType", "WOW Type", { "LP 12", "LP 24", "HP 12", "BP", "Notch", "Peak", "Comb", "Vowel" }, 0);
    addC ("wowPos", "WOW Position", { "Pre Dist", "Post Dist" }, 1);
    addF ("wowFreq", "WOW Frequency", 20, 18000, 1000, 1000, " Hz");
    addF ("wowRes", "WOW Resonance", 0, 1, 0.3f);
    addF ("wowVowel", "WOW Vowel", 0, 1, 0);
    addF ("wowDrive", "WOW Drive", 0, 1, 0);
    addF ("wowMix", "WOW Mix", 0, 1, 0);
    addC ("wowLfoShape", "WOW Wobble Shape", { "Sine", "Tri", "Saw", "Square" }, 0);
    addC ("wowLfoSync", "WOW Wobble Sync", syncChoices(), 0);
    addF ("wowLfoRate", "WOW Wobble Rate", 0.05f, 20, 2, 2, " Hz");
    addF ("wowLfoDepth", "WOW Wobble Depth", 0, 1, 0);

    // ---------------- Modulation (non modulable elle-même)
    recordDest = false;

    // Arpégiateur
    addC ("sync2", "Osc2 Hard Sync", { "Off", "On" }, 0);
    addC ("arpOn", "Arp On", { "Off", "On" }, 0);
    addC ("arpMode", "Arp Mode", { "Up", "Down", "Up/Down", "Random", "As Played", "Chord" }, 0);
    addC ("arpRate", "Arp Rate", { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" }, 2);
    addF ("arpOct", "Arp Octaves", 1, 4, 1, 0, "", 1);
    addF ("arpGate", "Arp Gate", 0.05f, 1, 0.6f);
    addF ("arpSwing", "Arp Swing", 0, 0.6f, 0);
    addC ("arpHold", "Arp Hold", { "Off", "On" }, 0);

    // Séquenceur de pas (source de modulation "Step Seq")
    addF ("stepLen", "Step Length", 1, 16, 8, 0, "", 1);
    addC ("stepSync", "Step Sync", syncChoices(), 5);
    addF ("stepRate", "Step Rate", 0.5f, 30, 8, 8, " Hz");
    addF ("stepGlide", "Step Glide", 0, 1, 0);
    for (int i = 1; i <= 16; ++i) addF ("step" + juce::String (i), "Step " + juce::String (i), 0, 1, 0.5f);

    // Macros intelligentes (0.5 = neutre) et Morph Pad
    {
        static const char* pmNames[8] = { "Character", "Brightness", "Warmth", "Movement", "Aggression", "Width", "Texture", "Space" };
        for (int i = 1; i <= 8; ++i) addF ("pm" + juce::String (i), pmNames[i - 1], 0, 1, 0.5f);
    }
    addC ("morphOn", "Morph On", { "Off", "On" }, 0);
    addF ("morphX", "Morph X", 0, 1, 0);
    addF ("morphY", "Morph Y", 0, 1, 0);

    addC ("sampleMode", "Sample Mode", { "Slices", "Chromatic" }, 0);
    addC ("sampleGate", "Sample Gate", { "One-shot", "Gate" }, 1);
    addC ("sampleLoop", "Sample Loop", { "Off", "On" }, 0);
    addF ("sampleRoot", "Sample Root Note", 0, 127, 60, 0, "", 1);
    addF ("sliceNote", "First Slice Note", 0, 120, 48, 0, "", 1);
    addF ("sampleSens", "Slice Sensitivity", 0, 1, 0.5f);
    addF ("sliceGrid", "Grid Slices", 1, 64, 8, 0, "", 1);

    for (int i = 1; i <= 6; ++i)
    {
        addF ("fmR" + juce::String (i), "FM Op" + juce::String (i) + " Ratio", 0.25f, 16, 1, 2, "x", 0.25f);
        addF ("fmE" + juce::String (i), "FM Op" + juce::String (i) + " Envelope", 0, 1, 0);
        for (int j = 1; j <= 6; ++j)
            addF ("fmM" + juce::String (i) + juce::String (j), "FM Op" + juce::String (i) + " Mod by Op" + juce::String (j), 0, 1, 0);
    }

    for (int i = 1; i <= 4; ++i)
    {
        const juce::String p = "lfo" + juce::String (i), n = "LFO " + juce::String (i) + " ";
        addC (p + "Shape", n + "Shape", { "Sine", "Tri", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Rnd" }, 0);
        addF (p + "Rate", n + "Rate", 0.02f, 40, 2, 2, " Hz");
        addC (p + "Sync", n + "Sync", syncChoices(), 0);
        addC (p + "Retrig", n + "Retrigger", { "Free", "Note" }, 0);
    }

    for (int i = 1; i <= 2; ++i)
    {
        const juce::String p = "env" + juce::String (i), n = "Env " + juce::String (i) + " ";
        addF (p + "A", n + "Attack", 0.001f, 5, 0.01f, 0.2f, " s");
        addF (p + "D", n + "Decay", 0.005f, 5, 0.3f, 0.5f, " s");
        addF (p + "S", n + "Sustain", 0, 1, 0.5f);
        addF (p + "R", n + "Release", 0.005f, 8, 0.3f, 0.5f, " s");
    }

    for (int i = 1; i <= 4; ++i)
        addF ("macro" + juce::String (i), "Macro " + juce::String (i), 0, 1, 0);

    juce::StringArray dstChoices;
    dstChoices.add ("None");
    dstChoices.addArray (modDestNames);

    for (int i = 1; i <= kNumSlots; ++i)
    {
        const juce::String p = "m" + juce::String (i), n = "Mod " + juce::String (i) + " ";
        addC (p + "Src", n + "Source", modSrcNames, 0);
        addC (p + "Dst", n + "Target", dstChoices, 0);
        addF (p + "Amt", n + "Amount", -1, 1, 0);
    }

    return layout;
}

//==============================================================================
DaftSynthProcessor::DaftSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DAFTSYNTH", createLayout())
{
    formats.registerBasicFormats();
    dl::WaveTables::get();   // construit les wavetables au chargement (pas dans le thread audio)

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            paramIndex[rp->paramID.toStdString()] = (int) allParams.size();
            allParams.push_back (rp);
            rawPtrs.push_back (apvts.getRawParameterValue (rp->paramID));
        }

    baseVals.assign (allParams.size(), 0.0f);
    effVals.assign (allParams.size(), 0.0f);
    modNorm.assign (allParams.size(), 0.0f);
    modFlag.assign (allParams.size(), 0);
    touched.reserve (kNumSlots);

    for (int i = 0; i < modDestIds.size(); ++i)
        destParamIdx.push_back (paramIndex.at (modDestIds[i].toStdString()));

    for (int i = 0; i < 8; ++i) pmIdx[i] = paramIndex.at ("pm" + std::to_string (i + 1));
    morphXIdx = paramIndex.at ("morphX");
    morphYIdx = paramIndex.at ("morphY");
    morphOnIdx = paramIndex.at ("morphOn");
    for (size_t i = 0; i < allParams.size(); ++i)
    {
        const std::string id = allParams[i]->paramID.toStdString();
        const bool matrixSlot = id.size() > 2 && id[0] == 'm' && std::isdigit ((unsigned char) id[1]);
        const bool excluded = matrixSlot || id.rfind ("morph", 0) == 0 || id.rfind ("pm", 0) == 0 || id.rfind ("sample", 0) == 0
                              || id.rfind ("slice", 0) == 0 || id == "mode" || id == "volume" || id.rfind ("arp", 0) == 0 || id.rfind ("step", 0) == 0;
        if (! excluded) morphable.push_back ((int) i);
    }

    for (int i = 0; i < 6; ++i)
    {
        const std::string n = std::to_string (i + 1);
        fmRIdx[i] = paramIndex.at ("fmR" + n);
        fmOIdx[i] = paramIndex.at ("fmO" + n);
        fmEIdx[i] = paramIndex.at ("fmE" + n);
        for (int j = 0; j < 6; ++j) fmMIdx[i][j] = paramIndex.at ("fmM" + n + std::to_string (j + 1));
    }

    for (int i = 0; i < kNumSlots; ++i)
    {
        const std::string p = "m" + std::to_string (i + 1);
        slotSrcIdx[i] = paramIndex.at (p + "Src");
        slotDstIdx[i] = paramIndex.at (p + "Dst");
        slotAmtIdx[i] = paramIndex.at (p + "Amt");
    }
}

bool DaftSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DaftSynthProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    synth.prepare (sr);
    modEngine.prepare ((float) sr);
    sampler.prepare (sr);
    for (auto& d : drums) d.active = false;

    wow.prepare ((float) sr);
    flanger.prepare ((float) sr);
    ott.prepare ((float) sr);
    limiter.prepare ((float) sr);
    revPre[0].init ((int) (sr * 0.21) + 8);
    revPre[1].init ((int) (sr * 0.21) + 8);
    chorus.prepare ((float) sr);
    phaser.prepare ((float) sr);
    delay.prepare ((float) sr);
    reverb.setSampleRate (sr);
    reverb.reset();

    mono.assign ((size_t) std::max (samplesPerBlock, 1), 0.0f);
    lastMode = -1;
}

//==============================================================================
// Presets (programmes)
//==============================================================================
void DaftSynthProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;

    // 1) tout remettre aux valeurs par défaut
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    // 2) appliquer les valeurs du preset
    for (const auto& kv : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));

    // 3) assignations de modulation
    applyPresetMods (presets[(size_t) index]);
}

void DaftSynthProcessor::applyPresetMods (const Preset& preset)
{
    int slot = 1;
    for (const auto& m : preset.mods)
    {
        if (slot > kNumSlots) break;
        const int src = modSrcNames.indexOf (m.src);
        const int dst = modDestIds.indexOf (m.dst) + 1;
        if (src <= 0 || dst <= 0) continue;

        const juce::String p = "m" + juce::String (slot);
        if (auto* ps = apvts.getParameter (p + "Src")) ps->setValueNotifyingHost (ps->convertTo0to1 ((float) src));
        if (auto* pd = apvts.getParameter (p + "Dst")) pd->setValueNotifyingHost (pd->convertTo0to1 ((float) dst));
        if (auto* pa = apvts.getParameter (p + "Amt")) pa->setValueNotifyingHost (pa->convertTo0to1 (m.amount));
        ++slot;
    }
}


namespace
{
struct MacroEntry { const char* id; float w[8]; };
// Poids de chaque macro sur les paramètres : Character, Brightness, Warmth, Movement, Aggression, Width, Texture, Space
const MacroEntry kMacroTable[] = {
    { "saw",         { -0.5f, 0, 0, 0, 0, 0, 0, 0 } },
    { "wtLevel",     { 0.5f, 0, 0, 0, 0, 0, 0, 0 } },
    { "fmLevel",     { 0.4f, 0, 0, 0, 0, 0, 0, 0 } },
    { "cutoff",      { 0, 0.5f, -0.15f, 0, 0, 0, 0, 0 } },
    { "wowFreq",     { 0, 0.3f, 0, 0, 0, 0, 0, 0 } },
    { "eqHighGain",  { 0, 0.3f, -0.2f, 0, 0, 0, 0, 0 } },
    { "sub",         { 0, 0, 0.4f, 0, 0, 0, 0, 0 } },
    { "eqLowGain",   { 0, 0, 0.3f, 0, 0, 0, 0, 0 } },
    { "distTone",    { 0, 0, -0.25f, 0, 0, 0, 0, 0 } },
    { "lfoCutoff",   { 0, 0, 0, 0.5f, 0, 0, 0, 0 } },
    { "lfoPwm",      { 0, 0, 0, 0.4f, 0, 0, 0, 0 } },
    { "wowLfoDepth", { 0, 0, 0, 0.4f, 0, 0, 0, 0 } },
    { "phaserMix",   { 0, 0, 0, 0.3f, 0, 0.2f, 0, 0 } },
    { "uniDetune",   { 0, 0, 0, 0, 0, 0.3f, 0, 0 } },
    { "dist",        { 0, 0, 0, 0, 0.5f, 0, 0, 0 } },
    { "drive",       { 0, 0, 0, 0, 0.3f, 0, 0, 0 } },
    { "res",         { 0, 0, 0, 0, 0.3f, 0, 0, 0 } },
    { "crushBits",   { 0, 0, 0, 0, -0.3f, 0, -0.3f, 0 } },
    { "ottMix",      { 0, 0, 0, 0, 0.4f, 0, 0, 0 } },
    { "wowDrive",    { 0, 0, 0, 0, 0.3f, 0, 0, 0 } },
    { "chorusMix",   { 0, 0, 0, 0, 0, 0.4f, 0, 0 } },
    { "flMix",       { 0, 0, 0, 0, 0, 0.3f, 0, 0 } },
    { "reverbWidth", { 0, 0, 0, 0, 0, 0.4f, 0, 0 } },
    { "noise",       { 0, 0, 0, 0, 0, 0, 0.5f, 0 } },
    { "wtPos",       { 0, 0, 0, 0, 0, 0, 0.4f, 0 } },
    { "fmFb",        { 0, 0, 0, 0, 0, 0, 0.3f, 0 } },
    { "reverbMix",   { 0, 0, 0, 0, 0, 0, 0, 0.5f } },
    { "reverbSize",  { 0, 0, 0, 0, 0, 0, 0, 0.4f } },
    { "delayMix",    { 0, 0, 0, 0, 0, 0, 0, 0.4f } },
    { "delayFb",     { 0, 0, 0, 0, 0, 0, 0, 0.2f } },
};
} // namespace

void DaftSynthProcessor::computeMacroMods()
{
    macroMods.clear();
    float m[8];
    bool any = false;
    for (int k = 0; k < 8; ++k)
    {
        m[k] = (baseVals[(size_t) pmIdx[k]] - 0.5f) * 2.0f;
        any = any || std::fabs (m[k]) > 1.0e-3f;
    }
    if (! any) return;

    for (const auto& e : kMacroTable)
    {
        float off = 0.0f;
        for (int k = 0; k < 8; ++k) off += m[k] * e.w[k];
        if (std::fabs (off) > 1.0e-4f) macroMods.push_back ({ paramIndex.at (e.id), off });
    }
}

void DaftSynthProcessor::storeSnapshot (int k)
{
    if (k < 0 || k > 3) return;
    snap[k].assign (allParams.size(), 0.0f);
    for (size_t i = 0; i < allParams.size(); ++i) snap[k][i] = allParams[i]->getValue();
    snapValid[k] = true;
}

// Morph Pad : interpole les valeurs des 4 snapshots (A, B, C, D) selon la position XY
void DaftSynthProcessor::applyMorphToBase()
{
    if (baseVals[(size_t) morphOnIdx] < 0.5f || ! snapValid[0]) return;

    const float x = baseVals[(size_t) morphXIdx], y = baseVals[(size_t) morphYIdx];
    const float w[4] = { (1 - x) * (1 - y), x * (1 - y), (1 - x) * y, x * y };   // A bas-gauche, B bas-droite, C haut-gauche, D haut-droite

    int best = 0;
    for (int k = 1; k < 4; ++k) if (snapValid[k] && w[k] > w[best]) best = k;

    for (int idx : morphable)
    {
        auto* p = allParams[(size_t) idx];
        float norm;
        if (dynamic_cast<juce::AudioParameterChoice*> (p) != nullptr)
        {
            norm = snap[best][(size_t) idx];
        }
        else
        {
            float sum = 0.0f, wsum = 0.0f;
            for (int k = 0; k < 4; ++k)
                if (snapValid[k]) { sum += w[k] * snap[k][(size_t) idx]; wsum += w[k]; }
            norm = wsum > 1.0e-6f ? sum / wsum : snap[0][(size_t) idx];
        }
        baseVals[(size_t) idx] = p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, norm));
    }
}

void DaftSynthProcessor::releaseArp()
{
    arp.releaseAll ([this] (int n) { synth.noteOff (n); modEngine.noteOff(); });
}

// ------------------------------------------------------------------ presets utilisateur et texte
juce::File DaftSynthProcessor::userPresetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("DaftSynth").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::Array<juce::File> DaftSynthProcessor::listUserPresets() const
{
    auto files = userPresetDir().findChildFiles (juce::File::findFiles, false, "*.dspreset");
    files.sort();
    return files;
}

juce::String DaftSynthProcessor::exportPresetText (const juce::String& name) const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("name", name);
    auto* values = new juce::DynamicObject();
    juce::Array<juce::var> mods;

    for (size_t i = 0; i < allParams.size(); ++i)
    {
        auto* p = allParams[i];
        const juce::String id = p->paramID;
        if (id.startsWith ("morph") || id.startsWith ("slice") || id == "sampleSens") continue;
        if (id.length() > 2 && id[0] == 'm' && juce::CharacterFunctions::isDigit (id[1])) continue;   // matrice : exportée dans "mods"

        if (std::fabs (p->getValue() - p->getDefaultValue()) < 1.0e-4f) continue;
        values->setProperty (id, (double) p->convertFrom0to1 (p->getValue()));
    }

    for (int s = 1; s <= kNumSlots; ++s)
    {
        const juce::String ps = "m" + juce::String (s);
        const int src = juce::roundToInt (apvts.getParameter (ps + "Src")->convertFrom0to1 (apvts.getParameter (ps + "Src")->getValue()));
        const int dst = juce::roundToInt (apvts.getParameter (ps + "Dst")->convertFrom0to1 (apvts.getParameter (ps + "Dst")->getValue()));
        auto* pa = apvts.getParameter (ps + "Amt");
        const float amt = pa->convertFrom0to1 (pa->getValue());
        if (src > 0 && dst > 0 && dst <= modDestIds.size() && std::fabs (amt) > 1.0e-4f)
        {
            auto* m = new juce::DynamicObject();
            m->setProperty ("src", modSrcNames[src]);
            m->setProperty ("dst", modDestIds[dst - 1]);
            m->setProperty ("amount", (double) amt);
            mods.add (juce::var (m));
        }
    }

    root->setProperty ("values", juce::var (values));
    root->setProperty ("mods", juce::var (mods));
    return juce::JSON::toString (juce::var (root), false);
}

bool DaftSynthProcessor::importPresetText (const juce::String& text, juce::String* nameOut)
{
    const juce::var v = juce::JSON::parse (text.trim());
    auto* root = v.getDynamicObject();
    if (root == nullptr) return false;
    auto* values = root->getProperty ("values").getDynamicObject();
    if (values == nullptr) return false;

    if (nameOut != nullptr) *nameOut = root->getProperty ("name").toString();

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const juce::String id = rp->paramID;
            if (id.startsWith ("morph") || id.startsWith ("slice") || id == "sampleSens") continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }

    for (const auto& nv : values->getProperties())
        if (auto* p = apvts.getParameter (nv.name.toString()))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) (double) nv.value));

    int slot = 1;
    if (auto* arr = root->getProperty ("mods").getArray())
        for (const auto& mv : *arr)
        {
            if (slot > kNumSlots) break;
            const int src = modSrcNames.indexOf (mv.getProperty ("src", "").toString());
            const int dst = modDestIds.indexOf (mv.getProperty ("dst", "").toString()) + 1;
            if (src <= 0 || dst <= 0) continue;
            const juce::String ps = "m" + juce::String (slot);
            if (auto* a = apvts.getParameter (ps + "Src")) a->setValueNotifyingHost (a->convertTo0to1 ((float) src));
            if (auto* b = apvts.getParameter (ps + "Dst")) b->setValueNotifyingHost (b->convertTo0to1 ((float) dst));
            if (auto* d = apvts.getParameter (ps + "Amt")) d->setValueNotifyingHost (d->convertTo0to1 ((float) (double) mv.getProperty ("amount", 0.0)));
            ++slot;
        }
    return true;
}

bool DaftSynthProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    return userPresetDir().getChildFile (clean + ".dspreset").replaceWithText (exportPresetText (name.trim()));
}

bool DaftSynthProcessor::loadUserPreset (const juce::File& f)
{
    return f.existsAsFile() && importPresetText (f.loadFileAsString());
}

// Texte à coller dans un chat avec une IA : liste des paramètres + format attendu pour recevoir un preset
juce::String DaftSynthProcessor::describeParamsForAI() const
{
    juce::String t;
    t << "Je veux creer un son pour le synthe DaftSynth. Reponds UNIQUEMENT avec un JSON de ce format (rien d'autre) :\n"
      << "{\"name\":\"Nom du son\",\"values\":{\"id_du_parametre\":valeur,...},\"mods\":[{\"src\":\"LFO 1\",\"dst\":\"cutoff\",\"amount\":0.5}]}\n"
      << "Les valeurs sont dans l'unite reelle du parametre (voir les plages). Pour les parametres a choix, donne l'index (0 = premier choix).\n"
      << "\"mods\" : sources possibles = " << modSrcNames.joinIntoString (", ") << " ; dst = un id de parametre de la liste, amount de -1 a 1 (12 assignations max).\n"
      << "Ne mets que les parametres qui changent (les autres gardent leur valeur par defaut). Mode : 0 Poly, 1 Mono, 2 Acid, 3 Drums, 4 Sample.\n\n"
      << "PARAMETRES (id | nom | plage ou choix | defaut) :\n";

    for (auto* p : allParams)
    {
        const juce::String id = p->paramID;
        if (id.startsWith ("morph") || id.startsWith ("slice") || id.startsWith ("sample") || id.startsWith ("step") || id.startsWith ("pm")) continue;
        if (id.length() > 2 && id[0] == 'm' && juce::CharacterFunctions::isDigit (id[1])) continue;

        t << id << " | " << p->getName (40) << " | ";
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (p))
        {
            for (int i = 0; i < pc->choices.size(); ++i) t << i << "=" << pc->choices[i] << (i + 1 < pc->choices.size() ? ", " : "");
            t << " | " << pc->getIndex() << "\n";
        }
        else
        {
            const auto r = p->getNormalisableRange();
            t << juce::String (r.start, 3) << " a " << juce::String (r.end, 3) << " | "
              << juce::String (p->convertFrom0to1 (p->getDefaultValue()), 3) << "\n";
        }
    }
    return t;
}

juce::StringArray DaftSynthProcessor::fmAlgoNames()
{
    return { "Single Op", "2-Op FM (2>1)", "Stack 4>3>2>1", "Two Stacks (2>1, 4>3)", "Three Pairs (parallel)",
             "Feedback Bell", "Dual Mod Tree", "Ring Loop 1>2>3>1" };
}

void DaftSynthProcessor::applyFmAlgo (int index)
{
    struct Link { int r, c; float a; };
    struct Algo { float ratio[6]; float out[6]; std::vector<Link> links; };
    static const std::vector<Algo> algos = {
        { { 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 0, 0 }, {} },
        { { 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 0, 0 }, { { 1, 2, 0.6f } } },
        { { 1, 1, 1, 1, 1, 1 }, { 1, 0, 0, 0, 0, 0 }, { { 1, 2, 0.8f }, { 2, 3, 0.8f }, { 3, 4, 0.8f } } },
        { { 1, 1, 1, 2, 1, 1 }, { 1, 0, 1, 0, 0, 0 }, { { 1, 2, 0.6f }, { 3, 4, 0.6f } } },
        { { 1, 2, 1, 3, 1, 4 }, { 0.7f, 0, 0.7f, 0, 0.7f, 0 }, { { 1, 2, 0.5f }, { 3, 4, 0.5f }, { 5, 6, 0.5f } } },
        { { 1, 3.5f, 1, 7, 1, 1 }, { 1, 0, 0.5f, 0, 0, 0 }, { { 1, 2, 0.7f }, { 2, 2, 0.5f }, { 3, 4, 0.4f } } },
        { { 1, 1, 2, 3, 0.5f, 1 }, { 1, 0, 0, 0, 0, 0 }, { { 1, 2, 0.6f }, { 1, 3, 0.5f }, { 2, 4, 0.5f }, { 3, 5, 0.5f } } },
        { { 1, 1, 1, 1, 1, 1 }, { 0.7f, 0.5f, 0.4f, 0, 0, 0 }, { { 1, 2, 0.5f }, { 2, 3, 0.5f }, { 3, 1, 0.5f } } },
    };
    if (index < 0 || index >= (int) algos.size()) return;
    const auto& a = algos[(size_t) index];

    auto setV = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    for (int i = 0; i < 6; ++i)
    {
        const juce::String n (i + 1);
        setV ("fmR" + n, a.ratio[i]);
        setV ("fmO" + n, a.out[i]);
        setV ("fmE" + n, 0.0f);
        for (int j = 0; j < 6; ++j) setV ("fmM" + n + juce::String (j + 1), 0.0f);
    }
    for (const auto& l : a.links) setV ("fmM" + juce::String (l.r) + juce::String (l.c), l.a);
    setV ("fm6Level", 1.0f);
}

void DaftSynthProcessor::randomizeSound (bool mutate)
{
    static const char* floats[] = { "saw", "pulse", "pw", "sub", "osc2", "osc2Semi", "osc2Detune", "noise", "cutoff", "res", "drive",
                                    "fenv", "keytrack", "hpf", "aA", "aD", "aS", "aR", "fA", "fD", "fS", "fR",
                                    "fmLevel", "fmAmt", "fmRatio", "fmFb", "fmEnv", "drift", "ringMix", "wtLevel", "wtPos", "uniVoices", "uniDetune", "dist", "distBias", "distTone",
                                    "wowFreq", "wowRes", "wowVowel", "wowDrive", "wowLfoDepth", "wowLfoRate", "wowMix" };
    static const char* choices[] = { "wtTable", "distType", "wowType", "wowLfoShape", "wowLfoSync" };

    auto setNorm = [this] (const char* id, float n)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n));
    };

    for (auto* id : floats)
    {
        auto* p = apvts.getParameter (id);
        if (p == nullptr) continue;
        const float cur = p->getValue();

        if (mutate)
        {
            setNorm (id, cur + (rnd.nextFloat() - 0.5f) * 0.3f);
        }
        else
        {
            float v = rnd.nextFloat();
            // quelques réglages de bon sens pour éviter les sons muets ou inutilisables
            if (juce::String (id) == "saw") v = 0.4f + 0.6f * v;
            else if (juce::String (id) == "aS") v = 0.3f + 0.7f * v;
            else if (juce::String (id) == "aA") v *= 0.25f;
            else if (juce::String (id) == "cutoff") v = 0.35f + 0.65f * v;
            else if (juce::String (id) == "res") v *= 0.8f;
            else if (juce::String (id) == "fmLevel" || juce::String (id) == "wtLevel" || juce::String (id) == "wowMix" || juce::String (id) == "dist" || juce::String (id) == "noise" || juce::String (id) == "pulse")
                v = rnd.nextFloat() < 0.5f ? 0.0f : v;
            setNorm (id, v);
        }
    }

    if (! mutate)
        for (auto* id : choices)
            if (auto* p = apvts.getParameter (id))
                setNorm (id, rnd.nextFloat());
}

const juce::String DaftSynthProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    return presets[(size_t) index].name;
}

void DaftSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);

    if (auto d = sampler.getData())
    {
        state.setProperty ("samplePath", samplePath, nullptr);
        juce::StringArray mk, sl;
        for (int m : d->markers) mk.add (juce::String (m));
        for (int i = 0; i < d->numSlices(); ++i)
        {
            const auto& s = sampler.slice[i];
            sl.add (juce::String (s.pitch.load()) + "," + juce::String (s.fine.load()) + "," + juce::String (s.level.load()) + ","
                    + juce::String (s.pan.load()) + "," + juce::String (s.drive.load()) + "," + juce::String (s.crush.load()) + ","
                    + juce::String (s.reverse.load() ? 1 : 0));
        }
        state.setProperty ("sampleMarkers", mk.joinIntoString (";"), nullptr);
        state.setProperty ("sampleSlices", sl.joinIntoString (";"), nullptr);
    }

    for (int k = 0; k < 4; ++k)
        if (snapValid[k])
        {
            juce::StringArray sv;
            for (float f : snap[k]) sv.add (juce::String (f, 5));
            state.setProperty ("morphSnap" + juce::String (k), sv.joinIntoString (","), nullptr);
        }

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DaftSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) apvts.state.getProperty ("program", 0));

            for (int k = 0; k < 4; ++k)
            {
                snapValid[k] = false;
                const juce::String s = apvts.state.getProperty ("morphSnap" + juce::String (k), "").toString();
                if (s.isNotEmpty())
                {
                    juce::StringArray tok;
                    tok.addTokens (s, ",", "");
                    if ((size_t) tok.size() == allParams.size())
                    {
                        snap[k].assign (allParams.size(), 0.0f);
                        for (int i = 0; i < tok.size(); ++i) snap[k][(size_t) i] = tok[i].getFloatValue();
                        snapValid[k] = true;
                    }
                }
            }

            // restauration du sample (si le fichier existe toujours)
            const juce::String path = apvts.state.getProperty ("samplePath", "").toString();
            if (path.isNotEmpty() && juce::File (path).existsAsFile() && loadSampleFile (juce::File (path)))
            {
                juce::StringArray mk, sl;
                mk.addTokens (apvts.state.getProperty ("sampleMarkers", "").toString(), ";", "");
                sl.addTokens (apvts.state.getProperty ("sampleSlices", "").toString(), ";", "");
                std::vector<int> markers;
                for (auto& m : mk) if (m.isNotEmpty()) markers.push_back (m.getIntValue());
                if (! markers.empty()) sampler.setMarkers (markers);
                for (int i = 0; i < sl.size() && i < dl::SampleEngine::kMaxSlices; ++i)
                {
                    juce::StringArray f;
                    f.addTokens (sl[i], ",", "");
                    if (f.size() >= 7)
                    {
                        auto& s = sampler.slice[i];
                        s.pitch = f[0].getFloatValue(); s.fine = f[1].getFloatValue(); s.level = f[2].getFloatValue();
                        s.pan = f[3].getFloatValue(); s.drive = f[4].getFloatValue(); s.crush = f[5].getFloatValue();
                        s.reverse = f[6].getIntValue() != 0;
                    }
                }
            }
        }
}

//==============================================================================
// Audio
//==============================================================================
void DaftSynthProcessor::readSynthParams (dl::SynthParams& p) const
{
    p.mode = juce::jlimit (0, 3, (int) pv ("mode"));
    p.octave = pv ("octave");
    p.saw = pv ("saw");
    p.pulse = pv ("pulse");
    p.pw = pv ("pw");
    p.sub = pv ("sub");
    p.noise = pv ("noise");
    p.osc2 = pv ("osc2");
    p.osc2Semi = pv ("osc2Semi");
    p.osc2Detune = pv ("osc2Detune");
    p.drift = pv ("drift");
    p.ringMix = pv ("ringMix");
    p.ringRatio = pv ("ringRatio");
    p.sync2 = ((int) pv ("sync2")) == 1;
    p.fmLevel = pv ("fmLevel");
    p.fmAmt = pv ("fmAmt");
    p.fmRatio = pv ("fmRatio");
    p.fmFb = pv ("fmFb");
    p.fmEnv = pv ("fmEnv");
    p.fm6Level = pv ("fm6Level");
    p.fm6Index = pv ("fm6Index");
    for (int i = 0; i < 6; ++i)
    {
        p.fm6Ratio[i] = effVals[(size_t) fmRIdx[i]];
        p.fm6Out[i] = effVals[(size_t) fmOIdx[i]];
        p.fm6Env[i] = effVals[(size_t) fmEIdx[i]];
        for (int j = 0; j < 6; ++j) p.fm6Mat[i][j] = effVals[(size_t) fmMIdx[i][j]];
    }
    p.wtTable = juce::jlimit (0, 6, (int) pv ("wtTable"));
    p.wtLevel = pv ("wtLevel");
    p.wtPos = pv ("wtPos");
    p.wtOct = pv ("wtOct");
    p.uniVoices = juce::jlimit (1, 7, juce::roundToInt (pv ("uniVoices")));
    p.uniDetune = pv ("uniDetune");

    p.cutoff = pv ("cutoff");
    p.res = pv ("res");
    p.drive = pv ("drive");
    p.fenv = pv ("fenv");
    p.keytrack = pv ("keytrack");

    const float h = pv ("hpf");
    p.hpOn = h > 0.01f;
    const float hz = 20.0f * std::pow (75.0f, h);
    p.hpCoef = std::exp (-dl::kTwoPi * hz / (float) sampleRate);

    p.aA = pv ("aA"); p.aD = pv ("aD"); p.aS = pv ("aS"); p.aR = pv ("aR");
    p.fA = pv ("fA"); p.fD = pv ("fD"); p.fS = pv ("fS"); p.fR = pv ("fR");

    p.lfoShape = (int) pv ("lfoShape");
    const float cpb = dl::syncCyclesPerBeat ((int) pv ("lfoSync"));
    p.lfoRate = cpb > 0.0f ? (float) (curBpm / 60.0 * (double) cpb) : pv ("lfoRate");
    p.lfoPitch = pv ("lfoPitch");
    p.lfoCutoff = pv ("lfoCutoff");
    p.lfoPwm = pv ("lfoPwm");
    p.glide = pv ("glide");
    p.accent = pv ("accent");
}

void DaftSynthProcessor::triggerDrum (int note, float vel)
{
    const int type = dl::drumTypeForNote (note);
    if (type < 0) return;

    // Un charley fermé coupe le charley ouvert
    if (type == dl::HatC)
        for (auto& d : drums)
            if (d.active && d.type == dl::HatO) d.active = false;

    int idx = -1;
    for (int i = 0; i < 16; ++i)
        if (! drums[i].active) { idx = i; break; }

    if (idx < 0)
    {
        uint32_t oldest = UINT32_MAX;
        for (int i = 0; i < 16; ++i)
            if (drums[i].age < oldest) { oldest = drums[i].age; idx = i; }
    }

    // Les toms ont 2 notes chacun : la note la plus haute accorde légèrement plus aigu
    float extraTune = 0.0f;
    if (note == 43 || note == 47 || note == 50) extraTune = 2.0f;

    drums[idx].trigger (type, vel, kitIdx, drumTune + extraTune, drumDecay, drumTone, (float) sampleRate, ++drumCounter);
}

void DaftSynthProcessor::renderDrums (float* out, int n)
{
    for (auto& d : drums)
    {
        if (! d.active) continue;
        for (int i = 0; i < n; ++i)
        {
            out[i] += d.render() * drumLevel;
            if (! d.active) break;
        }
    }
}

void DaftSynthProcessor::handleMidi (const juce::MidiMessage& m, int mode)
{
    if (m.isNoteOn())
    {
        const float vel = m.getFloatVelocity();
        if (arpActive) { arp.noteOn (m.getNoteNumber(), vel); return; }
        modEngine.noteOn (vel);
        if (mode == 3) triggerDrum (m.getNoteNumber(), vel);
        else if (mode == 4) sampler.noteOn (m.getNoteNumber(), vel);
        else synth.noteOn (m.getNoteNumber(), vel);
    }
    else if (m.isNoteOff())
    {
        if (arpActive) { arp.noteOff (m.getNoteNumber()); return; }
        if (mode == 4)
        {
            sampler.noteOff (m.getNoteNumber());
            modEngine.noteOff();
        }
        else if (mode != 3)
        {
            synth.noteOff (m.getNoteNumber());
            modEngine.noteOff();
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        synth.allNotesOff();
        sampler.allOff();
        modEngine.allOff();
        arp.reset();
    }
    else if (m.isPitchWheel())
    {
        synth.setBend (((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f);
    }
    else if (m.isController())
    {
        if (m.getControllerNumber() == 1)  // molette de modulation
        {
            modWheel = (float) m.getControllerValue() / 127.0f;
            synth.setMod (modWheel * 0.5f);
            modEngine.setWheel (modWheel);
        }
        else if (m.getControllerNumber() == 64)  // pédale de sustain
        {
            synth.setSustain (m.getControllerValue() >= 64);
        }
    }
}

// Traite un petit segment audio (<= 32 échantillons) avec des valeurs de modulation constantes
void DaftSynthProcessor::processSegment (int start, int len, int mode, double ppqStart, bool havePpq, double bpm, double beatsPerSample)
{
    const float sr = (float) sampleRate;
    const double ppq = ppqStart + (double) start * beatsPerSample;

    // ---- Sources de modulation
    // ---- Arpégiateur : génère les notes du synthé (avant le rendu audio du segment)
    if (arpActive)
    {
        dl::Arpeggiator::Cfg ac;
        ac.on = true;
        ac.mode = (int) pv ("arpMode");
        static const float rateCpb[6] = { 1.0f, 2.0f, 4.0f, 8.0f, 3.0f, 6.0f };
        ac.stepBeats = 1.0f / rateCpb[juce::jlimit (0, 5, (int) pv ("arpRate"))];
        ac.octaves = juce::roundToInt (pv ("arpOct"));
        ac.gate = pv ("arpGate");
        ac.swing = pv ("arpSwing");
        ac.hold = ((int) pv ("arpHold")) == 1;
        arp.setCfg (ac);

        const double beatNow = havePpq ? ppq : arpFreeBeat;
        const double beatEnd = beatNow + (double) len * beatsPerSample;
        arp.process (beatNow, beatEnd,
                     [this] (int n, float v) { synth.noteOn (n, v); modEngine.noteOn (v); arpLastNote.store (n); },
                     [this] (int n) { synth.noteOff (n); modEngine.noteOff(); });
        if (! havePpq) arpFreeBeat = beatEnd;
    }

    // ---- Sources de modulation
    modEngine.update (len, ppq, havePpq, bpm, lfoCfg, envCfg, macroVals, stepCfg, modSrcValues);

    // ---- Valeurs effectives = valeur du potard + modulation (dans le domaine normalisé 0..1)
    effVals = baseVals;
    touched.clear();
    for (int s = 0; s < numSlots; ++s)
    {
        const int idx = slots[s].dstIdx;
        if (! modFlag[(size_t) idx]) { modFlag[(size_t) idx] = 1; touched.push_back (idx); }
        modNorm[(size_t) idx] += slots[s].amt * modSrcValues[slots[s].src];
    }
    for (const auto& mm : macroMods)      // macros intelligentes : décalages dans le domaine normalisé
    {
        if (! modFlag[(size_t) mm.idx]) { modFlag[(size_t) mm.idx] = 1; touched.push_back (mm.idx); }
        modNorm[(size_t) mm.idx] += mm.off;
    }
    for (int idx : touched)
    {
        auto* p = allParams[(size_t) idx];
        const float norm = p->convertTo0to1 (baseVals[(size_t) idx]);
        effVals[(size_t) idx] = p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, norm + modNorm[(size_t) idx]));
        modNorm[(size_t) idx] = 0.0f;
        modFlag[(size_t) idx] = 0;
    }

    // ---- Synthé / drums
    dl::SynthParams sp;
    readSynthParams (sp);
    synth.setParams (sp);

    kitIdx = (int) pv ("kit");
    drumTune = pv ("drumTune");
    drumDecay = pv ("drumDecay");
    drumTone = pv ("drumTone");
    drumLevel = pv ("drumLevel");

    if (mode == 3) renderDrums (mono.data() + start, len);
    else synth.renderMono (mono.data() + start, len);

    // ---- Sampler (stéréo)
    float smpL[32] = {}, smpR[32] = {};
    {
        dl::SampleEngine::Params sp2;
        sp2.level = pv ("sampleLevel");
        sp2.tune = pv ("sampleTune");
        sp2.attack = pv ("sampleAttack");
        sp2.release = pv ("sampleRelease");
        sp2.chromatic = ((int) pv ("sampleMode")) == 1;
        sp2.gate = ((int) pv ("sampleGate")) == 1;
        sp2.loop = ((int) pv ("sampleLoop")) == 1;
        sp2.root = juce::roundToInt (pv ("sampleRoot"));
        sp2.baseNote = juce::roundToInt (pv ("sliceNote"));
        sampler.setParams (sp2);
        sampler.render (smpL, smpR, len);
    }

    // ---- Effets
    const float volume = pv ("volume");

    dl::DistParams dp;
    dp.type = (int) pv ("distType");
    dp.amount = pv ("dist");
    dp.bias = pv ("distBias");
    dp.tone = pv ("distTone");
    dp.mix = pv ("distMix");
    dp.outDb = pv ("distOut");

    dl::WowParams wp;
    wp.type = (int) pv ("wowType");
    wp.freq = pv ("wowFreq");
    wp.res = pv ("wowRes");
    wp.vowel = pv ("wowVowel");
    wp.drive = pv ("wowDrive");
    wp.mix = pv ("wowMix");
    wp.depth = pv ("wowLfoDepth");
    const bool wowPre = ((int) pv ("wowPos")) == 0;
    const int wowShape = (int) pv ("wowLfoShape");
    const float wCpb = dl::syncCyclesPerBeat ((int) pv ("wowLfoSync"));
    const float wowRateHz = wCpb > 0.0f ? (float) (bpm / 60.0 * (double) wCpb) : pv ("wowLfoRate");

    const float crushBits = pv ("crushBits");
    const float crushRate = pv ("crushRate");
    const int chorusMode = (int) pv ("chorus");

    const float phMix = pv ("phaserMix"), phDepth = pv ("phaserDepth"), phFb = pv ("phaserFb");
    const float phCpb = dl::syncCyclesPerBeat ((int) pv ("phaserSync"));
    const float phRate = phCpb > 0.0f ? (float) (bpm / 60.0 * (double) phCpb) : pv ("phaserRate");

    const float dMix = pv ("delayMix"), dFb = pv ("delayFb");
    const float dCpb = dl::syncCyclesPerBeat ((int) pv ("delaySync"));
    const float dTime = dCpb > 0.0f ? (float) (60000.0 / (bpm * (double) dCpb)) : pv ("delayTime");

    const float chorusMix = pv ("chorusMix");
    const int phStages = 4 + 2 * juce::jlimit (0, 2, (int) pv ("phaserStages"));
    const float flMix = pv ("flMix"), flRate = pv ("flRate"), flDepth = pv ("flDepth"), flFb = pv ("flFb"), flDelay = pv ("flDelay");
    const bool ping = ((int) pv ("delayPing")) == 1;
    const float dTone = pv ("delayTone");

    dl::EqParams ep;
    ep.lowG = pv ("eqLowGain"); ep.lowF = pv ("eqLowFreq"); ep.midG = pv ("eqMidGain"); ep.midF = pv ("eqMidFreq");
    ep.midQ = pv ("eqMidQ"); ep.highG = pv ("eqHighGain"); ep.highF = pv ("eqHighFreq");
    eq.set (ep, sr);

    dl::OttParams op;
    op.mix = pv ("ottMix"); op.depth = pv ("ottDepth"); op.time = pv ("ottTime");
    op.lowDb = pv ("ottLow"); op.midDb = pv ("ottMid"); op.highDb = pv ("ottHigh");
    if (op.mix > 0.001f) ott.setCrossovers (140.0f, 2500.0f);

    const float widthAmt = pv ("width");
    const float boostDb = pv ("boost"), limCeil = pv ("limCeil"), limRel = pv ("limRel");

    const float revMix = pv ("reverbMix"), revSize = pv ("reverbSize");
    const float revPreMs = pv ("reverbPre"), revDamp = pv ("reverbDamp"), revWidth = pv ("reverbWidth");
    const float pumpAmt = pv ("pump"), pumpRel = pv ("pumpRel");

    static const double pumpMult[4] = { 0.5, 1.0, 2.0, 4.0 };
    const double mult = pumpMult[juce::jlimit (0, 3, (int) pv ("pumpRate"))];

    float* Lout = outL + start;
    float* Rout = outR + start;

    for (int i = 0; i < len; ++i)
    {
        float l = (mono[(size_t) (start + i)] + smpL[i]) * volume;
        float r = (mono[(size_t) (start + i)] + smpR[i]) * volume;

        // wobble du module WOW (une valeur par échantillon)
        if (wp.mix > 0.001f && wp.depth > 0.001f)
        {
            wowPhase += wowRateHz / sr;
            if (wowPhase >= 1.0f) { wowPhase -= 1.0f; wowRand = rnd.nextFloat() * 2.0f - 1.0f; }
            switch (wowShape)
            {
                case 0: wp.lfo = std::sin (dl::kTwoPi * wowPhase); break;
                case 1: wp.lfo = 1.0f - 4.0f * std::fabs (wowPhase - 0.5f); break;
                case 2: wp.lfo = 2.0f * wowPhase - 1.0f; break;
                default: wp.lfo = wowPhase < 0.5f ? 1.0f : -1.0f; break;
            }
        }
        else wp.lfo = 0.0f;

        if (wowPre) wow.process (l, r, wp);
        distortion.process (l, r, dp, sr);
        if (! wowPre) wow.process (l, r, wp);
        crusher.process (l, r, crushBits, crushRate, sr);
        flanger.process (l, r, flMix, flRate, flDepth, flFb, flDelay);
        chorus.process (l, r, chorusMode, chorusMix);
        phaser.process (l, r, phMix, phRate, phDepth, phFb, phStages);
        eq.process (l, r);
        ott.process (l, r, op);
        delay.process (l, r, dMix, dTime, dFb, ping, dTone);

        if (pumpAmt > 0.001f && havePpq)
        {
            const double beats = (ppq + (double) i * beatsPerSample) * mult;
            const float phase = (float) (beats - std::floor (beats));
            const float x = std::min (1.0f, phase / pumpRel);
            const float g = 1.0f - pumpAmt * (1.0f - x) * (1.0f - x);
            l *= g;
            r *= g;
        }

        Lout[i] = l;
        Rout[i] = r;
    }

    if (revMix > 0.001f)
    {
        float tl[32], tr[32];
        const float pre = std::clamp (revPreMs * (float) sampleRate * 0.001f, 1.0f, (float) sampleRate * 0.2f);
        for (int i = 0; i < len; ++i)
        {
            tl[i] = revPre[0].read (pre);
            tr[i] = revPre[1].read (pre);
            revPre[0].write (Lout[i]);
            revPre[1].write (Rout[i]);
        }

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.65f * revSize;
        rp.damping = revDamp;
        rp.wetLevel = revMix * 0.5f;
        rp.dryLevel = 0.0f;
        rp.width = revWidth;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (tl, tr, len);

        for (int i = 0; i < len; ++i)
        {
            Lout[i] = Lout[i] * (1.0f - 0.3f * revMix) + tl[i];
            Rout[i] = Rout[i] * (1.0f - 0.3f * revMix) + tr[i];
        }
    }
    else
    {
        for (int i = 0; i < len; ++i)
        {
            revPre[0].write (Lout[i]);
            revPre[1].write (Rout[i]);
        }
    }

    // Largeur stéréo (mid / side) puis boost + limiteur de sortie
    if (std::fabs (widthAmt - 1.0f) > 0.001f)
        for (int i = 0; i < len; ++i)
        {
            const float mid = 0.5f * (Lout[i] + Rout[i]), side = 0.5f * (Lout[i] - Rout[i]) * widthAmt;
            Lout[i] = mid + side;
            Rout[i] = mid - side;
        }

    for (int i = 0; i < len; ++i)
        limiter.process (Lout[i], Rout[i], boostDb, limCeil, limRel);
}

void DaftSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    buffer.clear();

    if ((int) mono.size() < n) mono.assign ((size_t) n, 0.0f);
    std::fill (mono.begin(), mono.begin() + n, 0.0f);

    // ---- Valeurs de base des potards
    for (size_t i = 0; i < rawPtrs.size(); ++i) baseVals[i] = rawPtrs[i]->load();
    applyMorphToBase();
    computeMacroMods();
    effVals = baseVals;

    const int mode = juce::jlimit (0, 4, (int) bv ("mode"));
    if (mode != lastMode)
    {
        synth.allNotesOff();
        for (auto& d : drums) d.active = false;
        modEngine.allOff();
        arp.reset();
        lastMode = mode;
    }

    // ---- Position hôte (tempo, synchro des LFO / delay / pump)
    double ppqStart = 0.0, bpm = 120.0;
    bool havePpq = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto ppq = pos->getPpqPosition())
            {
                ppqStart = *ppq;
                havePpq = pos->getIsPlaying();
            }
            if (auto b = pos->getBpm()) bpm = *b;
        }
    }
    curBpm = bpm;
    const double beatsPerSample = bpm / (60.0 * sampleRate);

    // ---- Configuration de la modulation (lue une fois par bloc)
    for (int i = 0; i < 4; ++i)
    {
        const std::string p = "lfo" + std::to_string (i + 1);
        lfoCfg[i].shape = (int) bv (p + "Shape");
        lfoCfg[i].rate = bv (p + "Rate");
        lfoCfg[i].sync = (int) bv (p + "Sync");
        lfoCfg[i].retrig = ((int) bv (p + "Retrig")) == 1;
    }
    modEngine.setRetrig (lfoCfg);

    for (int i = 0; i < 2; ++i)
    {
        const std::string p = "env" + std::to_string (i + 1);
        envCfg[i].a = bv (p + "A");
        envCfg[i].d = bv (p + "D");
        envCfg[i].s = bv (p + "S");
        envCfg[i].r = bv (p + "R");
    }
    for (int i = 0; i < 4; ++i) macroVals[i] = bv ("macro" + std::to_string (i + 1));

    stepCfg.len = juce::roundToInt (bv ("stepLen"));
    stepCfg.sync = (int) bv ("stepSync");
    stepCfg.rate = bv ("stepRate");
    stepCfg.glide = bv ("stepGlide");
    for (int i = 0; i < 16; ++i) stepCfg.steps[i] = bv ("step" + std::to_string (i + 1));

    // arpégiateur actif seulement pour les modes synthé (Poly / Mono / Acid)
    const bool wantArp = ((int) bv ("arpOn")) == 1 && mode <= 2;
    if (arpActive && ! wantArp) { releaseArp(); arp.reset(); }
    arpActive = wantArp;

    numSlots = 0;
    for (int i = 0; i < kNumSlots; ++i)
    {
        const int src = (int) baseVals[(size_t) slotSrcIdx[i]];
        const int dst = (int) baseVals[(size_t) slotDstIdx[i]];
        const float amt = baseVals[(size_t) slotAmtIdx[i]];
        if (src > 0 && dst > 0 && dst <= (int) destParamIdx.size() && std::fabs (amt) > 1.0e-4f)
            slots[numSlots++] = { src, destParamIdx[(size_t) (dst - 1)], amt };
    }

    // Le mode du synthé doit être connu avant les événements MIDI du début de bloc
    {
        dl::SynthParams sp0;
        readSynthParams (sp0);
        synth.setParams (sp0);
    }

    // ---- Traitement par segments, découpés aux événements MIDI (précision à l'échantillon)
    outL = buffer.getWritePointer (0);
    outR = buffer.getWritePointer (1);

    constexpr int kChunk = 32;
    auto it = midi.begin();
    const auto endIt = midi.end();
    int pos = 0;

    while (pos < n)
    {
        while (it != endIt && (*it).samplePosition <= pos)
        {
            handleMidi ((*it).getMessage(), mode);
            ++it;
        }

        int next = std::min (n, pos + kChunk);
        if (it != endIt) next = std::min (next, std::max ((*it).samplePosition, pos + 1));

        processSegment (pos, next - pos, mode, ppqStart, havePpq, bpm, beatsPerSample);
        pos = next;
    }

    // ---- Enregistrement interne (resampling)
    if (recState.load() == 1)
    {
        int pos = recPos.load();
        const int cap = recBuf.getNumSamples();
        for (int i = 0; i < n && pos < cap; ++i, ++pos)
        {
            recBuf.setSample (0, pos, outL[i]);
            recBuf.setSample (1, pos, outR[i]);
        }
        recPos.store (pos);
        if (pos >= cap) recState.store (2);
    }

    // ---- Données pour l'interface animée
    {
        float sumSq = 0.0f, pkL = 0.0f, pkR = 0.0f;
        const int w = scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (outL[i] + outR[i]);
            scopeBuf[(size_t) ((w + i) & (kScopeSize - 1))] = m;
            sumSq += m * m;
            pkL = std::max (pkL, std::fabs (outL[i]));
            pkR = std::max (pkR, std::fabs (outR[i]));
        }
        scopeWrite.store ((w + n) & (kScopeSize - 1), std::memory_order_release);
        rmsOut.store (std::sqrt (sumSq / (float) std::max (n, 1)));
        peakL.store (pkL);
        peakR.store (pkR);
        blockCounter.fetch_add (1);
    }
}

bool DaftSynthProcessor::loadSampleFile (const juce::File& f)
{
    const float sens = baseVals.empty() ? 0.5f : apvts.getRawParameterValue ("sampleSens")->load();
    if (! sampler.loadFile (f, formats, sens)) return false;
    samplePath = f.getFullPathName();
    return true;
}

void DaftSynthProcessor::startRecording()
{
    if (recState.load() == 1) return;
    recBuf.setSize (2, (int) (sampleRate * 60.0));   // 60 s max, alloué ici (thread interface), pas dans le thread audio
    recBuf.clear();
    recPos.store (0);
    recState.store (1);
}

void DaftSynthProcessor::stopRecording()
{
    if (recState.load() == 1) recState.store (2);
    recStopTicks = 0;
}

// Appelée par l'interface : quand l'enregistrement est terminé, le transforme en sample découpé
bool DaftSynthProcessor::pollRecording()
{
    if (recState.load() != 2) return false;
    if (++recStopTicks < 4) return false;          // laisse finir le bloc audio en cours
    const int len = recPos.load();
    recState.store (0);
    if (len < 2048) return false;

    juce::AudioBuffer<float> copy (2, len);
    for (int ch = 0; ch < 2; ++ch) copy.copyFrom (ch, 0, recBuf, ch, 0, len);
    sampler.loadBuffer (std::move (copy), sampleRate, "Resample", apvts.getRawParameterValue ("sampleSens")->load());
    samplePath.clear();
    return true;
}

juce::AudioProcessorEditor* DaftSynthProcessor::createEditor()
{
    return new DaftSynthEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DaftSynthProcessor();
}
