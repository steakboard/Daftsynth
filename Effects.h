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
    void process (float& l, float& r, int mode, float mix = 1.0f)
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

        const float dry = 1.0f - 0.3f * mix, wet = 0.7f * mix;
        l = dry * l + wet * wl;
        r = dry * r + wet * wr;
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

    void process (float& l, float& r, float mix, float rate, float depth, float fb, int stages = 6)
    {
        if (mix < 0.001f) return;
        nStages = stages;

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
        for (int s = 0; s < nStages; ++s) y = ap[ch][s].process (y, a);
        fbState[ch] = std::clamp (y, -4.0f, 4.0f);

        return x * (1.0f - 0.5f * mix) + y * 0.5f * mix;
    }

    float sr = 44100.0f, phase = 0.0f;
    float fbState[2] = { 0, 0 };
    AllPass ap[2][8];
    int nStages = 6;
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

    void process (float& l, float& r, float mix, float timeMs, float fb, bool ping = false, float toneHz = 4000.0f)
    {
        const float target = std::clamp (timeMs * 0.001f * sr, 8.0f, (float) size - 4.0f);
        smTime += (target - smTime) * 0.0005f;

        const float echoL = readLine (bufL, smTime);
        const float echoR = readLine (bufR, smTime * 1.02f);

        const float tc = 1.0f - std::exp (-kTwoPi * std::clamp (toneHz, 200.0f, 18000.0f) / sr);
        lpL += tc * (echoL - lpL);
        lpR += tc * (echoR - lpR);

        if (ping)
        {
            // ping-pong : l'entrée part à gauche, les répétitions rebondissent de gauche à droite
            bufL[(size_t) w] = 0.5f * (l + r) + lpR * fb;
            bufR[(size_t) w] = lpL * fb;
        }
        else
        {
            bufL[(size_t) w] = l + lpL * fb;
            bufR[(size_t) w] = r + lpR * fb;
        }
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
// Module WOW : filtre multi-modes, filtre de voyelles (formants), drive et wobble
struct WowParams
{
    int type = 0;        // 0 LP12, 1 LP24, 2 HP12, 3 BP, 4 Notch, 5 Peak, 6 Comb, 7 Vowel
    float freq = 1000.0f;
    float res = 0.3f;
    float vowel = 0.0f;  // 0..1 : A E I O U
    float drive = 0.0f;
    float mix = 0.0f;
    float lfo = 0.0f;    // valeur courante du wobble (-1..1)
    float depth = 0.0f;  // profondeur du wobble
};

class WowFilter
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& c : ch)
        {
            c = Ch {};
            c.comb.init ((int) (sr * 0.03f) + 4);
        }
    }

    void process (float& l, float& r, const WowParams& p)
    {
        if (p.mix < 0.001f) return;
        l = chan (0, l, p);
        r = chan (1, r, p);
    }

private:
    struct Ch
    {
        SVF a, b, f1, f2, f3;
        ModDelay comb;
    };

    float chan (int i, float in, const WowParams& p)
    {
        auto& c = ch[i];
        const float g = 1.0f + p.drive * 6.0f;
        const float x = p.drive > 0.001f ? std::tanh (in * g) * (1.2f / std::sqrt (g)) : in;

        const float q = 0.6f + p.res * 14.0f;
        const float k = 1.0f / q;
        const float maxF = sr * 0.45f;
        const float f = std::clamp (p.freq * std::exp2 (p.depth * 4.0f * p.lfo), 20.0f, maxF);
        float y = x;

        switch (p.type)
        {
            case 0: c.a.set (f, q, sr); y = c.a.process (x).lp; break;
            case 1:
                c.a.set (f, q, sr);
                c.b.set (f, 0.707f, sr);
                y = c.b.process (c.a.process (x).lp).lp;
                break;
            case 2: c.a.set (f, q, sr); y = c.a.process (x).hp; break;
            case 3: c.a.set (f, q, sr); y = c.a.process (x).bp * k * 1.5f; break;
            case 4: { c.a.set (f, q, sr); const auto o = c.a.process (x); y = o.lp + o.hp; break; }
            case 5: { c.a.set (f, q, sr); y = x + c.a.process (x).bp * k * p.res * 8.0f; break; }
            case 6: // comb
            {
                const float d = std::clamp (sr / f, 4.0f, sr * 0.025f);
                const float wet = c.comb.read (d);
                y = x + wet * p.res * 0.92f;
                c.comb.write (std::clamp (y, -4.0f, 4.0f));
                y *= 0.6f;
                break;
            }
            default: // voyelles
            {
                static const float F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                               { 570, 840, 2410 }, { 300, 870, 2240 } };
                const float v = std::clamp (p.vowel + p.depth * 0.5f * p.lfo, 0.0f, 1.0f) * 4.0f;
                const int i0 = std::min (3, (int) v);
                const float fr = v - (float) i0;
                const float shift = std::clamp (p.freq / 1000.0f, 0.3f, 3.0f);
                const float fq = 4.0f + p.res * 14.0f;
                float f[3];
                for (int n = 0; n < 3; ++n)
                    f[n] = (F[i0][n] + (F[i0 + 1][n] - F[i0][n]) * fr) * shift;

                c.f1.set (f[0], fq, sr);
                c.f2.set (f[1], fq, sr);
                c.f3.set (f[2], fq, sr);
                const float kk = 1.0f / fq;
                y = (c.f1.process (x).bp + 0.7f * c.f2.process (x).bp + 0.4f * c.f3.process (x).bp) * kk * 2.5f;
                break;
            }
        }

        return in * (1.0f - p.mix) + y * p.mix;
    }

    float sr = 44100.0f;
    Ch ch[2];
};
// Flanger stéréo (LFO en quadrature, feedback positif ou négatif)
class Flanger
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        const int size = (int) (sr * 0.025f) + 8;
        line[0].init (size);
        line[1].init (size);
        phase = 0.0f;
    }

    void process (float& l, float& r, float mix, float rate, float depth, float fb, float baseMs)
    {
        if (mix < 0.001f) return;

        phase += rate / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        const float lfoL = std::sin (kTwoPi * phase), lfoR = std::sin (kTwoPi * (phase + 0.25f));
        const float dL = std::max (1.0f, baseMs * (1.0f + 0.9f * depth * lfoL) * sr * 0.001f);
        const float dR = std::max (1.0f, baseMs * (1.0f + 0.9f * depth * lfoR) * sr * 0.001f);

        const float wl = line[0].read (dL), wr = line[1].read (dR);
        line[0].write (std::clamp (l + wl * fb, -4.0f, 4.0f));
        line[1].write (std::clamp (r + wr * fb, -4.0f, 4.0f));

        const float dry = 1.0f - 0.3f * mix, wet = 0.7f * mix;
        l = dry * l + wet * wl;
        r = dry * r + wet * wr;
    }

private:
    float sr = 44100.0f, phase = 0.0f;
    ModDelay line[2];
};

// Filtre biquad (formules RBJ) pour l'égaliseur
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setCoefs (float B0, float B1, float B2, float A0, float A1, float A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    void lowShelf (float f, float gDb, float sr)
    {
        const float A = std::pow (10.0f, gDb / 40.0f), w0 = kTwoPi * std::min (f, sr * 0.45f) / sr;
        const float c = std::cos (w0), s = std::sin (w0), al = s * 0.70710678f, sq = 2.0f * std::sqrt (A) * al;
        setCoefs (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                  (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }

    void highShelf (float f, float gDb, float sr)
    {
        const float A = std::pow (10.0f, gDb / 40.0f), w0 = kTwoPi * std::min (f, sr * 0.45f) / sr;
        const float c = std::cos (w0), s = std::sin (w0), al = s * 0.70710678f, sq = 2.0f * std::sqrt (A) * al;
        setCoefs (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                  (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }

    void peak (float f, float gDb, float q, float sr)
    {
        const float A = std::pow (10.0f, gDb / 40.0f), w0 = kTwoPi * std::min (f, sr * 0.45f) / sr;
        const float c = std::cos (w0), al = std::sin (w0) / (2.0f * q);
        setCoefs (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
};

// Égaliseur 3 bandes : shelf grave, cloche médium, shelf aigu
struct EqParams
{
    float lowG = 0, lowF = 120, midG = 0, midF = 1000, midQ = 1, highG = 0, highF = 6000;
};

class Equalizer
{
public:
    void set (const EqParams& p, float sr)
    {
        active = std::fabs (p.lowG) > 0.05f || std::fabs (p.midG) > 0.05f || std::fabs (p.highG) > 0.05f;
        if (! active) return;
        for (auto& c : ch)
        {
            c.lo.lowShelf (p.lowF, p.lowG, sr);
            c.mid.peak (p.midF, p.midG, p.midQ, sr);
            c.hi.highShelf (p.highF, p.highG, sr);
        }
    }

    void process (float& l, float& r)
    {
        if (! active) return;
        l = ch[0].hi.process (ch[0].mid.process (ch[0].lo.process (l)));
        r = ch[1].hi.process (ch[1].mid.process (ch[1].lo.process (r)));
    }

private:
    struct Ch { Biquad lo, mid, hi; };
    Ch ch[2];
    bool active = false;
};

// Compresseur multibande 3 bandes "à la OTT" : compression vers le bas ET vers le haut (son dense et agressif)
struct OttParams
{
    float mix = 0, depth = 0.7f, time = 1.0f, lowDb = 0, midDb = 0, highDb = 0;
};

class MultiBand
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& c : ch) c = Ch {};
        for (auto& b : band) b = Band {};
    }

    void process (float& l, float& r, const OttParams& p)
    {
        if (p.mix < 0.001f) return;

        float lo[2], mi[2], hi[2];
        const float in[2] = { l, r };
        for (int i = 0; i < 2; ++i)
        {
            lo[i] = ch[i].x1.lp (in[i]);
            hi[i] = ch[i].x2.hp (in[i]);
            mi[i] = in[i] - lo[i] - hi[i];
        }

        const float atk = 1.0f - std::exp (-1.0f / (0.003f * p.time * sr));
        const float rel = 1.0f - std::exp (-1.0f / (0.10f * p.time * sr));
        const float sm = 1.0f - std::exp (-1.0f / (0.002f * sr));
        const float bandGain[3] = { p.lowDb, p.midDb, p.highDb };
        const float* src[3] = { lo, mi, hi };

        float out[2] = { 0.0f, 0.0f };
        for (int b = 0; b < 3; ++b)
        {
            auto& bd = band[b];
            const float level = std::max (std::fabs (src[b][0]), std::fabs (src[b][1]));
            bd.env += (level - bd.env) * (level > bd.env ? atk : rel);

            const float db = 20.0f * std::log10 (std::max (bd.env, 1.0e-5f));
            float gain = 0.0f;
            if (db > -22.0f) gain -= (db + 22.0f) * 0.75f;            // vers le bas, rapport 4:1 au-dessus de -22 dB
            if (db < -42.0f) gain += std::min (24.0f, (-42.0f - db) * 0.66f);   // vers le haut, rapport 3:1 sous -42 dB
            gain = gain * p.depth + bandGain[b];

            bd.g += (std::pow (10.0f, gain / 20.0f) - bd.g) * sm;
            out[0] += src[b][0] * bd.g;
            out[1] += src[b][1] * bd.g;
        }

        l = in[0] * (1.0f - p.mix) + out[0] * p.mix;
        r = in[1] * (1.0f - p.mix) + out[1] * p.mix;
    }

    void setCrossovers (float f1, float f2)
    {
        for (auto& c : ch)
        {
            c.x1.set (f1, sr);
            c.x2.set (f2, sr);
        }
    }

private:
    struct Xover
    {
        SVF lpA, lpB, hpA, hpB;
        void set (float f, float sr)
        {
            lpA.set (f, 0.7071f, sr); lpB.set (f, 0.7071f, sr);
            hpA.set (f, 0.7071f, sr); hpB.set (f, 0.7071f, sr);
        }
        float lp (float x) { return lpB.process (lpA.process (x).lp).lp; }
        float hp (float x) { return hpB.process (hpA.process (x).hp).hp; }
    };
    struct Ch { Xover x1, x2; };
    struct Band { float env = 0.0f, g = 1.0f; };

    float sr = 44100.0f;
    Ch ch[2];
    Band band[3];
};

// Limiteur de sortie avec gain d'entrée (boost) : "son boosté" sans dépasser 0 dB
class Limiter
{
public:
    void prepare (float sampleRate) { sr = sampleRate; g = 1.0f; }

    void process (float& l, float& r, float boostDb, float ceilDb, float relMs)
    {
        const float boost = std::pow (10.0f, boostDb / 20.0f);
        const float ceil = std::pow (10.0f, ceilDb / 20.0f);
        l *= boost;
        r *= boost;

        const float peak = std::max (std::fabs (l), std::fabs (r));
        const float target = peak > ceil ? ceil / peak : 1.0f;
        const float att = 1.0f - std::exp (-1.0f / (0.0004f * sr));
        const float rel = 1.0f - std::exp (-1.0f / (std::max (relMs, 5.0f) * 0.001f * sr));
        g += (target - g) * (target < g ? att : rel);

        l = std::clamp (l * g, -ceil, ceil);
        r = std::clamp (r * g, -ceil, ceil);
    }

private:
    float sr = 44100.0f, g = 1.0f;
};
} // namespace dl
