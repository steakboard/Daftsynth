#include "PluginEditor.h"


DaftSynthEditor::Ctl* DaftSynthEditor::addCtl (const char* id, const char* shortLabel)
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
        c->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        c->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        c->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        addAndMakeVisible (c->slider);
        c->sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, c->slider);
    }

    ctls.push_back (std::move (c));
    return ctls.back().get();
}

void DaftSynthEditor::addSection (const char* title, int row, std::initializer_list<std::pair<const char*, const char*>> items)
{
    Section s;
    s.title = title;
    s.row = row;
    for (const auto& it : items)
        s.ctls.push_back (addCtl (it.first, it.second));
    sections.push_back (std::move (s));
}

DaftSynthEditor::DaftSynthEditor (DaftSynthProcessor& p)
    : AudioProcessorEditor (&p), proc (p), viz (p)
{
    setLookAndFeel (&lnf);

    // Rangée 1 : oscillateurs + filtre
    addSection ("OSCILLATORS", 0, { { "octave", "Octave" }, { "saw", "Saw" }, { "pulse", "Pulse" }, { "pw", "PW" },
                                    { "sub", "Sub" }, { "osc2", "Osc 2" }, { "osc2Semi", "O2 Semi" },
                                    { "osc2Detune", "O2 Fine" }, { "noise", "Noise" } });
    addSection ("FILTER", 0, { { "cutoff", "Cutoff" }, { "res", "Reso" }, { "drive", "Drive" },
                               { "fenv", "Env Amt" }, { "keytrack", "KeyTrk" }, { "hpf", "HPF" } });

    // Rangée 2 : enveloppes, LFO, jeu
    addSection ("AMP ENV", 1, { { "aA", "Atk" }, { "aD", "Dec" }, { "aS", "Sus" }, { "aR", "Rel" } });
    addSection ("FILTER ENV", 1, { { "fA", "Atk" }, { "fD", "Dec" }, { "fS", "Sus" }, { "fR", "Rel" } });
    addSection ("LFO", 1, { { "lfoShape", "Shape" }, { "lfoRate", "Rate" }, { "lfoPitch", "Pitch" },
                            { "lfoCutoff", "Cutoff" }, { "lfoPwm", "PWM" } });
    addSection ("PLAY", 1, { { "glide", "Glide" }, { "accent", "Accent" }, { "volume", "Volume" } });

    // Rangée 3 : effets
    addSection ("DIST / CRUSH", 2, { { "dist", "Dist" }, { "crushBits", "Bits" }, { "crushRate", "Rate" } });
    addSection ("CHORUS / PHASER", 2, { { "chorus", "Chorus" }, { "phaserMix", "Mix" }, { "phaserRate", "Rate" },
                                        { "phaserDepth", "Depth" }, { "phaserFb", "Fdbk" } });
    addSection ("DELAY", 2, { { "delayMix", "Mix" }, { "delayTime", "Time" }, { "delayFb", "Fdbk" } });
    addSection ("REVERB", 2, { { "reverbMix", "Mix" }, { "reverbSize", "Size" } });
    addSection ("PUMP (SIDECHAIN)", 2, { { "pump", "Amount" }, { "pumpRate", "Rate" }, { "pumpRel", "Release" } });

    // Rangée 4 : drums
    addSection ("DRUM MACHINE  (mode Drums : C3 kick, D3 snare, D#3 clap, F#3 hat, A#3 open hat...)", 3,
                { { "kit", "Kit" }, { "drumTune", "Tune" }, { "drumDecay", "Decay" },
                  { "drumTone", "Tone" }, { "drumLevel", "Level" } });

    modeCtl = addCtl ("mode", "Mode");
    modeCtl->label.setVisible (false);

    addAndMakeVisible (logo);
    addAndMakeVisible (viz);

    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setFont (theme::mono (11.0f, true));
    modeLabel.setColour (juce::Label::textColourId, theme::pink);
    modeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (modeLabel);

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

void DaftSynthEditor::repaintPanelBorders()
{
    for (const auto& sec : sections)
    {
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
    }
}

void DaftSynthEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    // En-tête : logo | presets | mode
    auto header = area.removeFromTop (44);
    logo.setBounds (header.removeFromLeft (230));
    auto modeArea = header.removeFromRight (250);
    modeCtl->combo.setBounds (modeArea.removeFromRight (170).withSizeKeepingCentre (170, 28));
    modeLabel.setBounds (modeArea);
    prevBtn.setBounds (header.removeFromLeft (34).withSizeKeepingCentre (30, 28));
    nextBtn.setBounds (header.removeFromRight (34).withSizeKeepingCentre (30, 28));
    presetBox.setBounds (header.reduced (6, 0).withSizeKeepingCentre (header.getWidth() - 12, 28));

    area.removeFromTop (4);
    viz.setBounds (area.removeFromTop (104));
    area.removeFromTop (6);

    // Rangées de sections
    const int rowHeights[4] = { 150, 150, 150, 132 };
    int y = area.getY();

    for (int row = 0; row < 4; ++row)
    {
        int total = 0;
        for (const auto& s : sections)
            if (s.row == row) total += (int) s.ctls.size();

        const float unit = total > 0 ? (float) area.getWidth() / (float) total : 0.0f;
        float x = (float) area.getX();

        for (auto& s : sections)
        {
            if (s.row != row) continue;

            const int w = juce::roundToInt (unit * (float) s.ctls.size());
            s.bounds = { juce::roundToInt (x), y, w, rowHeights[row] };
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
        y += rowHeights[row];
    }

    overlay.setBounds (getLocalBounds());
    repaint();
}
