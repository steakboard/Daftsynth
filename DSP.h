#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Briques DSP de base de DaftSynth (namespace "dl" = DaftLab)
namespace dl
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

// Anti-aliasing des formes d'onde à discontinuités (scie / pulse)
inline float polyBlep (float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Générateur de bruit blanc (-1..1)
struct Rng
{
    uint32_t s;
    Rng()
    {
        static uint32_t counter = 0x9E3779B9u;
        counter = counter * 1664525u + 1013904223u;
        s = counter | 1u;
    }
    float next() noexcept
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return (float) (int32_t) s * (1.0f / 2147483648.0f);
    }
};

// Enveloppe ADSR à segments exponentiels (comportement analogique)
struct ADSR
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float level = 0.0f;

    void noteOn() noexcept { stage = Attack; }
    void noteOff() noexcept { if (stage != Idle) stage = Release; }
    void reset() noexcept { stage = Idle; level = 0.0f; }

    float next (float a, float d, float s, float r, float sr) noexcept
    {
        switch (stage)
        {
            case Attack:
                level += 1.0f / (std::max (a, 0.0005f) * sr);
                if (level >= 1.0f) { level = 1.0f; stage = Decay; }
                break;
            case Decay:
            {
                const float c = 1.0f - std::exp (-4.0f / (std::max (d, 0.001f) * sr));
                level += (s - level) * c;
                if (std::fabs (level - s) < 0.0005f) { level = s; stage = Sustain; }
                break;
            }
            case Sustain:
                level = s;
                break;
            case Release:
            {
                const float c = 1.0f - std::exp (-4.0f / (std::max (r, 0.001f) * sr));
                level -= level * c;
                if (level < 0.0005f) { level = 0.0f; stage = Idle; }
                break;
            }
            default: break;
        }
        return level;
    }
};

// Filtre passe-bas 24 dB "Moog" avec saturation (sur-échantillonné x2)
struct Ladder
{
    float s[4] = { 0, 0, 0, 0 };
    float prevIn = 0.0f;

    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0f; prevIn = 0.0f; }

    float process (float in, float cutoffHz, float res, float drive, float sr) noexcept
    {
        const float f = std::min (cutoffHz, sr * 0.45f) / (sr * 2.0f);
        const float g = 1.0f - std::exp (-kTwoPi * f);
        const float k = res * 3.9f * (1.0f - 0.6f * std::min (1.0f, f * 4.0f));
        const float gain = 1.0f + drive * 4.0f;

        for (int i = 0; i < 2; ++i)
        {
            const float x = (i == 0) ? 0.5f * (prevIn + in) : in;
            const float u = std::tanh (x * gain - k * s[3]);
            const float t0 = std::tanh (s[0]);
            const float t1 = std::tanh (s[1]);
            const float t2 = std::tanh (s[2]);
            const float t3 = std::tanh (s[3]);
            s[0] += g * (u - t0);
            s[1] += g * (t0 - t1);
            s[2] += g * (t1 - t2);
            s[3] += g * (t2 - t3);
        }
        prevIn = in;
        return s[3];
    }
};

struct OnePoleHP
{
    float z = 0.0f;
    float process (float x, float a) noexcept
    {
        z = (1.0f - a) * x + a * z;
        return x - z;
    }
};

struct OnePoleLP
{
    float z = 0.0f;
    float process (float x, float a) noexcept
    {
        z = (1.0f - a) * x + a * z;
        return z;
    }
};

// Filtre à variable d'état (TPT) : passe-bas, passe-bande, passe-haut
struct SVF
{
    float ic1 = 0.0f, ic2 = 0.0f;
    float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    struct Out { float lp, bp, hp; };

    void set (float fc, float q, float sr) noexcept
    {
        const float g = std::tan (kPi * std::min (fc, sr * 0.45f) / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    Out process (float v0) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return { v2, v1, v0 - k * v1 - v2 };
    }
};

// Passe-tout du 1er ordre (phaser)
struct AllPass
{
    float z = 0.0f;
    float process (float x, float a) noexcept
    {
        const float y = a * x + z;
        z = x - a * y;
        return y;
    }
};

// Ligne à retard avec interpolation linéaire (chorus)
struct ModDelay
{
    std::vector<float> buf;
    int w = 0;

    void init (int size) { buf.assign ((size_t) std::max (size, 8), 0.0f); w = 0; }

    float read (float delaySamples) const noexcept
    {
        const int size = (int) buf.size();
        float rp = (float) w - delaySamples;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp % size;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - std::floor (rp);
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }

    void write (float x) noexcept
    {
        buf[(size_t) w] = x;
        if (++w >= (int) buf.size()) w = 0;
    }
};
} // namespace dl
