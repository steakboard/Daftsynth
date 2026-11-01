#include "PluginEditor.h"


DaftSynthEditor::Ctl* DaftSynthEditor::addCtl (const juce::String& id, const juce::String& shortLabel, bool linear)
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
        if (linear)
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

void DaftSynthEditor::addMatrix()
{
    Section s;
    s.title = "MODULATION MATRIX  (source > target, amount -1..+1)";
    s.page = 1;
    s.row = 2;
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
    : AudioProcessorEditor (&p), proc (p), viz (p)
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
    addSection ("DRUM MACHINE  (mode Drums : C3 kick, D3 snare, D#3 clap, F#3 hat, A#3 open hat...)", 0, 2,
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

    // ---- Page FX
    addSection ("DISTORTION / SATURATION", 2, 0, { { "distType", "Type" }, { "dist", "Drive" }, { "distBias", "Bias" },
                                                   { "distTone", "Tone" }, { "distMix", "Mix" }, { "distOut", "Output" } });
    addSection ("BITCRUSH", 2, 0, { { "crushBits", "Bits" }, { "crushRate", "Rate" } });
    addSection ("PUMP (SIDECHAIN)", 2, 0, { { "pump", "Amount" }, { "pumpRate", "Rate" }, { "pumpRel", "Release" } });
    addSection ("CHORUS", 2, 1, { { "chorus", "Mode" } });
    addSection ("PHASER", 2, 1, { { "phaserMix", "Mix" }, { "phaserSync", "Sync" }, { "phaserRate", "Rate" },
                                  { "phaserDepth", "Depth" }, { "phaserFb", "Fdbk" } });
    addSection ("DELAY", 2, 1, { { "delayMix", "Mix" }, { "delaySync", "Sync" }, { "delayTime", "Time" }, { "delayFb", "Fdbk" } });
    addSection ("REVERB", 2, 1, { { "reverbMix", "Mix" }, { "reverbSize", "Size" } });

    modeCtl = addCtl ("mode", "Mode");
    modeCtl->label.setVisible (false);

    addAndMakeVisible (logo);
    addAndMakeVisible (viz);

    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setFont (theme::mono (11.0f, true));
    modeLabel.setColour (juce::Label::textColourId, theme::pink);
    modeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (modeLabel);

    static const char* pageNames[3] = { "SOUND", "MOD", "FX" };
    for (int i = 0; i < 3; ++i)
    {
        pageBtn[i].setButtonText (pageNames[i]);
        pageBtn[i].setClickingTogglesState (true);
        pageBtn[i].setRadioGroupId (100);
        pageBtn[i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (pageBtn[i]);
    }

    // Menu des presets, groupés par catégorie
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
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this] { selectPreset (proc.getCurrentProgram() - 1); };
    nextBtn.onClick = [this] { selectPreset (proc.getCurrentProgram() + 1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    addAndMakeVisible (overlay);   // scanlines CRT, toujours au-dessus

    setSize (1100, 780);
    setPage (0);
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

void DaftSynthEditor::setPage (int newPage)
{
    page = newPage;
    for (int i = 0; i < 3; ++i)
        pageBtn[i].setToggleState (i == page, juce::dontSendNotification);

    for (auto& c : ctls)
        if (c.get() != modeCtl) c->show (false);

    for (auto& sec : sections)
        if (sec.page == page)
            for (auto* c : sec.ctls) c->show (true);

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
        if (presetBox.getSelectedId() != id)
            presetBox.setSelectedId (id, juce::dontSendNotification);
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

    // En-tête : logo | pages | presets | mode
    auto header = area.removeFromTop (44);
    logo.setBounds (header.removeFromLeft (230));
    for (int i = 0; i < 3; ++i)
        pageBtn[i].setBounds (header.removeFromLeft (80).withSizeKeepingCentre (76, 28));
    auto modeArea = header.removeFromRight (250);
    modeCtl->combo.setBounds (modeArea.removeFromRight (170).withSizeKeepingCentre (170, 28));
    modeLabel.setBounds (modeArea);
    prevBtn.setBounds (header.removeFromLeft (34).withSizeKeepingCentre (30, 28));
    nextBtn.setBounds (header.removeFromRight (34).withSizeKeepingCentre (30, 28));
    presetBox.setBounds (header.reduced (6, 0).withSizeKeepingCentre (header.getWidth() - 12, 28));

    area.removeFromTop (4);
    viz.setBounds (area.removeFromTop (104));
    area.removeFromTop (6);

    // Hauteur des rangées selon la page
    const int avail = area.getHeight();
    std::vector<int> heights;
    if (page == 0)      heights = { avail * 36 / 100, avail * 36 / 100, avail - 2 * (avail * 36 / 100) };
    else if (page == 1) heights = { 150, 150, avail - 300 };
    else                heights = { avail / 2, avail - avail / 2 };

    int y = area.getY();

    for (int row = 0; row < (int) heights.size(); ++row)
    {
        int total = 0;
        for (const auto& s : sections)
            if (s.page == page && s.row == row && ! s.matrix) total += (int) s.ctls.size();

        const float unit = total > 0 ? (float) area.getWidth() / (float) total : 0.0f;
        float x = (float) area.getX();

        for (auto& s : sections)
        {
            if (s.page != page || s.row != row) continue;

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

            const int w = juce::roundToInt (unit * (float) s.ctls.size());
            s.bounds = { juce::roundToInt (x), y, w, heights[(size_t) row] };
            x += unit * (float) s.ctls.size();

            auto inner = s.bounds.reduced (6, 4);
            inner.removeFromTop (20);
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
