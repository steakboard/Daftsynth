#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "DSP.h"

namespace dl
{
// 7 wavetables générées par synthèse additive (32 images chacune, 1024 échantillons).
// Chaque image existe en 8 versions à bande limitée (mipmaps) pour éviter l'aliasing aux notes aiguës.
class WaveTables
{
public:
    static constexpr int N = 1024;   // échantillons par cycle
    static constexpr int F = 32;     // images par table (position 0..1)
    static constexpr int L = 8;      // niveaux de bande limitée : 4, 8, 16 ... 512 harmoniques
    static constexpr int T = 7;      // nombre de tables

    static const WaveTables& get()
    {
        static const WaveTables w;
        return w;
    }

    static const char* name (int t)
    {
        static const char* n[T] = { "Basic", "Growl Sweep", "Digital", "PWM", "Vowels", "FM Sweep", "Sync Sweep" };
        return n[std::clamp (t, 0, T - 1)];
    }

    const float* frame (int t, int f, int l) const
    {
        return &data[((((size_t) t * F + (size_t) f) * L) + (size_t) l) * (N + 1)];
    }

    // phase 0..1, pos 0..1 : lit la table avec interpolation entre images et entre échantillons
    float read (int t, float pos, float phase, int level) const
    {
        const float fp = std::clamp (pos, 0.0f, 1.0f) * (float) (F - 1);
        const int f0 = std::min ((int) fp, F - 2);
        const float ff = fp - (float) f0;
        const float x = phase * (float) N;
        const int i = std::min ((int) x, N - 1);
        const float fr = x - (float) i;
        const float* a = frame (t, f0, level);
        const float* b = frame (t, f0 + 1, level);
        const float sa = a[i] + (a[i + 1] - a[i]) * fr;
        const float sb = b[i] + (b[i + 1] - b[i]) * fr;
        return sa + (sb - sa) * ff;
    }

    // niveau de bande limitée adapté à la fréquence de lecture
    static int levelFor (float freq, float sr)
    {
        const float maxH = 0.45f * sr / std::max (freq, 1.0f);
        return std::clamp ((int) std::floor (std::log2 (std::max (maxH, 4.0f) / 4.0f)), 0, L - 1);
    }

private:
    WaveTables()
    {
        data.assign ((size_t) T * F * L * (N + 1), 0.0f);

        std::vector<float> S ((size_t) N);
        for (int i = 0; i < N; ++i) S[(size_t) i] = std::sin (kTwoPi * (float) i / (float) N);

        std::vector<float> a (513), b (513), acc ((size_t) N);

        for (int t = 0; t < T; ++t)
            for (int f = 0; f < F; ++f)
            {
                std::fill (a.begin(), a.end(), 0.0f);
                std::fill (b.begin(), b.end(), 0.0f);
                harmonics (t, (float) f / (float) (F - 1), a, b, S);

                std::fill (acc.begin(), acc.end(), 0.0f);
                int level = 0;
                for (int h = 1; h <= 512; ++h)
                {
                    const float ah = a[(size_t) h], bh = b[(size_t) h];
                    if (std::fabs (ah) + std::fabs (bh) > 1.0e-6f)
                        for (int n = 0; n < N; ++n)
                            acc[(size_t) n] += ah * S[(size_t) ((h * n) & (N - 1))] + bh * S[(size_t) ((h * n + N / 4) & (N - 1))];

                    if (h == (4 << level))
                    {
                        float* dst = const_cast<float*> (frame (t, f, level));
                        std::copy (acc.begin(), acc.end(), dst);
                        ++level;
                    }
                }

                // normalisation commune à tous les niveaux d'une image
                const float* top = frame (t, f, L - 1);
                float peak = 0.0f, sumSq = 0.0f;
                for (int n = 0; n < N; ++n)
                {
                    peak = std::max (peak, std::fabs (top[n]));
                    sumSq += top[n] * top[n];
                }
                const float rms = std::sqrt (sumSq / (float) N);
                // niveau efficace constant d'une image à l'autre (pas de saut de volume en morphing), crête limitée
                const float scale = (peak > 1.0e-6f && rms > 1.0e-6f) ? std::min (0.5f / rms, 1.5f / peak) : 0.0f;
                for (int l = 0; l < L; ++l)
                {
                    float* d = const_cast<float*> (frame (t, f, l));
                    for (int n = 0; n < N; ++n) d[n] *= scale;
                    d[N] = d[0];
                }
            }
    }

    static float lerp (float x, float y, float t) { return x + (y - x) * t; }

    static void harmonics (int t, float x, std::vector<float>& a, std::vector<float>& b, const std::vector<float>& S)
    {
        const float pi = kPi;
        switch (t)
        {
            case 0: // Basic : sinus -> triangle -> scie -> carré
            {
                const float fp = x * 3.0f;
                const int i = std::min ((int) fp, 2);
                const float fr = fp - (float) i;
                auto coef = [pi] (int shape, int h)
                {
                    switch (shape)
                    {
                        case 0: return h == 1 ? 1.0f : 0.0f;
                        case 1: return (h & 1) ? (8.0f / (pi * pi)) * ((((h - 1) / 2) & 1) ? -1.0f : 1.0f) / (float) (h * h) : 0.0f;
                        case 2: return (2.0f / pi) * ((h & 1) ? 1.0f : -1.0f) / (float) h;
                        default: return (h & 1) ? 4.0f / (pi * (float) h) : 0.0f;
                    }
                };
                for (int h = 1; h <= 512; ++h) a[(size_t) h] = lerp (coef (i, h), coef (i + 1, h), fr);
                break;
            }
            case 1: // Growl Sweep : scie avec pics de résonance qui montent dans le spectre
            {
                const float c1 = 1.5f + 16.0f * x, w1 = 1.5f + 2.0f * x, c2 = 5.0f + 30.0f * x, w2 = 3.0f;
                for (int h = 1; h <= 512; ++h)
                {
                    const float d1 = ((float) h - c1) / w1, d2 = ((float) h - c2) / w2;
                    a[(size_t) h] = (1.0f / (float) h) * (1.0f + 5.0f * std::exp (-d1 * d1) + 3.0f * std::exp (-d2 * d2));
                }
                break;
            }
            case 2: // Digital : harmoniques paires / impaires et peigne spectral
            {
                for (int h = 1; h <= 512; ++h)
                {
                    const float even = (h & 1) ? 1.0f : x;
                    const float comb = 1.0f + 0.8f * std::cos ((float) h * x * pi * 0.5f);
                    a[(size_t) h] = std::pow ((float) h, -0.6f) * even * comb * (((h / 2) & 1) ? -1.0f : 1.0f);
                }
                break;
            }
            case 3: // PWM : largeur d'impulsion de 50 % à 3 %
            {
                const float w = 0.5f - 0.47f * x;
                for (int h = 1; h <= 512; ++h)
                    b[(size_t) h] = (2.0f / (pi * (float) h)) * std::sin (pi * (float) h * w);
                break;
            }
            case 4: // Vowels : A E I O U
            {
                static const float Fm[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                                { 570, 840, 2410 }, { 300, 870, 2240 } };
                static const float G[3] = { 1.0f, 0.7f, 0.4f };
                const float v = x * 4.0f;
                const int i0 = std::min (3, (int) v);
                const float fr = v - (float) i0;
                float fm[3];
                for (int n = 0; n < 3; ++n) fm[n] = lerp (Fm[i0][n], Fm[i0 + 1][n], fr);
                for (int h = 1; h <= 512; ++h)
                {
                    const float fh = (float) h * 110.0f;
                    float amp = 0.02f / (float) h;
                    for (int n = 0; n < 3; ++n)
                    {
                        const float bw = fm[n] * 0.15f + 60.0f, d = (fh - fm[n]) / bw;
                        amp += G[n] * std::exp (-d * d);
                    }
                    a[(size_t) h] = amp;
                }
                break;
            }
            case 5: // FM Sweep : spectre d'un son FM (rapport 1:1) dont l'indice augmente
            {
                const float idx = x * 7.0f;
                float J[41];
                for (int n = 0; n <= 40; ++n)
                {
                    float s = 0.0f;
                    for (int k = 0; k < 64; ++k)
                    {
                        const float tau = pi * ((float) k + 0.5f) / 64.0f;
                        s += std::cos ((float) n * tau - idx * std::sin (tau));
                    }
                    J[n] = s / 64.0f;
                }
                for (int n = -40; n <= 40; ++n)
                {
                    const float jn = n >= 0 ? J[n] : ((n & 1) ? -J[-n] : J[-n]);
                    const int k = 1 + n;
                    if (k > 0) a[(size_t) k] += jn;
                    else if (k < 0) a[(size_t) -k] -= jn;
                }
                break;
            }
            default: // Sync Sweep : scie esclave dont la fréquence monte (synchro dure)
            {
                const float r = 1.0f + x * 7.0f;
                std::vector<float> y ((size_t) N);
                for (int n = 0; n < N; ++n)
                {
                    const float ph = r * (float) n / (float) N;
                    y[(size_t) n] = 2.0f * (ph - std::floor (ph)) - 1.0f;
                }
                for (int h = 1; h <= 512; ++h)
                {
                    float sa = 0.0f, sb = 0.0f;
                    for (int n = 0; n < N; ++n)
                    {
                        sa += y[(size_t) n] * S[(size_t) ((h * n) & (N - 1))];
                        sb += y[(size_t) n] * S[(size_t) ((h * n + N / 4) & (N - 1))];
                    }
                    a[(size_t) h] = 2.0f * sa / (float) N;
                    b[(size_t) h] = 2.0f * sb / (float) N;
                }
                break;
            }
        }
    }

    std::vector<float> data;
};
} // namespace dl
