#pragma once
#include "DSP.h"

namespace dl
{
// Chorus type Juno-106 : modes I, II et I+II, canaux gauche/droite en opposition de phase
class Chorus
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        const int size = (int) (sr * 0.03f) + 8;
        line[0].init (size);
        line[1].init (size);
        phase = 0.0f;
    }

    // mode : 0 = off, 1 = I, 2 = II, 3 = I+II
    void process (float& l, float& r, int mode)
    {
        if (mode <= 0) return;

        static const float rates[4]  = { 0.0f, 0.513f, 0.863f, 9.75f };
        static const float depths[4] = { 0.0f, 1.2f, 1.8f, 0.45f }; // ms

        phase += rates[mode] / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        const float tri = 1.0f - 4.0f * std::fabs (phase - 0.5f);
        const float base = 3.4f;
        const float dL = (base + depths[mode] * tri) * sr * 0.001f;
        const float dR = (base - depths[mode] * tri) * sr * 0.001f;

        const float wl = line[0].read (dL);
        const float wr = line[1].read (dR);
        line[0].write (l);
        line[1].write (r);

        l = 0.7f * l + 0.7f * wl;
        r = 0.7f * r + 0.7f * wr;
    }

private:
    float sr = 44100.0f, phase = 0.0f;
    ModDelay line[2];
};

// Phaser à 6 étages, LFO en quadrature entre les deux canaux
class Phaser
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        phase = 0.0f;
        fbState[0] = fbState[1] = 0.0f;
        for (auto& ch : ap)
            for (auto& s : ch) s = {};
    }

    void process (float& l, float& r, float mix, float rate, float depth, float fb)
    {
        if (mix < 0.001f) return;

        phase += rate / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        const float lfoL = 0.5f + 0.5f * std::sin (kTwoPi * phase);
        const float lfoR = 0.5f + 0.5f * std::sin (kTwoPi * (phase + 0.25f));
        l = run (0, l, lfoL, depth, fb, mix);
        r = run (1, r, lfoR, depth, fb, mix);
    }

private:
    float run (int ch, float x, float lfo, float depth, float fb, float mix)
    {
        const float f = std::min (200.0f * std::exp2 (5.0f * depth * lfo), sr * 0.45f);
        const float w = std::tan (kPi * f / sr);
        const float a = (1.0f - w) / (1.0f + w);

        float y = x + fbState[ch] * fb;
        for (int s = 0; s < 6; ++s) y = ap[ch][s].process (y, a);
        fbState[ch] = std::clamp (y, -4.0f, 4.0f);

        return x * (1.0f - 0.5f * mix) + y * 0.5f * mix;
    }

    float sr = 44100.0f, phase = 0.0f;
    float fbState[2] = { 0, 0 };
    AllPass ap[2][6];
};

// Delay stéréo avec filtre passe-bas dans la boucle (écho sombre type analogique)
class StereoDelay
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        size = (int) (sr * 2.1f);
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        w = 0;
        smTime = 0.3f * sr;
        lpL = lpR = 0.0f;
    }

    void process (float& l, float& r, float mix, float timeMs, float fb)
    {
        const float target = std::clamp (timeMs * 0.001f * sr, 8.0f, (float) size - 4.0f);
        smTime += (target - smTime) * 0.0005f;

        const float echoL = readLine (bufL, smTime);
        const float echoR = readLine (bufR, smTime * 1.02f);

        lpL += 0.45f * (echoL - lpL);
        lpR += 0.45f * (echoR - lpR);

        bufL[(size_t) w] = l + lpL * fb;
        bufR[(size_t) w] = r + lpR * fb;
        if (++w >= size) w = 0;

        l += echoL * mix;
        r += echoR * mix;
    }

private:
    float readLine (const std::vector<float>& b, float d) const
    {
        float rp = (float) w - d;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp % size;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - std::floor (rp);
        return b[(size_t) i0] * (1.0f - fr) + b[(size_t) i1] * fr;
    }

    float sr = 44100.0f, smTime = 13000.0f, lpL = 0.0f, lpR = 0.0f;
    int size = 1, w = 0;
    std::vector<float> bufL, bufR;
};

// Réduction de résolution et de fréquence d'échantillonnage (grain type SP-1200 : 12 bits / 26 kHz)
class Crusher
{
public:
    void process (float& l, float& r, float bits, float rate, float sr)
    {
        const bool bitsOn = bits < 15.5f;
        const bool rateOn = rate < sr * 0.99f;
        if (! bitsOn && ! rateOn) return;

        ph += rate / sr;
        if (ph >= 1.0f)
        {
            ph -= 1.0f;
            if (ph >= 1.0f) ph = 0.0f;
            hl = l;
            hr = r;
        }
        l = hl;
        r = hr;

        if (bitsOn)
        {
            const float lv = std::exp2 (bits - 1.0f);
            l = std::round (l * lv) / lv;
            r = std::round (r * lv) / lv;
        }
    }

private:
    float ph = 0.0f, hl = 0.0f, hr = 0.0f;
};

// Distorsion douce (saturation tanh)
inline float distort (float x, float amount)
{
    if (amount < 0.001f) return x;
    const float drive = 1.0f + amount * 15.0f;
    return std::tanh (drive * x) * (1.2f / std::sqrt (drive));
}
// Distorsion / saturation complète
struct DistParams
{
    int type = 0;        // 0 Soft, 1 Tube, 2 Hard Clip, 3 Wavefold, 4 Fuzz, 5 Rectify, 6 Tape
    float amount = 0.0f; // drive 0..1
    float bias = 0.0f;   // -1..1 (asymétrie, harmoniques paires)
    float tone = 1.0f;   // 0..1 (filtre passe-bas après la distorsion)
    float mix = 1.0f;    // 0..1
    float outDb = 0.0f;  // gain de sortie en dB
};

class Distortion
{
public:
    void process (float& l, float& r, const DistParams& p, float sr)
    {
        if (p.amount < 0.001f) return;

        const float g = 1.0f + p.amount * 15.0f;
        const float outGain = std::pow (10.0f, p.outDb / 20.0f);
        const float lpCoef = p.tone >= 0.999f ? 0.0f : std::exp (-kTwoPi * (200.0f * std::pow (100.0f, p.tone)) / sr);

        l = channel (0, l, p, g, outGain, lpCoef);
        r = channel (1, r, p, g, outGain, lpCoef);
    }

private:
    float shape (float x, const DistParams& p, float g, int ch)
    {
        const float k = 1.2f / std::sqrt (g);
        switch (p.type)
        {
            case 1: // Tube : asymétrique
            {
                const float b = p.bias * 0.8f + 0.25f;
                return (std::tanh (g * x + b) - std::tanh (b)) * k;
            }
            case 2: // Hard clip
                return std::clamp (g * x + p.bias * 0.5f, -1.0f, 1.0f) * 0.7f;
            case 3: // Wavefold
                return std::sin (g * x * 1.5707963f + p.bias) * 0.7f;
            case 4: // Fuzz
            {
                const float u = g * x + p.bias * 0.5f;
                return (u >= 0.0f ? 1.0f : -1.0f) * (1.0f - std::exp (-std::fabs (u))) * 0.8f;
            }
            case 5: // Rectify
            {
                const float v = std::tanh (g * x + p.bias * 0.3f);
                return (v * 0.5f + std::fabs (v) * 0.7f) * 0.8f;
            }
            case 6: // Tape : saturation douce + adoucissement des aigus
            {
                const float v = std::tanh (g * x * 0.7f + p.bias * 0.3f) * k * 1.1f;
                tapeLp[ch] += 0.55f * (v - tapeLp[ch]);
                return tapeLp[ch];
            }
            default: // Soft
                return std::tanh (g * x + p.bias * 0.5f) * k;
        }
    }

    float channel (int ch, float x, const DistParams& p, float g, float outGain, float lpCoef)
    {
        float y = shape (x, p, g, ch);

        // blocage du continu
        const float hp = y - dcX[ch] + 0.995f * dcY[ch];
        dcX[ch] = y;
        dcY[ch] = hp;
        y = hp;

        if (lpCoef > 0.0f)
        {
            toneLp[ch] = (1.0f - lpCoef) * y + lpCoef * toneLp[ch];
            y = toneLp[ch];
        }

        return (x * (1.0f - p.mix) + y * p.mix) * outGain;
    }

    float dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 }, toneLp[2] = { 0, 0 }, tapeLp[2] = { 0, 0 };
};
} // namespace dl
