#pragma once
#include "DSP.h"

// Boîte à rythmes synthétisée, inspirée de la TR-808, TR-909, LinnDrum et Oberheim DMX.
// Tous les sons sont générés par synthèse : aucun sample n'est utilisé.
namespace dl
{
enum DrumType { Kick = 0, Snare, Clap, HatC, HatO, TomLo, TomMid, TomHi, Rim, Cowbell, Crash, NumDrumTypes };

// Correspondance note MIDI -> instrument (map GM)
inline int drumTypeForNote (int n)
{
    switch (n)
    {
        case 35: case 36: return Kick;
        case 37: return Rim;
        case 38: case 40: return Snare;
        case 39: return Clap;
        case 41: case 43: return TomLo;
        case 42: case 44: return HatC;
        case 45: case 47: return TomMid;
        case 46: return HatO;
        case 48: case 50: return TomHi;
        case 49: case 51: case 52: case 57: return Crash;
        case 56: return Cowbell;
        default: return -1;
    }
}

class DrumVoice
{
public:
    bool active = false;
    int type = 0;
    uint32_t age = 0;

    void trigger (int ty, float v, int k, float tuneSemis, float decayMul, float toneAmt, float sampleRate, uint32_t a)
    {
        type = ty;
        vel = v;
        kit = std::clamp (k, 0, 3);
        tuneR = std::exp2 (tuneSemis / 12.0f);
        decM = decayMul;
        tone = toneAmt;
        sr = sampleRate;
        dt = 1.0f / sr;
        t = 0.0f;
        ph = ph2 = 0.0f;
        for (auto& q : hph) q = 0.0f;
        age = a;
        active = true;
        filtA = {};
        filtB = {};

        switch (type)
        {
            case Snare:  filtA.set (1200.0f + 3500.0f * tone, 0.7f, sr); break;
            case Clap:   filtA.set (1100.0f * tuneR, 2.0f, sr); break;
            case HatC: case HatO: case Crash:
                         filtA.set (6000.0f + 3000.0f * tone, 0.7f, sr); break;
            case Cowbell: filtA.set (1000.0f * tuneR, 1.2f, sr); break;
            default: break;
        }
    }

    float render()
    {
        if (! active) return 0.0f;

        float o = 0.0f, e = 0.0f;

        switch (type)
        {
            case Kick:
            {
                static const float startF[4] = { 170.0f, 260.0f, 150.0f, 180.0f };
                static const float endF[4]   = { 46.0f, 52.0f, 58.0f, 62.0f };
                static const float pitchD[4] = { 0.04f, 0.03f, 0.025f, 0.02f };
                static const float ampD[4]   = { 0.95f, 0.40f, 0.26f, 0.20f };
                static const float click[4]  = { 0.10f, 0.35f, 0.25f, 0.40f };
                static const float drv[4]    = { 0.30f, 1.20f, 0.60f, 0.50f };

                const float tau = ampD[kit] * decM * 0.25f;
                const float f = (endF[kit] + (startF[kit] - endF[kit]) * std::exp (-t / pitchD[kit])) * tuneR;
                ph += f * dt;
                ph -= std::floor (ph);
                const float s = std::sin (kTwoPi * ph);
                e = std::exp (-t / tau);
                const float clk = rng.next() * std::exp (-t / 0.002f) * click[kit] * (0.4f + 1.2f * tone);
                o = std::tanh (s * e * (1.0f + drv[kit] * 2.0f)) / (1.0f + drv[kit] * 0.5f) + clk;
                break;
            }
            case Snare:
            {
                static const float f1[4] = { 180.0f, 185.0f, 200.0f, 220.0f };
                static const float f2[4] = { 330.0f, 330.0f, 360.0f, 400.0f };
                static const float tT[4] = { 0.030f, 0.040f, 0.030f, 0.025f };
                static const float nT[4] = { 0.060f, 0.070f, 0.040f, 0.035f };
                static const float tL[4] = { 0.60f, 0.50f, 0.45f, 0.50f };
                static const float nL[4] = { 0.70f, 0.85f, 0.60f, 0.60f };

                const float sweep = 1.0f + 0.3f * std::exp (-t / 0.01f);
                ph  += f1[kit] * tuneR * sweep * dt;  ph  -= std::floor (ph);
                ph2 += f2[kit] * tuneR * sweep * dt;  ph2 -= std::floor (ph2);
                const float te = std::exp (-t / (tT[kit] * decM));
                const float ne = std::exp (-t / (nT[kit] * decM));
                const float tn = (std::sin (kTwoPi * ph) + 0.6f * std::sin (kTwoPi * ph2)) * te * tL[kit];
                const float nz = filtA.process (rng.next()).hp * ne * nL[kit];
                o = (tn + nz) * 0.7f;
                e = std::max (te, ne);
                break;
            }
            case Clap:
            {
                static const float tail[4] = { 0.12f, 0.09f, 0.07f, 0.06f };
                float env;
                if (t < 0.033f)
                    env = std::exp (-std::fmod (t, 0.011f) / 0.003f);
                else
                    env = 0.8f * std::exp (-(t - 0.033f) / (tail[kit] * decM));
                o = filtA.process (rng.next()).bp * env * 2.2f;
                e = (t < 0.033f) ? 1.0f : env;
                break;
            }
            case HatC:
            case HatO:
            {
                static const float hf[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
                static const float closedT[4] = { 0.020f, 0.030f, 0.025f, 0.018f };
                static const float openT[4]   = { 0.120f, 0.180f, 0.140f, 0.100f };

                float s = 0.0f;
                for (int i = 0; i < 6; ++i)
                {
                    hph[i] += hf[i] * tuneR * 2.5f * dt;
                    hph[i] -= std::floor (hph[i]);
                    s += (hph[i] < 0.5f) ? 1.0f : -1.0f;
                }
                const float tau = (type == HatC ? closedT[kit] : openT[kit]) * decM;
                e = std::exp (-t / tau);
                o = filtA.process (s * (1.0f / 6.0f)).hp * e * 2.0f;
                break;
            }
            case TomLo:
            case TomMid:
            case TomHi:
            {
                static const float base[3] = { 90.0f, 130.0f, 185.0f };
                static const float tauK[4] = { 0.20f, 0.16f, 0.14f, 0.12f };
                const float b = base[type - TomLo] * tuneR * (kit == 1 ? 1.1f : 1.0f);
                const float f = b * (1.0f + 0.8f * std::exp (-t / 0.04f));
                ph += f * dt;
                ph -= std::floor (ph);
                e = std::exp (-t / (tauK[kit] * decM));
                o = std::sin (kTwoPi * ph) * e + rng.next() * std::exp (-t / 0.003f) * 0.15f;
                break;
            }
            case Rim:
            {
                ph  += 1667.0f * tuneR * dt;  ph  -= std::floor (ph);
                ph2 += 455.0f * tuneR * dt;   ph2 -= std::floor (ph2);
                e = std::exp (-t / (0.010f * decM));
                o = (std::sin (kTwoPi * ph) * 0.6f + std::sin (kTwoPi * ph2) * 0.6f + rng.next() * 0.2f) * e * 0.7f;
                break;
            }
            case Cowbell:
            {
                ph  += 540.0f * tuneR * dt;  ph  -= std::floor (ph);
                ph2 += 800.0f * tuneR * dt;  ph2 -= std::floor (ph2);
                const float sq = ((ph < 0.5f) ? 1.0f : -1.0f) + ((ph2 < 0.5f) ? 1.0f : -1.0f);
                e = std::exp (-t / (0.07f * decM));
                o = filtA.process (sq * 0.5f).bp * e * 0.7f;
                break;
            }
            case Crash:
            default:
            {
                static const float hf[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
                float s = 0.0f;
                for (int i = 0; i < 6; ++i)
                {
                    hph[i] += hf[i] * tuneR * 2.5f * dt;
                    hph[i] -= std::floor (hph[i]);
                    s += (hph[i] < 0.5f) ? 1.0f : -1.0f;
                }
                e = std::exp (-t / (0.5f * decM));
                o = filtA.process (s * (0.4f / 6.0f) + rng.next() * 0.4f).hp * e * 1.6f;
                break;
            }
        }

        t += dt;
        if (t > 0.02f && e < 0.003f) active = false;
        return o * (0.25f + 0.75f * vel);
    }

private:
    float vel = 1.0f, tuneR = 1.0f, decM = 1.0f, tone = 0.5f, sr = 44100.0f, dt = 1.0f / 44100.0f;
    float t = 0.0f, ph = 0.0f, ph2 = 0.0f;
    float hph[6] = { 0, 0, 0, 0, 0, 0 };
    int kit = 0;
    SVF filtA, filtB;
    Rng rng;
};
} // namespace dl
