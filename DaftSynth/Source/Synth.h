#pragma once
#include "DSP.h"

namespace dl
{
// Instantané des paramètres du synthé (lu une fois par bloc audio)
struct SynthParams
{
    int mode = 0; // 0 = Poly (Juno), 1 = Mono (Moog), 2 = Acid (303)

    float octave = 0, saw = 0.8f, pulse = 0, pw = 0.5f, sub = 0, noise = 0;
    float osc2 = 0, osc2Semi = 0, osc2Detune = 0;

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

    double ph1 = 0.0, ph2 = 0.0, phSub = 0.0;
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

        const float semis = curNote + p.octave * 12.0f + c.bend + c.lfo * c.vibrato * 2.0f;
        const float f1 = 440.0f * std::exp2 ((semis - 69.0f) / 12.0f);
        const float f2 = f1 * std::exp2 ((p.osc2Semi + p.osc2Detune * 0.01f) / 12.0f);
        const float dt1 = std::min (f1 / c.sr, 0.45f);
        const float dt2 = std::min (f2 / c.sr, 0.45f);
        const float dts = dt1 * 0.5f;

        ph1 += dt1;  if (ph1 >= 1.0) ph1 -= 1.0;
        ph2 += dt2;  if (ph2 >= 1.0) ph2 -= 1.0;
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

        x *= 0.6f;

        if (p.hpOn)
            x = hp.process (x, p.hpCoef);

        const float fe = fEnv.next (p.fA, p.fD, p.fS, p.fR, c.sr);
        float oct = p.fenv * fe * 5.0f
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
