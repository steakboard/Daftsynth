#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Arpégiateur synchronisé au tempo (notes tenues -> séquence), avec swing, gate, octaves, hold (latch)
namespace dl
{
class Arpeggiator
{
public:
    struct Cfg
    {
        bool on = false;
        int mode = 0;            // 0 Up, 1 Down, 2 UpDown, 3 Random, 4 AsPlayed, 5 Chord
        float stepBeats = 0.25f; // durée d'un pas en noires
        int octaves = 1;
        float gate = 0.6f;       // 0..1 de la durée du pas
        float swing = 0.0f;      // retard des pas impairs (fraction de pas)
        bool hold = false;
    };

    void setCfg (const Cfg& c) { cfg = c; }
    bool hasNotes() const { return ! held.empty(); }

    void noteOn (int note, float vel)
    {
        if (physDown == 0 && cfg.hold) held.clear();
        ++physDown;
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());
        held.push_back ({ note, vel });
    }

    void noteOff (int note)
    {
        if (physDown > 0) --physDown;
        if (! cfg.hold)
            held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());
    }

    template <class OffFn>
    void releaseAll (OffFn&& off)
    {
        for (int n : cur) off (n);
        cur.clear();
        sounding = false;
    }

    void reset()
    {
        held.clear();
        physDown = 0;
        initialized = false;
        pos = 0;
    }

    // Traite l'intervalle de temps [beatNow, beatEnd) (en noires) et émet les notes via on(note, vel) / off(note)
    template <class OnFn, class OffFn>
    void process (double beatNow, double beatEnd, OnFn&& on, OffFn&& off)
    {
        const double sb = std::max (1.0e-4, (double) cfg.stepBeats);

        if (! initialized || beatNow < lastBeat - 0.02 || beatNow > lastBeat + 4.0)
        {
            releaseAll (off);
            nextStep = (long long) std::ceil (beatNow / sb - 1.0e-9);
            pos = 0;
            initialized = true;
        }
        lastBeat = beatEnd;

        if (sounding && offBeat < beatEnd) releaseAll (off);

        if (held.empty())
        {
            if (sounding) releaseAll (off);
            return;
        }

        auto trig = [&] (long long k) { return (double) k * sb + ((k & 1) ? (double) cfg.swing * sb : 0.0); };

        while (trig (nextStep) < beatEnd)
        {
            const double tb = trig (nextStep);
            if (sounding) releaseAll (off);

            // séquence de notes (avec octaves)
            std::vector<Held> seq;
            std::vector<Held> base = held;
            if (cfg.mode != 4)
                std::sort (base.begin(), base.end(), [] (const Held& a, const Held& b) { return a.note < b.note; });
            for (int o = 0; o < std::max (1, cfg.octaves); ++o)
                for (const auto& h : base) seq.push_back ({ h.note + 12 * o, h.vel });

            const int n = (int) seq.size();
            if (cfg.mode == 5)
            {
                for (const auto& h : seq) { on (h.note, h.vel); cur.push_back (h.note); }
            }
            else
            {
                int idx = 0;
                switch (cfg.mode)
                {
                    case 1: idx = n - 1 - (int) (pos % n); break;
                    case 2: { const int period = std::max (1, 2 * n - 2); const int p = (int) (pos % period); idx = p < n ? p : period - p; break; }
                    case 3: rngState = rngState * 1664525u + 1013904223u; idx = (int) ((rngState >> 8) % (uint32_t) n); break;
                    default: idx = (int) (pos % n); break;
                }
                on (seq[(size_t) idx].note, seq[(size_t) idx].vel);
                cur.push_back (seq[(size_t) idx].note);
            }

            sounding = true;
            offBeat = tb + std::clamp ((double) cfg.gate, 0.05, 1.0) * sb;
            ++pos;
            ++nextStep;
        }
    }

private:
    struct Held { int note; float vel; };

    Cfg cfg;
    std::vector<Held> held;
    std::vector<int> cur;
    int physDown = 0;
    bool initialized = false, sounding = false;
    double lastBeat = 0.0, offBeat = 0.0;
    long long nextStep = 0, pos = 0;
    uint32_t rngState = 12345u;
};
} // namespace dl
