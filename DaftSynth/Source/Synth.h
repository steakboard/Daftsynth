#pragma once
#include "DSP.h"
#include "WaveTables.h"

namespace dl
{
// Instantané des paramètres du synthé (lu une fois par bloc audio)
struct SynthParams
{
    int mode = 0; // 0 = Poly (Juno), 1 = Mono (Moog), 2 = Acid (303)

    float octave = 0, saw = 0.8f, pulse = 0, pw = 0.5f, sub = 0, noise = 0;
    float osc2 = 0, osc2Semi = 0, osc2Detune = 0;
    float fmLevel = 0, fmAmt = 0, fmRatio = 1.0f, fmFb = 0, fmEnv = 0;   // opérateur FM (modulateur -> porteuse)
    float wtLevel = 0, wtPos = 0, wtOct = 0, uniDetune = 12.0f;           // oscillateur wavetable + unison
    float drift = 0, ringMix = 0, ringRatio = 2.01f;                      // dérive analogique, ring mod
    bool sync2 = false;                                                    // hard sync (osc 2 resynchronisé par l'osc 1)
    int wtTable = 0, uniVoices = 1;

    // FM à 6 opérateurs avec matrice (mat[i][j] = quantité dont l'opérateur j module l'opérateur i)
    float fm6Level = 0, fm6Index = 2.0f;
    float fm6Ratio[6] = { 1, 1, 1, 1, 1, 1 }, fm6Out[6] = { 1, 0, 0, 0, 0, 0 }, fm6Env[6] = { 0, 0, 0, 0, 0, 0 };
    float fm6Mat[6][6] = {};

    float cutoff = 8000, res = 0.1f, drive = 0, fenv = 0.3f, keytrack = 0.5f;
    bool hpOn = false;
    float hpCoef = 0.0f;

    float aA = 0.005f, aD = 0.3f, aS = 0.8f, aR = 0.2f;
    float fA = 0.005f, fD = 0.4f, fS = 0.2f, fR = 0.3f;

    int lfoShape = 0;
    float lfoRate = 4, lfoPitch = 0, lfoCutoff = 0, lfoPwm = 0;

    float glide = 0, accent = 0.6f;
};

struct VoiceContext
{
    float lfo = 0.0f;
    float cutoffBase = 8000.0f;
    float bend = 0.0f;       // en demi-tons
    float vibrato = 0.0f;    // profondeur LFO -> hauteur (0..1)
    float sr = 44100.0f;
};

struct Voice
{
    bool active = false, gate = false, pending = false, accent = false;
    int note = 60;
    float vel = 0.8f;
    uint32_t age = 0;

    float curNote = 60.0f, targetNote = 60.0f, glideCoef = 1.0f;
    bool gliding = false;

    double ph1 = 0.0, ph2 = 0.0, phSub = 0.0, phFmC = 0.0, phFmM = 0.0;
    float fmPrev = 0.0f;
    double wph[7] = { 0, 0, 0, 0, 0, 0, 0 };
    double fph[6] = { 0, 0, 0, 0, 0, 0 };
    double phRing = 0.0;
    float drift1 = 0.0f, drift2 = 0.0f;   // dérive lente aléatoire propre à chaque voix
    float fprev[6] = { 0, 0, 0, 0, 0, 0 };
    ADSR aEnv, fEnv;
    Ladder lad;
    OnePoleHP hp;
    Rng rng;

    void trigger (int n, float v, bool retrig, float glideTime, bool acc, float sr, uint32_t newAge)
    {
        note = n;
        vel = v;
        accent = acc;
        targetNote = (float) n;

        if (! active || glideTime <= 0.0f)
        {
            curNote = targetNote;
            gliding = false;
            glideCoef = 1.0f;
        }
        else
        {
            gliding = true;
            glideCoef = 1.0f - std::exp (-4.0f / (glideTime * sr));
        }

        if (! active)
        {
            for (auto& w : wph) w = (double) (rng.next() * 0.5f + 0.5f);
            for (auto& q : fph) q = 0.0;
            for (auto& q : fprev) q = 0.0f;
            aEnv.reset();
            fEnv.reset();
            lad.reset();
            aEnv.noteOn();
            fEnv.noteOn();
        }
        else if (retrig)
        {
            aEnv.noteOn();
            fEnv.noteOn();
        }

        gate = true;
        pending = false;
        active = true;
        age = newAge;
    }

    void release() noexcept
    {
        gate = false;
        pending = false;
        aEnv.noteOff();
        fEnv.noteOff();
    }

    float render (const SynthParams& p, const VoiceContext& c)
    {
        if (! active) return 0.0f;

        if (gliding) curNote += (targetNote - curNote) * glideCoef;

        // dérive analogique : marche aléatoire lente, indépendante pour chaque voix (hauteur ±12 cents, filtre ±1/4 d'octave)
        drift1 += 3.0e-5f * (444.0f * rng.next() - drift1);     // bruit filtré, constante de temps ~0,75 s, écart-type ~1
        drift2 += 3.0e-5f * (444.0f * rng.next() - drift2);
        const float semis = curNote + p.octave * 12.0f + c.bend + c.lfo * c.vibrato * 2.0f + drift1 * 0.08f * p.drift;
        const float f1 = 440.0f * std::exp2 ((semis - 69.0f) / 12.0f);
        const float f2 = f1 * std::exp2 ((p.osc2Semi + p.osc2Detune * 0.01f) / 12.0f);
        const float dt1 = std::min (f1 / c.sr, 0.45f);
        const float dt2 = std::min (f2 / c.sr, 0.45f);
        const float dts = dt1 * 0.5f;

        ph1 += dt1;
        ph2 += dt2;
        if (ph1 >= 1.0)
        {
            if (p.sync2) ph2 = (ph1 - 1.0) * (double) (dt2 / std::max (dt1, 1.0e-6f));   // hard sync
            ph1 -= 1.0;
        }
        if (ph2 >= 1.0) ph2 -= std::floor (ph2);
        phSub += dts; if (phSub >= 1.0) phSub -= 1.0;

        const float t1 = (float) ph1;
        const float t2 = (float) ph2;
        const float ts = (float) phSub;
        const float pw = std::clamp (p.pw + c.lfo * p.lfoPwm * 0.4f, 0.05f, 0.95f);

        float x = 0.0f;

        if (p.saw > 0.0f)
            x += (2.0f * t1 - 1.0f - polyBlep (t1, dt1)) * p.saw;

        if (p.pulse > 0.0f)
        {
            float pl = (t1 < pw) ? 1.0f : -1.0f;
            pl += polyBlep (t1, dt1);
            pl -= polyBlep (std::fmod (t1 + 1.0f - pw, 1.0f), dt1);
            x += pl * p.pulse;
        }

        if (p.sub > 0.0f)
        {
            float sq = (ts < 0.5f) ? 1.0f : -1.0f;
            sq += polyBlep (ts, dts);
            sq -= polyBlep (std::fmod (ts + 0.5f, 1.0f), dts);
            x += sq * p.sub;
        }

        if (p.osc2 > 0.0f)
            x += (2.0f * t2 - 1.0f - polyBlep (t2, dt2)) * p.osc2;

        if (p.noise > 0.0f)
            x += rng.next() * p.noise;

        // Oscillateur wavetable avec unison (jusqu'à 7 voix désaccordées)
        if (p.wtLevel > 0.0f)
        {
            const auto& wt = WaveTables::get();
            const int nu = std::clamp (p.uniVoices, 1, 7);
            const float fw = f1 * std::exp2 (p.wtOct);
            float acc = 0.0f;
            for (int u = 0; u < nu; ++u)
            {
                const float off = nu > 1 ? ((float) u / (float) (nu - 1) * 2.0f - 1.0f) * p.uniDetune : 0.0f;
                const float fu = fw * std::exp2 (off / 1200.0f);
                wph[u] += (double) (fu / c.sr);
                if (wph[u] >= 1.0) wph[u] -= 1.0;
                acc += wt.read (p.wtTable, p.wtPos, (float) wph[u], WaveTables::levelFor (fu, c.sr));
            }
            x += acc * p.wtLevel * 0.9f / std::sqrt ((float) nu);
        }

        const float fe = fEnv.next (p.fA, p.fD, p.fS, p.fR, c.sr);

        // Opérateur FM : un modulateur sinusoïdal (avec feedback) module la phase de la porteuse
        if (p.fmLevel > 0.0f)
        {
            phFmM += (double) (f1 * p.fmRatio / c.sr);  if (phFmM >= 1.0) phFmM -= 1.0;
            phFmC += (double) (f1 / c.sr);              if (phFmC >= 1.0) phFmC -= 1.0;
            const float m = std::sin (kTwoPi * (float) phFmM + p.fmFb * 2.5f * fmPrev);
            fmPrev = m;
            const float idx = p.fmAmt * (1.0f - p.fmEnv + p.fmEnv * fe);
            x += std::sin (kTwoPi * (float) phFmC + idx * m) * p.fmLevel;
        }

        // FM à 6 opérateurs : chaque opérateur est modulé par les sorties (échantillon précédent) des autres via la matrice
        if (p.fm6Level > 0.0f)
        {
            float o[6], sum = 0.0f;
            for (int i = 0; i < 6; ++i)
            {
                fph[i] += (double) (f1 * p.fm6Ratio[i] / c.sr);
                fph[i] -= std::floor (fph[i]);
                float mod = 0.0f;
                for (int j = 0; j < 6; ++j) mod += p.fm6Mat[i][j] * fprev[j];
                const float envScale = 1.0f - p.fm6Env[i] + p.fm6Env[i] * fe;
                o[i] = std::sin (kTwoPi * (float) fph[i] + p.fm6Index * mod) * envScale;
                sum += o[i] * p.fm6Out[i];
            }
            for (int i = 0; i < 6; ++i) fprev[i] = o[i];
            x += sum * p.fm6Level;
        }

        // ring modulation : le mélange des oscillateurs est multiplié par un sinus inharmonique
        if (p.ringMix > 0.001f)
        {
            phRing += (double) (f1 * p.ringRatio / c.sr);
            phRing -= std::floor (phRing);
            x = x * (1.0f - p.ringMix) + x * std::sin (kTwoPi * (float) phRing) * p.ringMix * 1.6f;
        }

        x *= 0.6f;

        if (p.hpOn)
            x = hp.process (x, p.hpCoef);

        float oct = drift2 * 0.1f * p.drift
                    + p.fenv * fe * 5.0f
                    + p.keytrack * ((float) note - 60.0f) / 12.0f
                    + c.lfo * p.lfoCutoff * 2.0f;
        if (accent) oct += p.accent * 2.0f * fe;

        const float cutoffHz = std::clamp (c.cutoffBase * std::exp2 (oct), 20.0f, 18000.0f);
        const float y = lad.process (x, cutoffHz, p.res, p.drive, c.sr);

        const float ae = aEnv.next (p.aA, p.aD, p.aS, p.aR, c.sr);

        float velGain;
        if (p.mode == 2)
            velGain = accent ? (1.0f + 0.6f * p.accent) : 0.75f;
        else
            velGain = 0.3f + 0.7f * vel;

        if (aEnv.stage == ADSR::Idle || (aEnv.stage == ADSR::Sustain && p.aS < 0.001f))
            active = false;

        return y * ae * velGain;
    }
};

class SynthEngine
{
public:
    static constexpr int kMaxVoices = 12;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        allNotesOff();
        lfoPhase = 0.0f;
        cutSm = P.cutoff;
    }

    void setParams (const SynthParams& p) { P = p; }
    void setBend (float semitones) { bend = semitones; }
    void setMod (float m) { mod = m; }

    void allNotesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.gate = false;
            v.pending = false;
        }
        held.clear();
    }

    void setSustain (bool down)
    {
        sustain = down;
        if (! down)
            for (auto& v : voices)
                if (v.active && v.pending) v.release();
    }

    void noteOn (int note, float vel)
    {
        if (P.mode == 0)
        {
            int idx = -1;
            for (int i = 0; i < kMaxVoices; ++i)
                if (voices[i].active && voices[i].note == note && voices[i].gate) { idx = i; break; }

            if (idx < 0)
                for (int i = 0; i < kMaxVoices; ++i)
                    if (! voices[i].active) { idx = i; break; }

            if (idx < 0)
            {
                uint32_t best = UINT32_MAX;
                for (int i = 0; i < kMaxVoices; ++i)
                {
                    const uint32_t score = voices[i].age + (voices[i].gate ? (1u << 30) : 0u);
                    if (score < best) { best = score; idx = i; }
                }
            }
            voices[idx].trigger (note, vel, true, 0.0f, false, sampleRate, ++counter);
            return;
        }

        // Modes monophoniques (Moog / 303)
        auto& v = voices[0];
        const bool legato = ! held.empty() && v.active && v.gate;

        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());
        held.push_back ({ note, vel });

        if (P.mode == 2)
        {
            const float g = legato ? std::max (P.glide, 0.06f) : 0.0f;
            v.trigger (note, vel, ! legato, g, vel >= 0.78f, sampleRate, ++counter);
        }
        else
        {
            v.trigger (note, vel, true, P.glide, false, sampleRate, ++counter);
        }
    }

    void noteOff (int note)
    {
        if (P.mode == 0)
        {
            for (auto& v : voices)
                if (v.active && v.gate && ! v.pending && v.note == note)
                {
                    if (sustain) v.pending = true;
                    else v.release();
                }
            return;
        }

        if (held.empty()) return;
        const bool wasTop = (held.back().note == note);
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());

        auto& v = voices[0];
        if (held.empty())
        {
            v.release();
        }
        else if (wasTop)
        {
            const auto last = held.back();
            const float g = (P.mode == 2) ? std::max (P.glide, 0.06f) : P.glide;
            v.trigger (last.note, last.vel, false, g, P.mode == 2 && last.vel >= 0.78f, sampleRate, ++counter);
        }
    }

    // Ajoute le signal mono du synthé dans out[0..n)
    void renderMono (float* out, int n)
    {
        VoiceContext c;
        c.sr = sampleRate;
        c.bend = bend;

        for (int i = 0; i < n; ++i)
        {
            // LFO global
            const float prev = lfoPhase;
            lfoPhase += P.lfoRate / sampleRate;
            if (lfoPhase >= 1.0f)
            {
                lfoPhase -= 1.0f;
                lfoRand = rng.next();
            }
            (void) prev;

            switch (P.lfoShape)
            {
                case 0: c.lfo = std::sin (kTwoPi * lfoPhase); break;
                case 1: c.lfo = 1.0f - 4.0f * std::fabs (lfoPhase - 0.5f); break;
                case 2: c.lfo = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
                default: c.lfo = lfoRand; break;
            }

            cutSm += (P.cutoff - cutSm) * 0.002f;
            c.cutoffBase = cutSm;
            c.vibrato = std::min (1.0f, P.lfoPitch + mod);

            float s = 0.0f;
            const int nv = (P.mode == 0) ? kMaxVoices : 1;
            for (int v = 0; v < nv; ++v)
                if (voices[v].active) s += voices[v].render (P, c);

            out[i] += s * 0.75f;
        }
    }

private:
    struct Held { int note; float vel; };

    SynthParams P;
    Voice voices[kMaxVoices];
    std::vector<Held> held;
    Rng rng;

    float sampleRate = 44100.0f;
    float lfoPhase = 0.0f, lfoRand = 0.0f;
    float cutSm = 8000.0f;
    float bend = 0.0f, mod = 0.0f;
    bool sustain = false;
    uint32_t counter = 0;
};
} // namespace dl
