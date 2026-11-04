#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "WaveTables.h"
#include "Theme.h"

// Affiche la wavetable courante (images en filigrane + image active en rose).
// On peut cliquer / glisser dessus pour changer la position dans la table.
class WaveDisplay : public juce::Component
{
public:
    explicit WaveDisplay (juce::AudioProcessorValueTreeState& a) : apvts (a) { setMouseCursor (juce::MouseCursor::LeftRightResizeCursor); }

    void refresh()
    {
        const int t = (int) apvts.getRawParameterValue ("wtTable")->load();
        const float p = apvts.getRawParameterValue ("wtPos")->load();
        const float lv = apvts.getRawParameterValue ("wtLevel")->load();
        if (t != lastT || std::fabs (p - lastPos) > 0.002f || std::fabs (lv - lastLevel) > 0.01f)
        {
            lastT = t;
            lastPos = p;
            lastLevel = lv;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace theme;
        const auto r = getLocalBounds().toFloat();
        g.setColour (black);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (purple.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

        const auto& wt = dl::WaveTables::get();
        const int t = std::clamp (lastT, 0, dl::WaveTables::T - 1);
        const float w = r.getWidth() - 12.0f, h = r.getHeight();
        const float cy = h * 0.5f, amp = h * 0.32f;
        const bool active = lastLevel > 0.001f;

        auto makePath = [&] (int f)
        {
            juce::Path p;
            const float* d = wt.frame (t, f, dl::WaveTables::L - 1);
            for (int i = 0; i <= 160; ++i)
            {
                const float x = 6.0f + w * (float) i / 160.0f;
                const float y = cy - d[(i * (dl::WaveTables::N - 1)) / 160] * amp;
                if (i == 0) p.startNewSubPath (x, y);
                else p.lineTo (x, y);
            }
            return p;
        };

        g.setColour (purple.withAlpha (0.25f));
        for (int f = 0; f < dl::WaveTables::F; f += 3)
            g.strokePath (makePath (f), juce::PathStrokeType (1.0f));

        const int cur = std::clamp ((int) std::round (lastPos * (float) (dl::WaveTables::F - 1)), 0, dl::WaveTables::F - 1);
        const auto path = makePath (cur);
        const auto col = active ? pink : lightGray.withAlpha (0.5f);
        g.setColour (col.withAlpha (0.15f));
        g.strokePath (path, juce::PathStrokeType (6.0f));
        g.setColour (col);
        g.strokePath (path, juce::PathStrokeType (1.8f));

        g.setFont (mono (10.0f));
        g.setColour (lightGray.withAlpha (0.8f));
        g.drawText (dl::WaveTables::name (t), getLocalBounds().reduced (8, 3), juce::Justification::topLeft);
        g.drawText (juce::String (lastPos, 2), getLocalBounds().reduced (8, 3), juce::Justification::bottomRight);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (auto* p = apvts.getParameter ("wtPos")) p->beginChangeGesture();
        setFromMouse (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (auto* p = apvts.getParameter ("wtPos")) p->endChangeGesture();
    }

private:
    void setFromMouse (const juce::MouseEvent& e)
    {
        if (auto* p = apvts.getParameter ("wtPos"))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (float) e.x / (float) std::max (1, getWidth())));
    }

    juce::AudioProcessorValueTreeState& apvts;
    int lastT = 0;
    float lastPos = 0.0f, lastLevel = 0.0f;
};
