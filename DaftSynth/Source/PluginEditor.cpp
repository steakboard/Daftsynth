#include "PluginEditor.h"


DaftSynthEditor::Ctl* DaftSynthEditor::addCtl (const juce::String& id, const juce::String& shortLabel, bool linear, bool vertical)
{
    auto c = std::make_unique<Ctl>();
    c->id = id;

    auto* param = proc.apvts.getParameter (id);
    jassert (param != nullptr);

    c->label.setText (shortLabel, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (theme::mono (11.0f));
    c->label.setColour (juce::Label::textColourId, theme::lightGray);
    addAndMakeVisible (c->label);

    if (dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr)
    {
        c->isChoice = true;
        auto* pc = dynamic_cast<juce::AudioParameterChoice*> (param);
        c->combo.addItemList (pc->choices, 1);
        addAndMakeVisible (c->combo);
        c->comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c->combo);
    }
    else
    {
        c->linear = linear;
        if (vertical)
        {
            c->slider.setSliderStyle (juce::Slider::LinearVertical);
            c->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        }
        else if (linear)
        {
            c->slider.setSliderStyle (juce::Slider::LinearHorizontal);
            c->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 18);
        }
        else
        {
            c->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            c->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        }
        c->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        addAndMakeVisible (c->slider);
        c->sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, c->slider);
    }

    c->slider.setTooltip (param->getName (64));
    c->combo.setTooltip (param->getName (64));
    ctls.push_back (std::move (c));
    return ctls.back().get();
}

void DaftSynthEditor::addSection (const char* title, int page, int row, std::initializer_list<std::pair<const char*, const char*>> items)
{
    Section s;
    s.title = title;
    s.page = page;
    s.row = row;
    for (const auto& it : items)
        s.ctls.push_back (addCtl (it.first, it.second));
    sections.push_back (std::move (s));
}

DaftSynthEditor::Ctl* DaftSynthEditor::addManualCtl (const juce::String& label, double lo, double hi, double def, double interval,
                                                    const juce::String& suffix, std::function<void (double)> onChange)
{
    auto c = std::make_unique<Ctl>();
    c->label.setText (label, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (theme::mono (11.0f));
    c->label.setColour (juce::Label::textColourId, theme::lightGray);
    addAndMakeVisible (c->label);

    c->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    c->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    c->slider.setRange (lo, hi, interval);
    c->slider.setValue (def, juce::dontSendNotification);
    c->slider.setDoubleClickReturnValue (true, def);
    c->slider.setTextValueSuffix (suffix);
    auto* sl = &c->slider;
    c->slider.onValueChange = [this, sl, onChange] { if (! syncing) onChange (sl->getValue()); };
    addAndMakeVisible (c->slider);

    ctls.push_back (std::move (c));
    return ctls.back().get();
}

DaftSynthEditor::Ctl* DaftSynthEditor::addManualChoice (const juce::String& label, const juce::StringArray& items, std::function<void (int)> onChange)
{
    auto c = std::make_unique<Ctl>();
    c->isChoice = true;
    c->label.setText (label, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (theme::mono (11.0f));
    c->label.setColour (juce::Label::textColourId, theme::lightGray);
    addAndMakeVisible (c->label);
    c->combo.addItemList (items, 1);
    c->combo.setSelectedId (1, juce::dontSendNotification);
    auto* cb = &c->combo;
    c->combo.onChange = [this, cb, onChange] { if (! syncing) onChange (cb->getSelectedId() - 1); };
    addAndMakeVisible (c->combo);

    ctls.push_back (std::move (c));
    return ctls.back().get();
}

void DaftSynthEditor::syncSliceControls()
{
    const int sel = juce::jlimit (0, dl::SampleEngine::kMaxSlices - 1, proc.sampler.selected.load());
    const auto& s = proc.sampler.slice[sel];
    syncing = true;
    slCtl[0]->slider.setValue (s.pitch.load(), juce::dontSendNotification);
    slCtl[1]->slider.setValue (s.fine.load(), juce::dontSendNotification);
    slCtl[2]->slider.setValue (s.level.load(), juce::dontSendNotification);
    slCtl[3]->slider.setValue (s.pan.load(), juce::dontSendNotification);
    slCtl[4]->slider.setValue (s.drive.load(), juce::dontSendNotification);
    slCtl[5]->slider.setValue (s.crush.load(), juce::dontSendNotification);
    slCtl[6]->combo.setSelectedId (s.reverse.load() ? 2 : 1, juce::dontSendNotification);
    syncing = false;
    lastSelShown = sel;
}

void DaftSynthEditor::updateSampleInfo()
{
    auto d = proc.sampler.getData();
    juce::String t;
    if (proc.isRecording()) t = "ENREGISTREMENT EN COURS... (clique STOP pour terminer)";
    else if (d && d->length() > 0)
        t = d->name + "  |  " + juce::String ((double) d->length() / d->rate, 2) + " s  |  " + juce::String (d->numSlices())
            + " tranches  |  tranche " + juce::String (proc.sampler.selected.load() + 1) + " (touche MIDI "
            + juce::String (juce::roundToInt (proc.apvts.getRawParameterValue ("sliceNote")->load()) + proc.sampler.selected.load()) + ")";
    else t = "Aucun sample. Le mode Sample se choisit en haut a droite.";
    sampleInfo.setText (t, juce::dontSendNotification);
}

void DaftSynthEditor::importSample (const juce::File& f)
{
    if (! proc.loadSampleFile (f))
    {
        sampleInfo.setText ("Impossible de lire ce fichier (formats : wav, aiff, flac, ogg, 10 min max).", juce::dontSendNotification);
        return;
    }
    if (auto* m = proc.apvts.getParameter ("mode")) m->setValueNotifyingHost (m->convertTo0to1 (4.0f));   // passe en mode Sample
    sampleView.refresh();
    syncSliceControls();
    updateSampleInfo();
    setPage (6);
}

bool DaftSynthEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg")) return true;
    return false;
}

void DaftSynthEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg")) { importSample (juce::File (f)); break; }
}

void DaftSynthEditor::addMatrix()
{
    Section s;
    s.title = "MODULATION MATRIX  (source > target, amount -1..+1)";
    s.page = 1;
    s.row = 3;
    s.matrix = true;
    for (int i = 1; i <= DaftSynthProcessor::kNumSlots; ++i)
    {
        const juce::String p = "m" + juce::String (i);
        s.ctls.push_back (addCtl (p + "Src", ""));
        s.ctls.push_back (addCtl (p + "Dst", ""));
        s.ctls.push_back (addCtl (p + "Amt", "", true));
    }
    sections.push_back (std::move (s));
}

DaftSynthEditor::DaftSynthEditor (DaftSynthProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveView (p.apvts), viz (p), sampleView (p.sampler), morphPad (p)
{
    setLookAndFeel (&lnf);

    // ---- Page SOUND
    addSection ("OSCILLATORS", 0, 0, { { "octave", "Octave" }, { "saw", "Saw" }, { "pulse", "Pulse" }, { "pw", "PW" },
                                       { "sub", "Sub" }, { "osc2", "Osc 2" }, { "osc2Semi", "O2 Semi" },
                                       { "osc2Detune", "O2 Fine" }, { "noise", "Noise" } });
    addSection ("FILTER", 0, 0, { { "cutoff", "Cutoff" }, { "res", "Reso" }, { "drive", "Drive" },
                                  { "fenv", "Env Amt" }, { "keytrack", "KeyTrk" }, { "hpf", "HPF" } });
    addSection ("AMP ENV", 0, 1, { { "aA", "Atk" }, { "aD", "Dec" }, { "aS", "Sus" }, { "aR", "Rel" } });
    addSection ("FILTER ENV", 0, 1, { { "fA", "Atk" }, { "fD", "Dec" }, { "fS", "Sus" }, { "fR", "Rel" } });
    addSection ("CLASSIC LFO", 0, 1, { { "lfoShape", "Shape" }, { "lfoSync", "Sync" }, { "lfoRate", "Rate" },
                                       { "lfoPitch", "Pitch" }, { "lfoCutoff", "Cutoff" }, { "lfoPwm", "PWM" } });
    addSection ("PLAY", 0, 1, { { "glide", "Glide" }, { "accent", "Accent" }, { "volume", "Volume" } });
    addSection ("WAVETABLE + UNISON  (clique / glisse sur la forme d'onde)", 0, 2,
                { { "wtTable", "Table" }, { "wtPos", "Position" }, { "wtOct", "Octave" }, { "wtLevel", "Level" },
                  { "uniVoices", "Unison" }, { "uniDetune", "Detune" } });
    sections.back().wave = true;
    addSection ("FM OPERATOR", 0, 2, { { "fmLevel", "Level" }, { "fmAmt", "Index" }, { "fmRatio", "Ratio" },
                                       { "fmFb", "Fdbk" }, { "fmEnv", "Env" } });
    addSection ("ANALOG + OSC MOD", 0, 3, { { "drift", "Drift" }, { "ringMix", "Ring" }, { "ringRatio", "Ring Rt" }, { "sync2", "Sync 2" } });
    addSection ("DRUM MACHINE  (mode Drums : C3 kick, D3 snare, D#3 clap, F#3 hat...)", 0, 3,
                { { "kit", "Kit" }, { "drumTune", "Tune" }, { "drumDecay", "Decay" },
                  { "drumTone", "Tone" }, { "drumLevel", "Level" } });

    // ---- Page MOD
    addSection ("LFO 1", 1, 0, { { "lfo1Shape", "Shape" }, { "lfo1Rate", "Rate" }, { "lfo1Sync", "Sync" }, { "lfo1Retrig", "Retrig" } });
    addSection ("LFO 2", 1, 0, { { "lfo2Shape", "Shape" }, { "lfo2Rate", "Rate" }, { "lfo2Sync", "Sync" }, { "lfo2Retrig", "Retrig" } });
    addSection ("LFO 3", 1, 0, { { "lfo3Shape", "Shape" }, { "lfo3Rate", "Rate" }, { "lfo3Sync", "Sync" }, { "lfo3Retrig", "Retrig" } });
    addSection ("LFO 4", 1, 0, { { "lfo4Shape", "Shape" }, { "lfo4Rate", "Rate" }, { "lfo4Sync", "Sync" }, { "lfo4Retrig", "Retrig" } });
    addSection ("ENVELOPE 1", 1, 1, { { "env1A", "Atk" }, { "env1D", "Dec" }, { "env1S", "Sus" }, { "env1R", "Rel" } });
    addSection ("ENVELOPE 2", 1, 1, { { "env2A", "Atk" }, { "env2D", "Dec" }, { "env2S", "Sus" }, { "env2R", "Rel" } });
    addSection ("MACROS", 1, 1, { { "macro1", "Macro 1" }, { "macro2", "Macro 2" }, { "macro3", "Macro 3" }, { "macro4", "Macro 4" } });
    addMatrix();

    // ---- Page WOW
    addSection ("WOW FILTER", 3, 0, { { "wowType", "Type" }, { "wowPos", "Position" }, { "wowFreq", "Freq" }, { "wowRes", "Reso" },
                                      { "wowVowel", "Vowel" }, { "wowDrive", "Drive" }, { "wowMix", "Mix" } });
    addSection ("WOBBLE  (LFO integre, synchronise au tempo)", 3, 1, { { "wowLfoShape", "Shape" }, { "wowLfoSync", "Sync" },
                                                                         { "wowLfoRate", "Rate" }, { "wowLfoDepth", "Depth" } });

    // ---- Page FX
    addSection ("DISTORTION / SATURATION", 2, 0, { { "distType", "Type" }, { "dist", "Drive" }, { "distBias", "Bias" },
                                                   { "distTone", "Tone" }, { "distMix", "Mix" }, { "distOut", "Output" } });
    addSection ("BITCRUSH", 2, 0, { { "crushBits", "Bits" }, { "crushRate", "Rate" } });
    addSection ("PUMP (SIDECHAIN)", 2, 0, { { "pump", "Amount" }, { "pumpRate", "Rate" }, { "pumpRel", "Release" } });
    addSection ("CHORUS", 2, 1, { { "chorus", "Mode" }, { "chorusMix", "Mix" } });
    addSection ("PHASER", 2, 1, { { "phaserMix", "Mix" }, { "phaserSync", "Sync" }, { "phaserRate", "Rate" },
                                  { "phaserDepth", "Depth" }, { "phaserFb", "Fdbk" }, { "phaserStages", "Stages" } });
    addSection ("FLANGER", 2, 1, { { "flMix", "Mix" }, { "flRate", "Rate" }, { "flDepth", "Depth" },
                                   { "flFb", "Fdbk" }, { "flDelay", "Delay" } });
    addSection ("DELAY", 2, 2, { { "delayMix", "Mix" }, { "delaySync", "Sync" }, { "delayTime", "Time" }, { "delayFb", "Fdbk" },
                                 { "delayPing", "Ping" }, { "delayTone", "Tone" } });
    addSection ("REVERB", 2, 2, { { "reverbMix", "Mix" }, { "reverbSize", "Size" }, { "reverbPre", "PreDly" },
                                  { "reverbDamp", "Damp" }, { "reverbWidth", "Width" } });

    // ---- Page MIX
    addSection ("EQUALIZER", 4, 0, { { "eqLowGain", "Low dB" }, { "eqLowFreq", "Low Hz" }, { "eqMidGain", "Mid dB" },
                                     { "eqMidFreq", "Mid Hz" }, { "eqMidQ", "Mid Q" }, { "eqHighGain", "High dB" },
                                     { "eqHighFreq", "High Hz" } });
    addSection ("OTT MULTIBAND (compression vers le haut et le bas)", 4, 0,
                { { "ottMix", "Mix" }, { "ottDepth", "Depth" }, { "ottTime", "Time" }, { "ottLow", "Low" },
                  { "ottMid", "Mid" }, { "ottHigh", "High" } });
    addSection ("OUTPUT BOOST + LIMITER  (boost = son plus fort, le limiteur empeche de depasser 0 dB)", 4, 1,
                { { "boost", "Boost" }, { "limCeil", "Ceiling" }, { "limRel", "Release" }, { "width", "Width" } });

    // ---- Page FM (6 opérateurs + matrice)
    addSection ("FM GLOBAL", 5, 0, { { "fm6Level", "Level" }, { "fm6Index", "Index" } });
    sections.back().algo = true;
    addSection ("RATIO (par operateur)", 5, 0, { { "fmR1", "Op 1" }, { "fmR2", "Op 2" }, { "fmR3", "Op 3" },
                                                 { "fmR4", "Op 4" }, { "fmR5", "Op 5" }, { "fmR6", "Op 6" } });
    addSection ("SORTIE AUDIO (par operateur)", 5, 0, { { "fmO1", "Op 1" }, { "fmO2", "Op 2" }, { "fmO3", "Op 3" },
                                                        { "fmO4", "Op 4" }, { "fmO5", "Op 5" }, { "fmO6", "Op 6" } });
    addSection ("ENVELOPPE D'INDEX", 5, 0, { { "fmE1", "Op 1" }, { "fmE2", "Op 2" }, { "fmE3", "Op 3" },
                                             { "fmE4", "Op 4" }, { "fmE5", "Op 5" }, { "fmE6", "Op 6" } });
    {
        Section g;
        g.title = "MATRICE DE MODULATION  (ligne = operateur module, colonne = operateur qui le module ; la diagonale = feedback)";
        g.page = 5;
        g.row = 1;
        g.grid = true;
        for (int r = 1; r <= 6; ++r)
            for (int c = 1; c <= 6; ++c)
                g.ctls.push_back (addCtl ("fmM" + juce::String (r) + juce::String (c), "", true));
        sections.push_back (std::move (g));
    }

    algoBox.addItemList (DaftSynthProcessor::fmAlgoNames(), 1);
    algoBox.setTextWhenNothingSelected ("Algorithme...");
    algoBox.onChange = [this] { proc.applyFmAlgo (algoBox.getSelectedId() - 1); };
    addAndMakeVisible (algoBox);

    // ---- Séquenceur de pas (page MOD)
    {
        Section st;
        st.title = "SEQUENCEUR DE PAS  (source de modulation \"Step Seq\" : assigne-la dans la matrice)";
        st.page = 1;
        st.row = 2;
        st.steps = true;
        st.ctls.push_back (addCtl ("stepLen", "Longueur"));
        st.ctls.push_back (addCtl ("stepSync", "Sync"));
        st.ctls.push_back (addCtl ("stepRate", "Rate"));
        st.ctls.push_back (addCtl ("stepGlide", "Glide"));
        for (int i = 1; i <= 16; ++i) st.ctls.push_back (addCtl ("step" + juce::String (i), "", false, true));
        sections.push_back (std::move (st));
    }

    // ---- Page PLAY : macros intelligentes, Morph Pad, arpégiateur
    addSection ("MACROS INTELLIGENTES  (chaque macro agit sur plusieurs reglages, 0.5 = neutre)", 7, 0,
                { { "pm1", "Character" }, { "pm2", "Bright" }, { "pm3", "Warmth" }, { "pm4", "Movement" },
                  { "pm5", "Aggress." }, { "pm6", "Width" }, { "pm7", "Texture" }, { "pm8", "Space" } });
    {
        Section mp;
        mp.title = "MORPH PAD  (regle un son, STORE A..D, active MORPH, bouge le point)";
        mp.page = 7;
        mp.row = 0;
        mp.morph = true;
        mp.extra = 4;
        mp.ctls.push_back (addCtl ("morphOn", "Morph"));
        sections.push_back (std::move (mp));
    }
    addSection ("ARPEGIATEUR  (notes tenues jouees en sequence, synchronise au tempo ; modes Poly / Mono / Acid)", 7, 1,
                { { "arpOn", "Arp" }, { "arpMode", "Mode" }, { "arpRate", "Rate" }, { "arpOct", "Octaves" },
                  { "arpGate", "Gate" }, { "arpSwing", "Swing" }, { "arpHold", "Hold" } });

    for (int k = 0; k < 4; ++k)
    {
        snapBtn[k].setTooltip ("Enregistre le son actuel dans le coin " + juce::String::charToString ((juce::juce_wchar) ('A' + k)) + " du Morph Pad");
        snapBtn[k].onClick = [this, k] { proc.storeSnapshot (k); morphPad.refresh(); };
        addAndMakeVisible (snapBtn[k]);
    }
    addAndMakeVisible (morphPad);

    // ---- Page SAMPLE
    addSection ("LECTURE", 6, 0, { { "sampleLevel", "Level" }, { "sampleTune", "Tune" }, { "sampleAttack", "Atk" },
                                   { "sampleRelease", "Rel" }, { "sampleMode", "Mode" }, { "sampleGate", "Gate" },
                                   { "sampleLoop", "Loop" }, { "sampleRoot", "Root" } });
    addSection ("DECOUPE", 6, 0, { { "sampleSens", "Sensib." }, { "sliceGrid", "Grille" }, { "sliceNote", "1ere note" } });
    {
        Section sl;
        sl.title = "TRANCHE SELECTIONNEE  (pitch, reverse et effets propres a chaque tranche)";
        sl.page = 6;
        sl.row = 1;
        auto sel = [this] { return juce::jlimit (0, dl::SampleEngine::kMaxSlices - 1, proc.sampler.selected.load()); };
        sl.ctls.push_back (slCtl[0] = addManualCtl ("Pitch", -24, 24, 0, 1, " st", [this, sel] (double v) { proc.sampler.slice[sel()].pitch = (float) v; }));
        sl.ctls.push_back (slCtl[1] = addManualCtl ("Fine", -100, 100, 0, 1, " ct", [this, sel] (double v) { proc.sampler.slice[sel()].fine = (float) v; }));
        sl.ctls.push_back (slCtl[2] = addManualCtl ("Level", 0, 2, 1, 0.01, "", [this, sel] (double v) { proc.sampler.slice[sel()].level = (float) v; }));
        sl.ctls.push_back (slCtl[3] = addManualCtl ("Pan", -1, 1, 0, 0.01, "", [this, sel] (double v) { proc.sampler.slice[sel()].pan = (float) v; }));
        sl.ctls.push_back (slCtl[4] = addManualCtl ("Drive", 0, 1, 0, 0.01, "", [this, sel] (double v) { proc.sampler.slice[sel()].drive = (float) v; }));
        sl.ctls.push_back (slCtl[5] = addManualCtl ("Crush", 4, 16, 16, 1, " bit", [this, sel] (double v) { proc.sampler.slice[sel()].crush = (float) v; }));
        sl.ctls.push_back (slCtl[6] = addManualChoice ("Reverse", { "Off", "On" }, [this, sel] (int i) { proc.sampler.slice[sel()].reverse = (i == 1); }));
        sections.push_back (std::move (sl));
    }

    for (auto* b : { &importBtn, &autoBtn, &gridBtn, &clearBtn, &recBtn }) addAndMakeVisible (*b);
    importBtn.setTooltip ("Importer un fichier audio (ou glisse-le dans le plugin)");
    autoBtn.setTooltip ("Decoupe automatique sur les attaques (reglage : Sensib.)");
    gridBtn.setTooltip ("Decoupe en parts egales (reglage : Grille)");
    clearBtn.setTooltip ("Supprimer toutes les coupes");
    recBtn.setTooltip ("Resampling : enregistre ce que joue le plugin (max 60 s) et en fait un sample decoupe");
    importBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Choisir un fichier audio", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile()) importSample (f);
                              });
    };
    autoBtn.onClick = [this] { proc.sampler.autoSlice (proc.apvts.getRawParameterValue ("sampleSens")->load()); syncSliceControls(); };
    gridBtn.onClick = [this] { proc.sampler.gridSlice (juce::roundToInt (proc.apvts.getRawParameterValue ("sliceGrid")->load())); syncSliceControls(); };
    clearBtn.onClick = [this] { proc.sampler.clearSlices(); syncSliceControls(); };
    recBtn.onClick = [this]
    {
        if (proc.isRecording()) { proc.stopRecording(); recBtn.setButtonText ("REC"); }
        else { proc.startRecording(); recBtn.setButtonText ("STOP"); }
    };
    sampleInfo.setFont (theme::mono (11.0f));
    sampleInfo.setColour (juce::Label::textColourId, theme::lightGray);
    addAndMakeVisible (sampleInfo);
    addAndMakeVisible (sampleView);
    sampleView.onSelect = [this] { syncSliceControls(); updateSampleInfo(); };

    modeCtl = addCtl ("mode", "Mode");
    modeCtl->label.setVisible (false);

    addAndMakeVisible (waveView);
    addAndMakeVisible (logo);
    addAndMakeVisible (viz);

    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setFont (theme::mono (11.0f, true));
    modeLabel.setColour (juce::Label::textColourId, theme::pink);
    modeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (modeLabel);

    static const char* pageNames[8] = { "PLAY", "SOUND", "MOD", "FX", "WOW", "MIX", "FM", "SAMPLE" };
    for (int i = 0; i < 8; ++i)
    {
        pageBtn[i].setButtonText (pageNames[i]);
        pageBtn[i].setClickingTogglesState (true);
        pageBtn[i].setRadioGroupId (100);
        pageBtn[i].onClick = [this, i] { setPage (pageOf[i]); };
        addAndMakeVisible (pageBtn[i]);
    }

    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1000)
        {
            const int i = id - 1000;
            if (i >= 0 && i < userFiles.size()) proc.loadUserPreset (userFiles[i]);
        }
        else if (id > 0) proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (presetBox);
    rebuildPresetMenu();
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);

    saveBtn.setTooltip ("Sauvegarder le son actuel comme preset utilisateur");
    saveBtn.onClick = [this] { askSavePreset(); };
    menuBtn.setTooltip ("Copier / coller un preset en texte, liste des parametres pour l'IA, dossier des presets");
    menuBtn.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (menuBtn);

    prevBtn.onClick = [this] { selectPreset (proc.getCurrentProgram() - 1); };
    nextBtn.onClick = [this] { selectPreset (proc.getCurrentProgram() + 1); };
    diceBtn.setTooltip ("Son aleatoire complet");
    mutBtn.setTooltip ("Petite variation du son actuel");
    diceBtn.onClick = [this] { proc.randomizeSound (false); };
    mutBtn.onClick = [this] { proc.randomizeSound (true); };
    addAndMakeVisible (diceBtn);
    addAndMakeVisible (mutBtn);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    addAndMakeVisible (overlay);   // scanlines CRT, toujours au-dessus

    setSize (1100, 822);
    setPage (7);
    startTimerHz (30);
}

DaftSynthEditor::~DaftSynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DaftSynthEditor::selectPreset (int index)
{
    const int n = proc.getNumPrograms();
    index = (index % n + n) % n;
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

void DaftSynthEditor::rebuildPresetMenu()
{
    const int keep = presetBox.getSelectedId();
    presetBox.clear (juce::dontSendNotification);
    const auto& presets = factoryPresets();
    juce::String lastCat;
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (lastCat != presets[i].category)
        {
            presetBox.addSectionHeading (presets[i].category);
            lastCat = presets[i].category;
        }
        presetBox.addItem (presets[i].name, (int) i + 1);
    }

    userFiles = proc.listUserPresets();
    if (userFiles.size() > 0)
    {
        presetBox.addSectionHeading ("Mes presets");
        for (int i = 0; i < userFiles.size(); ++i)
            presetBox.addItem (userFiles[i].getFileNameWithoutExtension(), 1000 + i);
    }
    if (keep > 0) presetBox.setSelectedId (keep, juce::dontSendNotification);
}

void DaftSynthEditor::askSavePreset()
{
    auto* w = new juce::AlertWindow ("Sauvegarder le preset", "Nom du preset :", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", "Mon son");
    w->addButton ("Sauvegarder", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<DaftSynthEditor> self (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([self, w] (int r)
    {
        if (r != 1 || self == nullptr) return;
        const auto name = w->getTextEditorContents ("name");
        if (self->proc.saveUserPreset (name))
        {
            self->rebuildPresetMenu();
            for (int i = 0; i < self->userFiles.size(); ++i)
                if (self->userFiles[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (name.trim()))
                    self->presetBox.setSelectedId (1000 + i, juce::dontSendNotification);
        }
    }), true);
}

void DaftSynthEditor::showPresetMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Copier le preset (texte)");
    m.addItem (2, "Coller un preset (texte)");
    m.addItem (3, "Copier la liste des parametres (pour demander un son a une IA)");
    m.addSeparator();
    m.addItem (4, "Supprimer ce preset utilisateur", presetBox.getSelectedId() >= 1000);
    m.addItem (5, "Ouvrir le dossier des presets");
    juce::Component::SafePointer<DaftSynthEditor> self (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuBtn), [self] (int r)
    {
        if (self == nullptr || r == 0) return;
        auto& p = self->proc;
        if (r == 1)
            juce::SystemClipboard::copyTextToClipboard (p.exportPresetText (self->presetBox.getText().isNotEmpty() ? self->presetBox.getText() : "Mon son"));
        else if (r == 2)
        {
            juce::String name;
            if (p.importPresetText (juce::SystemClipboard::getTextFromClipboard(), &name))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "DaftSynth", "Preset charge : " + name
                                                        + "\nClique sur SAVE pour le garder dans tes presets.");
            else
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "DaftSynth",
                                                        "Le texte du presse-papiers n'est pas un preset DaftSynth valide.");
        }
        else if (r == 3)
        {
            juce::SystemClipboard::copyTextToClipboard (p.describeParamsForAI());
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "DaftSynth",
                                                    "Liste copiee. Colle-la dans une conversation avec une IA, ajoute la description du son voulu, "
                                                    "puis colle sa reponse avec 'Coller un preset (texte)'.");
        }
        else if (r == 4)
        {
            const int i = self->presetBox.getSelectedId() - 1000;
            if (i >= 0 && i < self->userFiles.size()) { self->userFiles[i].deleteFile(); self->presetBox.setSelectedId (0, juce::dontSendNotification); self->rebuildPresetMenu(); }
        }
        else if (r == 5)
            DaftSynthProcessor::userPresetDir().revealToUser();
    });
}

void DaftSynthEditor::setPage (int newPage)
{
    page = newPage;
    for (int i = 0; i < 8; ++i)
        pageBtn[i].setToggleState (pageOf[i] == page, juce::dontSendNotification);

    for (auto& c : ctls)
        if (c.get() != modeCtl) c->show (false);

    for (auto& sec : sections)
        if (sec.page == page)
            for (auto* c : sec.ctls) c->show (true);

    waveView.setVisible (page == 0);
    morphPad.setVisible (page == 7);
    for (auto& b : snapBtn) b.setVisible (page == 7);
    algoBox.setVisible (page == 5);
    for (auto* c : std::initializer_list<juce::Component*> { &sampleView, &importBtn, &autoBtn, &gridBtn, &clearBtn, &recBtn, &sampleInfo })
        c->setVisible (page == 6);
    if (page == 6) { sampleView.refresh(); syncSliceControls(); updateSampleInfo(); }
    resized();
    repaint();
}

void DaftSynthEditor::repaintPanelBorders()
{
    for (const auto& sec : sections)
    {
        if (sec.page != page) continue;
        const auto b = sec.bounds;
        repaint (b.getX(), b.getY(), b.getWidth(), 8);
        repaint (b.getX(), b.getBottom() - 8, b.getWidth(), 8);
        repaint (b.getX(), b.getY(), 8, b.getHeight());
        repaint (b.getRight() - 8, b.getY(), 8, b.getHeight());
    }
}

void DaftSynthEditor::timerCallback()
{
    if (++tick % 6 == 0)
    {
        const int id = proc.getCurrentProgram() + 1;
        if (presetBox.getSelectedId() < 1000 && presetBox.getSelectedId() != id)
            presetBox.setSelectedId (id, juce::dontSendNotification);
    }

    if (page == 7) morphPad.refresh();

    waveView.refresh();
    if (proc.pollRecording())
    {
        recBtn.setButtonText ("REC");
        sampleView.refresh();
        syncSliceControls();
        updateSampleInfo();
    }
    if (page == 6)
    {
        sampleView.refresh();
        if (proc.sampler.selected.load() != lastSelShown) { syncSliceControls(); updateSampleInfo(); }
        if (tick % 15 == 0) updateSampleInfo();
    }
    energy = viz.getEnergy();
    logo.update (energy, viz.getGlitch());

    // Les bordures néon des panneaux pulsent avec le son
    if (std::abs (energy - paintedEnergy) > 0.015f)
    {
        paintedEnergy = energy;
        repaintPanelBorders();
    }
}

void DaftSynthEditor::paint (juce::Graphics& g)
{
    using namespace theme;
    g.fillAll (black);

    for (const auto& s : sections)
    {
        if (s.page != page) continue;
        const auto r = s.bounds.toFloat().reduced (3.0f);
        const float c = 9.0f;
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getY());
        p.lineTo (r.getRight() - c, r.getY());
        p.lineTo (r.getRight(), r.getY() + c);
        p.lineTo (r.getRight(), r.getBottom());
        p.lineTo (r.getX() + c, r.getBottom());
        p.lineTo (r.getX(), r.getBottom() - c);
        p.closeSubPath();

        g.setColour (darkGray);
        g.fillPath (p);

        const float pulse = juce::jlimit (0.0f, 1.0f, energy * 2.0f);
        g.setColour (purple.withAlpha (0.05f + 0.20f * pulse));
        g.strokePath (p, juce::PathStrokeType (5.0f));
        g.setColour (purple.withAlpha (0.10f + 0.25f * pulse));
        g.strokePath (p, juce::PathStrokeType (3.0f));
        g.setColour (purple.withAlpha (0.40f + 0.55f * pulse));
        g.strokePath (p, juce::PathStrokeType (1.2f));

        // accents roses dans deux coins
        g.setColour (pink.withAlpha (0.75f + 0.25f * pulse));
        g.drawLine (r.getX(), r.getY(), r.getX() + 14.0f, r.getY(), 2.0f);
        g.drawLine (r.getRight(), r.getBottom(), r.getRight() - 14.0f, r.getBottom(), 2.0f);

        // titre
        g.setColour (pink);
        g.fillRect (r.getX() + 8.0f, r.getY() + 9.0f, 4.0f, 4.0f);
        g.setFont (mono (11.0f, true));
        g.drawText (s.title, juce::Rectangle<int> ((int) r.getX() + 17, (int) r.getY() + 4, (int) r.getWidth() - 24, 14),
                    juce::Justification::centredLeft);

        if (s.grid)
        {
            g.setColour (lightGray.withAlpha (0.7f));
            g.setFont (mono (10.0f));
            const int labelW = 70, cw = (gridArea.getWidth() - labelW) / 6, rh = gridArea.getHeight() / 6;
            for (int c = 0; c < 6; ++c)
                g.drawText ("mod par Op " + juce::String (c + 1), gridArea.getX() + labelW + c * cw, gridArea.getY() - 15, cw, 14, juce::Justification::centred);
            for (int rr = 0; rr < 6; ++rr)
            {
                g.setColour (pink.withAlpha (0.9f));
                g.drawText ("OP " + juce::String (rr + 1), gridArea.getX(), gridArea.getY() + rr * rh, labelW - 6, rh, juce::Justification::centredRight);
            }
        }

        if (s.matrix)
        {
            g.setColour (lightGray.withAlpha (0.6f));
            g.setFont (mono (10.0f));
            const int colW = matrixHeaderArea.getWidth() / 2;
            for (int col = 0; col < 2; ++col)
            {
                const int x0 = matrixHeaderArea.getX() + col * colW;
                g.drawText ("SOURCE", x0, matrixHeaderArea.getY(), 105, 14, juce::Justification::centredLeft);
                g.drawText ("TARGET", x0 + 111, matrixHeaderArea.getY(), 150, 14, juce::Justification::centredLeft);
                g.drawText ("AMOUNT", x0 + 267, matrixHeaderArea.getY(), 100, 14, juce::Justification::centredLeft);
            }
        }
    }
}

void DaftSynthEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    // En-tête : logo | presets (< nom > SAVE ...) | DICE MUT | mode
    auto header = area.removeFromTop (44);
    logo.setBounds (header.removeFromLeft (140));
    auto modeArea = header.removeFromRight (190);
    modeCtl->combo.setBounds (modeArea.removeFromRight (135).withSizeKeepingCentre (135, 28));
    modeLabel.setBounds (modeArea);
    mutBtn.setBounds (header.removeFromRight (58).withSizeKeepingCentre (54, 28));
    diceBtn.setBounds (header.removeFromRight (62).withSizeKeepingCentre (58, 28));
    menuBtn.setBounds (header.removeFromRight (40).withSizeKeepingCentre (36, 28));
    saveBtn.setBounds (header.removeFromRight (60).withSizeKeepingCentre (56, 28));
    prevBtn.setBounds (header.removeFromLeft (34).withSizeKeepingCentre (30, 28));
    nextBtn.setBounds (header.removeFromRight (34).withSizeKeepingCentre (30, 28));
    presetBox.setBounds (header.reduced (6, 0).withSizeKeepingCentre (header.getWidth() - 12, 28));

    area.removeFromTop (4);
    viz.setBounds (area.removeFromTop (104));
    area.removeFromTop (4);

    // Rangée d'onglets (pages)
    {
        auto tabs = area.removeFromTop (30);
        const int tw = tabs.getWidth() / 8;
        for (int i = 0; i < 8; ++i)
            pageBtn[i].setBounds (tabs.removeFromLeft (tw).reduced (3, 1));
        area.removeFromTop (4);
    }

    if (page == 6)
    {
        auto tb = area.removeFromTop (28);
        for (auto* b : { &importBtn, &autoBtn, &gridBtn, &clearBtn, &recBtn })
            b->setBounds (tb.removeFromLeft (76).reduced (2, 0));
        sampleInfo.setBounds (tb.reduced (8, 0));
        area.removeFromTop (4);
        sampleView.setBounds (area.removeFromTop (224));
        area.removeFromTop (6);
    }

    // Hauteur des rangées selon la page
    const int avail = area.getHeight();
    std::vector<int> heights;
    if (page == 0)      heights = { avail / 4, avail / 4, avail / 4, avail - 3 * (avail / 4) };
    else if (page == 1) heights = { 130, 130, 118, avail - 378 };
    else if (page == 7) heights = { avail * 58 / 100, avail - avail * 58 / 100 };
    else if (page == 6) heights = { avail / 2, avail - avail / 2 };
    else if (page == 5) heights = { avail * 38 / 100, avail - avail * 38 / 100 };
    else if (page == 3 || page == 4) heights = { avail * 55 / 100, avail - avail * 55 / 100 };
    else                heights = { avail / 3, avail / 3, avail - 2 * (avail / 3) };

    int y = area.getY();

    for (int row = 0; row < (int) heights.size(); ++row)
    {
        int total = 0;
        for (const auto& s : sections)
            if (s.page == page && s.row == row && ! s.matrix && ! s.grid) total += (s.steps ? 0 : (int) s.ctls.size()) + (s.wave ? 2 : 0) + (s.algo ? 1 : 0) + s.extra;

        const float unit = total > 0 ? (float) area.getWidth() / (float) total : 0.0f;
        float x = (float) area.getX();

        for (auto& s : sections)
        {
            if (s.page != page || s.row != row) continue;

            if (s.steps)
            {
                s.bounds = { area.getX(), y, area.getWidth(), heights[(size_t) row] };
                auto inner = s.bounds.reduced (8, 4);
                inner.removeFromTop (20);
                const int kw = 76;
                for (int i = 0; i < 4; ++i)
                {
                    auto cell = inner.removeFromLeft (kw);
                    s.ctls[(size_t) i]->label.setBounds (cell.removeFromTop (14));
                    if (s.ctls[(size_t) i]->isChoice) s.ctls[(size_t) i]->combo.setBounds (cell.withSizeKeepingCentre (kw - 8, 24));
                    else s.ctls[(size_t) i]->slider.setBounds (cell);
                }
                inner.removeFromLeft (10);
                const int sw = inner.getWidth() / 16;
                for (int i = 0; i < 16; ++i)
                    s.ctls[(size_t) (4 + i)]->slider.setBounds (inner.getX() + i * sw + 3, inner.getY() + 2, sw - 6, inner.getHeight() - 4);
                continue;
            }

            if (s.grid)
            {
                s.bounds = { area.getX(), y, area.getWidth(), heights[(size_t) row] };
                auto inner = s.bounds.reduced (10, 4);
                inner.removeFromTop (20);
                inner.removeFromTop (16);                       // en-têtes de colonnes
                gridArea = inner;
                const int labelW = 70, cw = (inner.getWidth() - labelW) / 6, rh = inner.getHeight() / 6;
                for (int r = 0; r < 6; ++r)
                    for (int c = 0; c < 6; ++c)
                        s.ctls[(size_t) (r * 6 + c)]->slider.setBounds (inner.getX() + labelW + c * cw + 4, inner.getY() + r * rh + 2, cw - 8, rh - 4);
                continue;
            }

            if (s.matrix)
            {
                s.bounds = { area.getX(), y, area.getWidth(), heights[(size_t) row] };
                auto inner = s.bounds.reduced (10, 4);
                inner.removeFromTop (20);
                matrixHeaderArea = inner.removeFromTop (16);
                const int colW = inner.getWidth() / 2;
                const int rowH = inner.getHeight() / 6;

                for (int slot = 0; slot < DaftSynthProcessor::kNumSlots; ++slot)
                {
                    const int col = slot / 6, r = slot % 6;
                    auto cell = juce::Rectangle<int> (inner.getX() + col * colW, inner.getY() + r * rowH, colW - 14, rowH).reduced (0, 3);
                    s.ctls[(size_t) slot * 3]->combo.setBounds (cell.removeFromLeft (105));
                    cell.removeFromLeft (6);
                    s.ctls[(size_t) slot * 3 + 1]->combo.setBounds (cell.removeFromLeft (150));
                    cell.removeFromLeft (6);
                    s.ctls[(size_t) slot * 3 + 2]->slider.setBounds (cell);
                }
                continue;
            }

            const float units = (float) s.ctls.size() + (s.wave ? 2.0f : 0.0f) + (s.algo ? 1.0f : 0.0f) + (float) s.extra;
            const int w = juce::roundToInt (unit * units);
            s.bounds = { juce::roundToInt (x), y, w, heights[(size_t) row] };
            x += unit * units;

            auto inner = s.bounds.reduced (6, 4);
            inner.removeFromTop (20);
            if (s.wave)
                waveView.setBounds (inner.removeFromLeft (juce::roundToInt (unit * 2.0f)).reduced (2, 4));
            if (s.algo)
            {
                auto top = inner.removeFromTop (30);
                algoBox.setBounds (top.reduced (2, 2));
            }
            if (s.morph)
            {
                // pad carré à gauche ; à droite : interrupteur Morph et boutons STORE A..D
                auto padArea = inner.removeFromLeft (juce::jmin (inner.getHeight(), juce::roundToInt (unit * 2.6f)));
                morphPad.setBounds (padArea.reduced (2, 4));
                auto col = inner.reduced (6, 0);
                s.ctls[0]->label.setBounds (col.removeFromTop (16));
                s.ctls[0]->combo.setBounds (col.removeFromTop (28));
                col.removeFromTop (8);
                const int bh = std::min (34, col.getHeight() / 2);
                auto r1 = col.removeFromTop (bh), r2 = col.removeFromTop (bh);
                snapBtn[2].setBounds (r1.removeFromLeft (r1.getWidth() / 2).reduced (2));
                snapBtn[3].setBounds (r1.reduced (2));
                snapBtn[0].setBounds (r2.removeFromLeft (r2.getWidth() / 2).reduced (2));
                snapBtn[1].setBounds (r2.reduced (2));
                continue;
            }
            const int cellW = inner.getWidth() / (int) s.ctls.size();

            for (auto* c : s.ctls)
            {
                auto cell = inner.removeFromLeft (cellW);
                c->label.setBounds (cell.removeFromTop (16));
                if (c->isChoice)
                    c->combo.setBounds (cell.withSizeKeepingCentre (juce::jmax (40, cell.getWidth() - 6), 26));
                else
                    c->slider.setBounds (cell);
            }
        }
        y += heights[(size_t) row];
    }

    overlay.setBounds (getLocalBounds());
    repaint();
}
