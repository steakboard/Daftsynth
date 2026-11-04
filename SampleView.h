#pragma once
#include <functional>
#include "SampleEngine.h"
#include "Theme.h"

// Forme d'onde du sample avec tranches et marqueurs.
//  - clic : sélectionne une tranche
//  - glisser près d'un marqueur : le déplace
//  - double-clic : ajoute un marqueur ; clic droit (ou Ctrl+clic) sur un marqueur : le supprime
class SampleView : public juce::Component
{
public:
    explicit SampleView (dl::SampleEngine& e) : eng (e) {}

    std::function<void()> onSelect;

    void refresh()
    {
        auto d = eng.getData();
        const bool audioChanged = (d ? d->audio.get() : nullptr) != (data ? data->audio.get() : nullptr);
        data = d;
        if (audioChanged) rebuildPeaks();

        bool playing = false;
        for (auto& u : eng.uiPos) playing = playing || u.load() >= 0;
        const int sel = eng.selected.load();

        if (audioChanged || playing || wasPlaying || sel != lastSel || d.get() != lastData)
        {
            wasPlaying = playing;
            lastSel = sel;
            lastData = d.get();
            repaint();
        }
    }

    void resized() override { rebuildPeaks(); }

    void paint (juce::Graphics& g) override
    {
        using namespace theme;
        const auto r = getLocalBounds().toFloat();
        g.setColour (black);
        g.fillRoundedRectangle (r, 3.0f);

        if (! data || data->length() == 0)
        {
            g.setColour (lightGray.withAlpha (0.6f));
            g.setFont (mono (13.0f));
            g.drawText ("Glisse un fichier audio ici (wav, aiff, flac, ogg) ou clique sur IMPORT", getLocalBounds(), juce::Justification::centred);
            g.setColour (purple.withAlpha (0.7f));
            g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            return;
        }

        const int w = getWidth(), h = getHeight();
        const float cy = (float) h * 0.5f, amp = (float) h * 0.42f;
        const int sel = eng.selected.load();

        for (int i = 0; i < data->numSlices(); ++i)
        {
            const float x0 = posToX (data->sliceStart (i)), x1 = posToX (data->sliceEnd (i));
            g.setColour (i == sel ? purple.withAlpha (0.38f) : purple.withAlpha ((i & 1) ? 0.07f : 0.15f));
            g.fillRect (x0, 0.0f, x1 - x0, (float) h);
            if (x1 - x0 > 16.0f)
            {
                g.setFont (mono (10.0f, i == sel));
                g.setColour (i == sel ? pink : lightGray.withAlpha (0.7f));
                g.drawText (juce::String (i + 1), juce::Rectangle<float> (x0 + 3.0f, 2.0f, 24.0f, 12.0f), juce::Justification::topLeft);
            }
        }

        g.setColour (lightGray.withAlpha (0.12f));
        g.drawHorizontalLine ((int) cy, 0.0f, (float) w);

        g.setColour (pink.withAlpha (0.9f));
        for (int x = 0; x < (int) pmin.size() && x < w; ++x)
            g.drawVerticalLine (x, cy - pmax[(size_t) x] * amp, cy - pmin[(size_t) x] * amp + 1.0f);

        for (int i = 1; i < data->numSlices(); ++i)
        {
            const float x = posToX (data->sliceStart (i));
            g.setColour (lightGray.withAlpha (0.85f));
            g.drawVerticalLine ((int) x, 0.0f, (float) h);
            juce::Path tri;
            tri.addTriangle (x - 5.0f, 0.0f, x + 5.0f, 0.0f, x, 8.0f);
            g.fillPath (tri);
        }

        g.setColour (green);
        for (auto& u : eng.uiPos)
        {
            const int p = u.load();
            if (p >= 0) g.drawVerticalLine ((int) posToX (p), 0.0f, (float) h);
        }

        g.setColour (purple.withAlpha (0.7f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! data || data->length() == 0) return;
        const int hit = hitMarker (e.x);

        if (e.mods.isPopupMenu() || e.mods.isCtrlDown())
        {
            if (hit > 0) eng.removeMarker (hit);
            return;
        }

        if (hit > 0)
        {
            dragging = hit;
            return;
        }

        eng.selected = sliceAt (xToPos (e.x));
        if (onSelect) onSelect();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging > 0) eng.moveMarker (dragging, xToPos (e.x));
    }

    void mouseUp (const juce::MouseEvent&) override { dragging = -1; }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (! data || data->length() == 0 || hitMarker (e.x) > 0) return;
        eng.insertMarker (xToPos (e.x));
    }

private:
    int xToPos (int x) const { return (int) ((double) juce::jlimit (0, getWidth(), x) / (double) std::max (1, getWidth()) * data->length()); }
    float posToX (int p) const { return (float) ((double) p / (double) std::max (1, data->length()) * (double) getWidth()); }

    int hitMarker (int x) const
    {
        for (int i = 1; i < data->numSlices(); ++i)
            if (std::abs (posToX (data->sliceStart (i)) - (float) x) <= 5.0f) return i;
        return -1;
    }

    int sliceAt (int pos) const
    {
        for (int i = data->numSlices() - 1; i >= 0; --i)
            if (pos >= data->sliceStart (i)) return i;
        return 0;
    }

    void rebuildPeaks()
    {
        const int w = std::max (1, getWidth());
        pmin.assign ((size_t) w, 0.0f);
        pmax.assign ((size_t) w, 0.0f);
        if (! data || data->length() == 0) return;

        const auto& a = *data->audio;
        const int len = a.getNumSamples();
        for (int x = 0; x < w; ++x)
        {
            const int s0 = (int) ((juce::int64) len * x / w), s1 = std::max (s0 + 1, (int) ((juce::int64) len * (x + 1) / w));
            const int step = std::max (1, (s1 - s0) / 128);
            float mn = 0.0f, mx = 0.0f;
            for (int s = s0; s < s1 && s < len; s += step)
            {
                float v = a.getSample (0, s);
                if (a.getNumChannels() > 1) v = 0.5f * (v + a.getSample (1, s));
                mn = std::min (mn, v);
                mx = std::max (mx, v);
            }
            pmin[(size_t) x] = mn;
            pmax[(size_t) x] = mx;
        }
    }

    dl::SampleEngine& eng;
    std::shared_ptr<const dl::SampleData> data;
    const dl::SampleData* lastData = nullptr;
    std::vector<float> pmin, pmax;
    int dragging = -1, lastSel = -1;
    bool wasPlaying = false;
};
