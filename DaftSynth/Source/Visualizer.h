#pragma once
#include <array>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "Theme.h"

//==============================================================================
// Bandeau animé : soleil rétro + grille en perspective + spectre + oscilloscope
//==============================================================================
class VisualizerComponent : public juce::Component, private juce::Timer
{
public:
    explicit VisualizerComponent (DaftSynthProcessor& p) : proc (p)
    {
        setOpaque (true);
        startTimerHz (30);
    }

    ~VisualizerComponent() override { stopTimer(); }

    float getEnergy() const { return energy; }
    float getGlitch() const { return glitch; }

    void paint (juce::Graphics& g) override
    {
        using namespace theme;
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth(), h = bounds.getHeight();
        const float cx = w * 0.5f;
        const float horizon = h * 0.56f;

        // Ciel (noir -> violet sombre) et sol
        g.fillAll (black);
        g.setGradientFill (juce::ColourGradient (black, 0.0f, 0.0f,
                                                 juce::Colour (0xff1c0a36).withMultipliedBrightness (0.7f + energy * 0.6f),
                                                 0.0f, horizon, false));
        g.fillRect (0.0f, 0.0f, w, horizon);

        // Soleil rétro : halo, disque dégradé rose -> violet, bandes noires
        const float sunR = 22.0f + 12.0f * bass + 4.0f * energy;
        const float sunY = horizon - sunR * 0.45f;
        for (int k = 3; k >= 1; --k)
        {
            g.setColour (pink.withAlpha (0.04f + 0.05f * bass));
            const float rr = sunR + (float) k * 6.0f;
            g.fillEllipse (cx - rr, sunY - rr, rr * 2.0f, rr * 2.0f);
        }
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<int> (0, 0, (int) w, (int) horizon));
            g.setGradientFill (juce::ColourGradient (pink, cx, sunY - sunR, purple, cx, sunY + sunR, false));
            g.fillEllipse (cx - sunR, sunY - sunR, sunR * 2.0f, sunR * 2.0f);
            g.setColour (black.withAlpha (0.92f));
            for (int i = 0; i < 5; ++i)
            {
                const float yy = sunY + sunR * (0.05f + 0.19f * (float) i);
                g.fillRect (cx - sunR, yy, sunR * 2.0f, 1.0f + 0.8f * (float) i);
            }
        }

        // Grille en perspective qui défile (plus vite quand le son est fort)
        const float fh = h - horizon;
        const auto gridCol = purple.interpolatedWith (pink, juce::jlimit (0.0f, 1.0f, energy * 1.5f));
        for (int i = 0; i < 9; ++i)
        {
            const float t = std::fmod (((float) i + gridOffset) / 9.0f, 1.0f);
            const float yy = horizon + fh * t * t;
            g.setColour (gridCol.withAlpha (0.12f + 0.55f * t));
            g.drawHorizontalLine ((int) yy, 0.0f, w);
        }
        for (int k = -16; k <= 16; ++k)
        {
            g.setColour (gridCol.withAlpha (0.28f));
            g.drawLine (cx + (float) k * 4.0f, horizon, cx + (float) k * (w / 18.0f), h, 1.0f);
        }
        g.setColour (pink.withAlpha (0.35f + 0.5f * energy));
        g.drawHorizontalLine ((int) horizon, 0.0f, w);

        // Spectre (barres violet -> rose, repères gris clair)
        const float sx0 = 14.0f, sx1 = cx - 78.0f;
        const float barArea = sx1 - sx0;
        const float barW = barArea / (float) kBands;
        const float floorY = h - 7.0f, maxH = h - 22.0f;
        g.setGradientFill (juce::ColourGradient (pink, 0.0f, floorY - maxH, purple, 0.0f, floorY, false));
        for (int b = 0; b < kBands; ++b)
        {
            const float bh = std::max (1.5f, bands[(size_t) b] * maxH);
            g.fillRect (sx0 + (float) b * barW + 1.0f, floorY - bh, barW - 2.0f, bh);
        }
        g.setColour (lightGray.withAlpha (0.85f));
        for (int b = 0; b < kBands; ++b)
        {
            const float py = floorY - peaks[(size_t) b] * maxH - 3.0f;
            g.fillRect (sx0 + (float) b * barW + 1.0f, py, barW - 2.0f, 1.5f);
        }

        // Oscilloscope (vert fluo, avec halo)
        const float ox0 = cx + 78.0f, ox1 = w - 14.0f;
        const float oy = h * 0.5f, amp = h * 0.36f;
        g.setColour (lightGray.withAlpha (0.12f));
        g.drawHorizontalLine ((int) oy, ox0, ox1);
        {
            juce::Path trace;
            const int pts = std::max (2, (int) (ox1 - ox0));
            for (int i = 0; i < pts; ++i)
            {
                const int si = std::min (scopeStart + (i * kScopeWin) / pts, kN - 1);
                const float v = juce::jlimit (-1.0f, 1.0f, scope[(size_t) si] * 1.8f);
                const float px = ox0 + (float) i;
                const float py = oy - v * amp;
                if (i == 0) trace.startNewSubPath (px, py);
                else trace.lineTo (px, py);
            }
            g.setColour (green.withAlpha (0.10f));
            g.strokePath (trace, juce::PathStrokeType (6.0f));
            g.setColour (green.withAlpha (0.25f));
            g.strokePath (trace, juce::PathStrokeType (3.0f));
            g.setColour (green);
            g.strokePath (trace, juce::PathStrokeType (1.2f));
        }

        // Vumètres centraux (segments violet -> rose)
        for (int ch = 0; ch < 2; ++ch)
        {
            const float lvl = ch == 0 ? meterL : meterR;
            const int lit = (int) std::round (juce::jlimit (0.0f, 1.0f, lvl) * 14.0f);
            for (int s = 0; s < 14; ++s)
            {
                const float sx = cx - 56.0f + (float) s * 8.0f;
                const float sy = h - 15.0f + (float) ch * 6.0f;
                g.setColour (s < lit ? purple.interpolatedWith (pink, (float) s / 13.0f)
                                     : darkGray2);
                g.fillRect (sx, sy, 6.0f, 3.0f);
            }
        }

        // Étiquettes et voyant de signal
        g.setFont (mono (9.0f));
        g.setColour (lightGray.withAlpha (0.55f));
        g.drawText ("SPECTRUM", (int) sx0 + 2, 3, 80, 12, juce::Justification::centredLeft);
        g.drawText ("SCOPE", (int) ox0 + 2, 3, 80, 12, juce::Justification::centredLeft);
        g.drawText ("SIG", (int) (cx - 8.0f), 3, 30, 12, juce::Justification::centredLeft);
        g.setColour (green.withAlpha (0.2f + 0.8f * juce::jmin (1.0f, energy * 3.0f)));
        g.fillEllipse (cx - 16.0f, 6.0f, 5.0f, 5.0f);

        // Cadre à angles coupés
        const auto r = bounds.reduced (1.0f);
        const float c = 10.0f;
        juce::Path frame;
        frame.startNewSubPath (r.getX(), r.getY());
        frame.lineTo (r.getRight() - c, r.getY());
        frame.lineTo (r.getRight(), r.getY() + c);
        frame.lineTo (r.getRight(), r.getBottom());
        frame.lineTo (r.getX() + c, r.getBottom());
        frame.lineTo (r.getX(), r.getBottom() - c);
        frame.closeSubPath();
        g.setColour (purple.withAlpha (0.35f + 0.5f * energy));
        g.strokePath (frame, juce::PathStrokeType (1.4f));
        g.setColour (pink);
        g.drawLine (r.getX(), r.getY(), r.getX() + 16.0f, r.getY(), 2.0f);
        g.drawLine (r.getRight(), r.getBottom(), r.getRight() - 16.0f, r.getBottom(), 2.0f);
    }

private:
    static constexpr int kN = 2048;
    static constexpr int kBands = 44;
    static constexpr int kScopeWin = 600;

    void timerCallback() override
    {
        const uint32_t bc = proc.getBlockCounter();
        idle = (bc == lastBlock) ? idle + 1 : 0;
        lastBlock = bc;
        const bool silent = idle > 8;

        if (silent) scope.fill (0.0f);
        else proc.copyScope (scope.data(), kN);

        // Déclenchement sur passage par zéro montant pour stabiliser l'oscilloscope
        scopeStart = 0;
        for (int i = 0; i < kN / 2 - 1; ++i)
            if (scope[(size_t) i] <= 0.0f && scope[(size_t) i + 1] > 0.0f) { scopeStart = i; break; }

        // Spectre
        std::copy (scope.begin(), scope.end(), fftData.begin());
        std::fill (fftData.begin() + kN, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        float sr = (float) proc.getSampleRate();
        if (sr <= 0.0f) sr = 44100.0f;
        const float binHz = sr / (float) kN;
        const float fMin = 40.0f, fMax = std::min (16000.0f, sr * 0.45f);

        for (int b = 0; b < kBands; ++b)
        {
            const float f0 = fMin * std::pow (fMax / fMin, (float) b / (float) kBands);
            const float f1 = fMin * std::pow (fMax / fMin, (float) (b + 1) / (float) kBands);
            const int i0 = std::max (1, (int) (f0 / binHz));
            const int i1 = std::max (i0 + 1, std::min (kN / 2 - 1, (int) (f1 / binHz) + 1));
            float mag = 0.0f;
            for (int i = i0; i < i1; ++i) mag = std::max (mag, fftData[(size_t) i]);

            const float db = juce::Decibels::gainToDecibels (mag / 512.0f, -100.0f);
            float v = juce::jmap (db, -75.0f, -8.0f, 0.0f, 1.0f);
            v = juce::jlimit (0.0f, 1.0f, v + 0.12f * (float) b / (float) kBands * (v > 0.02f ? 1.0f : 0.0f));

            bands[(size_t) b] = std::max (v, bands[(size_t) b] * 0.88f);
            peaks[(size_t) b] = std::max (bands[(size_t) b], peaks[(size_t) b] - 0.012f);
        }

        float lowSum = 0.0f;
        for (int b = 0; b < 6; ++b) lowSum += bands[(size_t) b];
        bass += (lowSum / 6.0f - bass) * 0.4f;

        // Énergie globale, transitoires (glitch) et vumètres
        const float target = silent ? 0.0f : juce::jlimit (0.0f, 1.0f, proc.getRms() * 4.0f);
        energy += (target - energy) * (target > energy ? 0.5f : 0.12f);
        glitch *= 0.8f;
        if (target - lastTarget > 0.1f) glitch = 1.0f;
        lastTarget = target;

        meterL = std::max (silent ? 0.0f : proc.getPeakL(), meterL * 0.88f);
        meterR = std::max (silent ? 0.0f : proc.getPeakR(), meterR * 0.88f);

        gridOffset = std::fmod (gridOffset + 0.012f + 0.09f * energy, 9.0f);

        repaint();
    }

    DaftSynthProcessor& proc;
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { (size_t) kN, juce::dsp::WindowingFunction<float>::hann, false };

    std::array<float, kN> scope {};
    std::array<float, 2 * kN> fftData {};
    std::array<float, kBands> bands {}, peaks {};

    float energy = 0.0f, glitch = 0.0f, bass = 0.0f, gridOffset = 0.0f, lastTarget = 0.0f;
    float meterL = 0.0f, meterR = 0.0f;
    int scopeStart = 0, idle = 0;
    uint32_t lastBlock = 0;
};

//==============================================================================
// Logo avec effet glitch (aberration chromatique rose / violet sur les transitoires)
//==============================================================================
class LogoComponent : public juce::Component
{
public:
    void update (float newEnergy, float newGlitch)
    {
        energy = newEnergy;
        glitch = newGlitch;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace theme;
        const auto area = getLocalBounds();
        const juce::String text = "DAFTSYNTH";
        g.setFont (mono (22.0f, true));

        const int dx = 1 + (int) (glitch * 4.0f);
        g.setColour (purple.withAlpha (0.9f));
        g.drawText (text, area.translated (-dx, 0), juce::Justification::centredLeft);
        g.setColour (pink.withAlpha (0.9f));
        g.drawText (text, area.translated (dx, 0), juce::Justification::centredLeft);
        g.setColour (lightGray);
        g.drawText (text, area, juce::Justification::centredLeft);

        // Tranche décalée aléatoirement pendant un glitch
        if (glitch > 0.3f)
        {
            juce::Graphics::ScopedSaveState ss (g);
            const int bandY = rng.nextInt (juce::jmax (1, area.getHeight() - 8));
            g.reduceClipRegion (juce::Rectangle<int> (0, bandY, area.getWidth(), 4 + rng.nextInt (6)));
            g.setColour (pink);
            g.drawText (text, area.translated (rng.nextInt (13) - 6, 0), juce::Justification::centredLeft);
        }

        g.setFont (mono (9.0f));
        g.setColour (lightGray.withAlpha (0.55f));
        g.drawText ("v0.1", area.withTrimmedLeft (124).withTrimmedBottom (12), juce::Justification::bottomLeft);
    }

private:
    float energy = 0.0f, glitch = 0.0f;
    juce::Random rng;
};

//==============================================================================
// Scanlines CRT + vignette (dessinées une fois, mises en cache)
//==============================================================================
class ScanlineOverlay : public juce::Component
{
public:
    ScanlineOverlay()
    {
        setInterceptsMouseClicks (false, false);
        setBufferedToImage (true);
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        for (int y = 0; y < getHeight(); y += 3)
            g.fillRect (0.0f, (float) y, b.getWidth(), 1.0f);

        juce::ColourGradient vignette (juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(),
                                       juce::Colours::black.withAlpha (0.45f), 0.0f, 0.0f, true);
        g.setGradientFill (vignette);
        g.fillRect (b);
    }
};
