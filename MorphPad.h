#pragma once
#include "PluginProcessor.h"
#include "Theme.h"

// Pad XY : mélange en continu 4 états sonores (A bas-gauche, B bas-droite, C haut-gauche, D haut-droite)
class MorphPad : public juce::Component
{
public:
    explicit MorphPad (DaftSynthProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }

    void refresh()
    {
        const float x = proc.apvts.getRawParameterValue ("morphX")->load();
        const float y = proc.apvts.getRawParameterValue ("morphY")->load();
        const bool on = proc.apvts.getRawParameterValue ("morphOn")->load() > 0.5f;
        int valid = 0;
        for (int k = 0; k < 4; ++k) valid |= (proc.snapshotValid (k) ? 1 : 0) << k;
        if (std::fabs (x - lx) > 0.002f || std::fabs (y - ly) > 0.002f || on != lon || valid != lvalid)
        {
            lx = x; ly = y; lon = on; lvalid = valid;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace theme;
        const auto r = getLocalBounds().toFloat();
        g.setColour (black);
        g.fillRoundedRectangle (r, 4.0f);

        g.setColour (purple.withAlpha (0.18f));
        for (int i = 1; i < 4; ++i)
        {
            g.drawVerticalLine ((int) (r.getWidth() * (float) i / 4.0f), 0.0f, r.getHeight());
            g.drawHorizontalLine ((int) (r.getHeight() * (float) i / 4.0f), 0.0f, r.getWidth());
        }

        g.setFont (mono (13.0f, true));
        const char* names[4] = { "A", "B", "C", "D" };
        const juce::Rectangle<float> corners[4] = {
            { 6.0f, r.getHeight() - 22.0f, 20.0f, 16.0f }, { r.getWidth() - 26.0f, r.getHeight() - 22.0f, 20.0f, 16.0f },
            { 6.0f, 6.0f, 20.0f, 16.0f }, { r.getWidth() - 26.0f, 6.0f, 20.0f, 16.0f } };
        for (int k = 0; k < 4; ++k)
        {
            g.setColour (((lvalid >> k) & 1) ? pink : lightGray.withAlpha (0.35f));
            g.drawText (names[k], corners[k], juce::Justification::centred);
        }

        const float px = lx * r.getWidth(), py = (1.0f - ly) * r.getHeight();
        const auto col = lon ? pink : lightGray.withAlpha (0.5f);
        g.setColour (col.withAlpha (0.12f));
        g.fillEllipse (px - 16.0f, py - 16.0f, 32.0f, 32.0f);
        g.setColour (col.withAlpha (0.3f));
        g.fillEllipse (px - 9.0f, py - 9.0f, 18.0f, 18.0f);
        g.setColour (col);
        g.fillEllipse (px - 4.0f, py - 4.0f, 8.0f, 8.0f);

        g.setColour (purple.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto* id : { "morphX", "morphY" }) if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
        set (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { set (e); }
    void mouseUp (const juce::MouseEvent&) override
    {
        for (auto* id : { "morphX", "morphY" }) if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
    }

private:
    void set (const juce::MouseEvent& e)
    {
        const float x = juce::jlimit (0.0f, 1.0f, (float) e.x / (float) std::max (1, getWidth()));
        const float y = juce::jlimit (0.0f, 1.0f, 1.0f - (float) e.y / (float) std::max (1, getHeight()));
        if (auto* px = proc.apvts.getParameter ("morphX")) px->setValueNotifyingHost (x);
        if (auto* py = proc.apvts.getParameter ("morphY")) py->setValueNotifyingHost (y);
    }

    DaftSynthProcessor& proc;
    float lx = -1.0f, ly = -1.0f;
    bool lon = false;
    int lvalid = -1;
};
