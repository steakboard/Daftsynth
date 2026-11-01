#pragma once
#include "DSP.h"

namespace dl
{
// Sources de modulation utilisables dans la matrice
enum ModSrc
{
    SrcNone = 0,
    SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4,
    SrcEnv1, SrcEnv2,
    SrcVelocity, SrcWheel,
    SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4,
    SrcRandom,
    NumModSrc
};

// Nombre de cycles par temps (noire) pour chaque division de synchro.
// 0 = libre (pas de synchro). Ordre : Free, 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4T, 1/8T, 1/16T, 1/4., 1/8., 1/16.
inline float syncCyclesPerBeat (int idx)
{
    static const float t[13] = { 0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f,
                                 1.5f, 3.0f, 6.0f,
                                 2.0f / 3.0f, 4.0f / 3.0f, 8.0f / 3.0f };
    return t[std::clamp (idx, 0, 12)];
}

struct LfoConfig
{
    int shape = 0;       // 0 Sine, 1 Tri, 2 SawUp, 3 SawDown, 4 Square, 5 S&H, 6 SmoothRandom
    float rate = 1.0f;   // Hz (si pas de synchro)
    int sync = 0;        // index de division (0 = libre)
    bool retrig = false; // redémarre à chaque note
};

struct EnvConfig
{
    float a = 0.01f, d = 0.3f, s = 0.5f, r = 0.3f;
};

class ModEngine
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        for (auto& l : lfo)
        {
            l.phase = 0.0f;
            l.rnd = rng.next();
            l.nextRnd = rng.next();
        }
        env[0].reset();
        env[1].reset();
        held = 0;
        vel = 0.0f;
        wheel = 0.0f;
        noteRand = 0.5f;
    }

    void setRetrig (const LfoConfig cfg[4])
    {
        for (int i = 0; i < 4; ++i) retrig[i] = cfg[i].retrig;
    }

    void noteOn (float v)
    {
        ++held;
        vel = v;
        noteRand = rng.next() * 0.5f + 0.5f;
        env[0].noteOn();
        env[1].noteOn();
        for (int i = 0; i < 4; ++i)
            if (retrig[i]) lfo[i].phase = 0.0f;
    }

    void noteOff()
    {
        if (held > 0) --held;
        if (held == 0)
        {
            env[0].noteOff();
            env[1].noteOff();
        }
    }

    void allOff()
    {
        held = 0;
        env[0].noteOff();
        env[1].noteOff();
    }

    void setWheel (float w) { wheel = w; }

    // Avance de n échantillons et écrit les valeurs des sources dans out[NumModSrc]
    // (LFO : -1..1 ; enveloppes, vélocité, molette, macros, random : 0..1)
    void update (int n, double ppq, bool havePpq, double bpm,
                 const LfoConfig cfg[4], const EnvConfig ecfg[2], const float macros[4], float out[NumModSrc])
    {
        out[SrcNone] = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            auto& l = lfo[i];
            const auto& c = cfg[i];
            const float cpb = syncCyclesPerBeat (c.sync);
            const float old = l.phase;
            float p;

            if (cpb > 0.0f)
            {
                if (havePpq)
                    p = (float) (ppq * (double) cpb - std::floor (ppq * (double) cpb));
                else
                    p = old + (float) (bpm / 60.0 * (double) cpb * (double) n / (double) sr);
            }
            else
            {
                p = old + c.rate * (float) n / sr;
            }

            p -= std::floor (p);
            if (p < old)
            {
                l.rnd = l.nextRnd;
                l.nextRnd = rng.next();
            }
            l.phase = p;

            float v = 0.0f;
            switch (c.shape)
            {
                case 0: v = std::sin (kTwoPi * p); break;
                case 1: v = 1.0f - 4.0f * std::fabs (p - 0.5f); break;
                case 2: v = 2.0f * p - 1.0f; break;
                case 3: v = 1.0f - 2.0f * p; break;
                case 4: v = p < 0.5f ? 1.0f : -1.0f; break;
                case 5: v = l.rnd; break;
                default:
                {
                    const float s = p * p * (3.0f - 2.0f * p);
                    v = l.rnd + (l.nextRnd - l.rnd) * s;
                    break;
                }
            }
            out[SrcLfo1 + i] = v;
        }

        for (int e = 0; e < 2; ++e)
        {
            float lvl = env[e].level;
            for (int k = 0; k < n; ++k)
                lvl = env[e].next (ecfg[e].a, ecfg[e].d, ecfg[e].s, ecfg[e].r, sr);
            out[SrcEnv1 + e] = lvl;
        }

        out[SrcVelocity] = vel;
        out[SrcWheel] = wheel;
        for (int m = 0; m < 4; ++m) out[SrcMacro1 + m] = macros[m];
        out[SrcRandom] = noteRand;
    }

private:
    struct Lfo { float phase = 0.0f, rnd = 0.0f, nextRnd = 0.0f; };

    Lfo lfo[4];
    ADSR env[2];
    bool retrig[4] = { false, false, false, false };
    Rng rng;
    float sr = 44100.0f;
    int held = 0;
    float vel = 0.0f, wheel = 0.0f, noteRand = 0.5f;
};
} // namespace dl
