#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Définition des paramètres
//==============================================================================
juce::StringArray DaftSynthProcessor::modDestIds, DaftSynthProcessor::modDestNames;
juce::StringArray DaftSynthProcessor::modSrcNames { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Env 1", "Env 2",
                                                    "Velocity", "Mod Wheel", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Random" };

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

    addC ("mode", "Mode", { "Poly (Juno)", "Mono (Moog)", "Acid (303)", "Drums" }, 0);

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

    // Drums
    addC ("kit", "Drum Kit", { "TR-808", "TR-909", "LinnDrum", "Oberheim DMX" }, 0);
    addF ("drumTune", "Drum Tune", -12, 12, 0, 0, " st");
    addF ("drumDecay", "Drum Decay", 0.3f, 2, 1, 1, "x");
    addF ("drumTone", "Drum Tone", 0, 1, 0.5f);
    addF ("drumLevel", "Drum Level", 0, 1.5f, 0.9f);

    // ---------------- Modulation (non modulable elle-même)
    recordDest = false;

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
    for (auto& d : drums) d.active = false;

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
        modEngine.noteOn (vel);
        if (mode == 3) triggerDrum (m.getNoteNumber(), vel);
        else synth.noteOn (m.getNoteNumber(), vel);
    }
    else if (m.isNoteOff())
    {
        if (mode != 3)
        {
            synth.noteOff (m.getNoteNumber());
            modEngine.noteOff();
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        synth.allNotesOff();
        modEngine.allOff();
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
    modEngine.update (len, ppq, havePpq, bpm, lfoCfg, envCfg, macroVals, modSrcValues);

    // ---- Valeurs effectives = valeur du potard + modulation (dans le domaine normalisé 0..1)
    effVals = baseVals;
    touched.clear();
    for (int s = 0; s < numSlots; ++s)
    {
        const int idx = slots[s].dstIdx;
        if (! modFlag[(size_t) idx]) { modFlag[(size_t) idx] = 1; touched.push_back (idx); }
        modNorm[(size_t) idx] += slots[s].amt * modSrcValues[slots[s].src];
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

    // ---- Effets
    const float volume = pv ("volume");

    dl::DistParams dp;
    dp.type = (int) pv ("distType");
    dp.amount = pv ("dist");
    dp.bias = pv ("distBias");
    dp.tone = pv ("distTone");
    dp.mix = pv ("distMix");
    dp.outDb = pv ("distOut");

    const float crushBits = pv ("crushBits");
    const float crushRate = pv ("crushRate");
    const int chorusMode = (int) pv ("chorus");

    const float phMix = pv ("phaserMix"), phDepth = pv ("phaserDepth"), phFb = pv ("phaserFb");
    const float phCpb = dl::syncCyclesPerBeat ((int) pv ("phaserSync"));
    const float phRate = phCpb > 0.0f ? (float) (bpm / 60.0 * (double) phCpb) : pv ("phaserRate");

    const float dMix = pv ("delayMix"), dFb = pv ("delayFb");
    const float dCpb = dl::syncCyclesPerBeat ((int) pv ("delaySync"));
    const float dTime = dCpb > 0.0f ? (float) (60000.0 / (bpm * (double) dCpb)) : pv ("delayTime");

    const float revMix = pv ("reverbMix"), revSize = pv ("reverbSize");
    const float pumpAmt = pv ("pump"), pumpRel = pv ("pumpRel");

    static const double pumpMult[4] = { 0.5, 1.0, 2.0, 4.0 };
    const double mult = pumpMult[juce::jlimit (0, 3, (int) pv ("pumpRate"))];

    float* Lout = outL + start;
    float* Rout = outR + start;

    for (int i = 0; i < len; ++i)
    {
        float l = mono[(size_t) (start + i)] * volume;
        float r = l;

        distortion.process (l, r, dp, sr);
        crusher.process (l, r, crushBits, crushRate, sr);
        chorus.process (l, r, chorusMode);
        phaser.process (l, r, phMix, phRate, phDepth, phFb);
        delay.process (l, r, dMix, dTime, dFb);

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
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.65f * revSize;
        rp.damping = 0.4f;
        rp.wetLevel = revMix * 0.5f;
        rp.dryLevel = 1.0f - 0.3f * revMix;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (Lout, Rout, len);
    }

    // Limiteur doux en sortie
    for (int i = 0; i < len; ++i)
    {
        Lout[i] = std::tanh (Lout[i]);
        Rout[i] = std::tanh (Rout[i]);
    }
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
    effVals = baseVals;

    const int mode = juce::jlimit (0, 3, (int) bv ("mode"));
    if (mode != lastMode)
    {
        synth.allNotesOff();
        for (auto& d : drums) d.active = false;
        modEngine.allOff();
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

juce::AudioProcessorEditor* DaftSynthProcessor::createEditor()
{
    return new DaftSynthEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DaftSynthProcessor();
}
