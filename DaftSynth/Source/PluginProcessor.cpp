#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Définition des paramètres
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DaftSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&layout] (const char* id, const char* name, float lo, float hi, float def,
                           float centre = 0.0f, const char* unit = "", float interval = 0.0f)
    {
        juce::NormalisableRange<float> range (lo, hi, interval);
        if (centre > 0.0f) range.setSkewForCentre (centre);

        const juce::String u (unit);
        const int decimals = (hi - lo) < 10.0f ? 2 : ((hi - lo) < 100.0f ? 1 : 0);
        auto attrs = juce::AudioParameterFloatAttributes()
                         .withStringFromValueFunction ([u, decimals] (float v, int) { return juce::String (v, decimals) + u; });

        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def, attrs));
    };

    auto addC = [&layout] (const char* id, const char* name, juce::StringArray choices, int def)
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
    addF ("lfoRate", "LFO Rate", 0.05f, 30, 4, 3, " Hz");
    addF ("lfoPitch", "LFO > Pitch", 0, 1, 0);
    addF ("lfoCutoff", "LFO > Cutoff", 0, 1, 0);
    addF ("lfoPwm", "LFO > PWM", 0, 1, 0);

    // Jeu
    addF ("glide", "Glide", 0, 2, 0, 0.2f, " s");
    addF ("accent", "Acid Accent", 0, 1, 0.6f);
    addF ("volume", "Master Volume", 0, 1, 0.7f);

    // Effets
    addF ("dist", "Distortion", 0, 1, 0);
    addF ("crushBits", "Crush Bits", 4, 16, 16, 0, " bit", 1);
    addF ("crushRate", "Crush Rate", 2000, 48000, 48000, 12000, " Hz");
    addC ("chorus", "Chorus", { "Off", "I", "II", "I+II" }, 0);
    addF ("phaserMix", "Phaser Mix", 0, 1, 0);
    addF ("phaserRate", "Phaser Rate", 0.05f, 8, 0.4f, 0.5f, " Hz");
    addF ("phaserDepth", "Phaser Depth", 0, 1, 0.7f);
    addF ("phaserFb", "Phaser Feedback", 0, 0.95f, 0.5f);
    addF ("delayMix", "Delay Mix", 0, 1, 0);
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

    return layout;
}

//==============================================================================
DaftSynthProcessor::DaftSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DAFTSYNTH", createLayout())
{
}

bool DaftSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DaftSynthProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    synth.prepare (sr);
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
    p.lfoRate = pv ("lfoRate");
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
        if (mode == 3) triggerDrum (m.getNoteNumber(), vel);
        else synth.noteOn (m.getNoteNumber(), vel);
    }
    else if (m.isNoteOff())
    {
        if (mode != 3) synth.noteOff (m.getNoteNumber());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        synth.allNotesOff();
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
        }
        else if (m.getControllerNumber() == 64)  // pédale de sustain
        {
            synth.setSustain (m.getControllerValue() >= 64);
        }
    }
}

void DaftSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const float sr = (float) sampleRate;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    buffer.clear();

    if ((int) mono.size() < n) mono.assign ((size_t) n, 0.0f);
    std::fill (mono.begin(), mono.begin() + n, 0.0f);

    // ---- Paramètres
    dl::SynthParams sp;
    readSynthParams (sp);
    synth.setParams (sp);

    if (sp.mode != lastMode)
    {
        synth.allNotesOff();
        for (auto& d : drums) d.active = false;
        lastMode = sp.mode;
    }

    kitIdx = (int) pv ("kit");
    drumTune = pv ("drumTune");
    drumDecay = pv ("drumDecay");
    drumTone = pv ("drumTone");
    drumLevel = pv ("drumLevel");

    // ---- Position hôte (pour le pump synchronisé au tempo)
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

    // ---- Génération du son, MIDI avec précision à l'échantillon
    auto renderRange = [&] (int a, int b)
    {
        if (b <= a) return;
        if (sp.mode == 3) renderDrums (mono.data() + a, b - a);
        else synth.renderMono (mono.data() + a, b - a);
    };

    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (pos, n, meta.samplePosition);
        renderRange (pos, evPos);
        handleMidi (meta.getMessage(), sp.mode);
        pos = evPos;
    }
    renderRange (pos, n);

    // ---- Effets
    const float volume = pv ("volume");
    const float dist = pv ("dist");
    const float crushBits = pv ("crushBits");
    const float crushRate = pv ("crushRate");
    const int chorusMode = (int) pv ("chorus");
    const float phMix = pv ("phaserMix"), phRate = pv ("phaserRate"), phDepth = pv ("phaserDepth"), phFb = pv ("phaserFb");
    const float dMix = pv ("delayMix"), dTime = pv ("delayTime"), dFb = pv ("delayFb");
    const float revMix = pv ("reverbMix"), revSize = pv ("reverbSize");
    const float pumpAmt = pv ("pump"), pumpRel = pv ("pumpRel");

    static const double pumpMult[4] = { 0.5, 1.0, 2.0, 4.0 };
    const double mult = pumpMult[juce::jlimit (0, 3, (int) pv ("pumpRate"))];
    const double beatsPerSample = bpm / (60.0 * sampleRate);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        float l = mono[(size_t) i] * volume;
        float r = l;

        if (dist > 0.001f)
        {
            l = dl::distort (l, dist);
            r = l;
        }

        crusher.process (l, r, crushBits, crushRate, sr);
        chorus.process (l, r, chorusMode);
        phaser.process (l, r, phMix, phRate, phDepth, phFb);
        delay.process (l, r, dMix, dTime, dFb);

        if (pumpAmt > 0.001f && havePpq)
        {
            const double beats = (ppqStart + (double) i * beatsPerSample) * mult;
            const float phase = (float) (beats - std::floor (beats));
            const float x = std::min (1.0f, phase / pumpRel);
            const float g = 1.0f - pumpAmt * (1.0f - x) * (1.0f - x);
            l *= g;
            r *= g;
        }

        L[i] = l;
        R[i] = r;
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
        reverb.processStereo (L, R, n);
    }

    // Limiteur doux en sortie
    for (int i = 0; i < n; ++i)
    {
        L[i] = std::tanh (L[i]);
        R[i] = std::tanh (R[i]);
    }

    // Données pour l'interface animée
    {
        float sumSq = 0.0f, pkL = 0.0f, pkR = 0.0f;
        const int w = scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            scopeBuf[(size_t) ((w + i) & (kScopeSize - 1))] = m;
            sumSq += m * m;
            pkL = std::max (pkL, std::fabs (L[i]));
            pkR = std::max (pkR, std::fabs (R[i]));
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
