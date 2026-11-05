#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include <juce_audio_utils/juce_audio_utils.h>
#include "DSP.h"

// Sampler à tranches : import audio, découpage (transitoires / grille / manuel), pitch, reverse et effets par tranche.
// Les données audio sont immuables et publiées par pointeur : le thread audio ne prend jamais de verrou.
namespace dl
{
struct SliceParams
{
    std::atomic<float> pitch { 0.0f }, fine { 0.0f }, level { 1.0f }, pan { 0.0f }, drive { 0.0f }, crush { 16.0f };
    std::atomic<bool> reverse { false };

    void reset()
    {
        pitch = 0.0f; fine = 0.0f; level = 1.0f; pan = 0.0f; drive = 0.0f; crush = 16.0f; reverse = false;
    }

    void copyFrom (const SliceParams& o)
    {
        pitch = o.pitch.load(); fine = o.fine.load(); level = o.level.load(); pan = o.pan.load();
        drive = o.drive.load(); crush = o.crush.load(); reverse = o.reverse.load();
    }
};

struct SampleData
{
    std::shared_ptr<const juce::AudioBuffer<float>> audio;
    double rate = 44100.0;
    std::vector<int> markers;     // débuts de tranches (le premier = début de la 1re tranche, souvent 0)
    juce::String name;

    int length() const { return audio ? audio->getNumSamples() : 0; }
    int numSlices() const { return (int) markers.size(); }
    int sliceStart (int i) const { return markers[(size_t) i]; }
    int sliceEnd (int i) const { return i + 1 < (int) markers.size() ? markers[(size_t) i + 1] : length(); }
};

class SampleEngine
{
public:
    static constexpr int kMaxSlices = 64;
    static constexpr int kVoices = 16;

    struct Params
    {
        float level = 1.0f, tune = 0.0f, attack = 0.002f, release = 0.1f;
        bool loop = false, gate = true, chromatic = false;
        int root = 60, baseNote = 48;
    };

    // ------------------------------------------------------------------ thread de l'interface
    bool loadFile (const juce::File& file, juce::AudioFormatManager& formats, float sensitivity)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples <= 0
            || reader->lengthInSamples > (juce::int64) (reader->sampleRate * 60.0 * 10.0))
            return false;

        const int ch = (int) juce::jmin (2u, reader->numChannels);
        juce::AudioBuffer<float> buf (ch, (int) reader->lengthInSamples);
        reader->read (&buf, 0, buf.getNumSamples(), 0, true, ch > 1);
        loadBuffer (std::move (buf), reader->sampleRate, file.getFileName(), sensitivity);
        return true;
    }

    void loadBuffer (juce::AudioBuffer<float>&& buf, double rate, const juce::String& name, float sensitivity)
    {
        auto d = std::make_shared<SampleData>();
        d->audio = std::make_shared<const juce::AudioBuffer<float>> (std::move (buf));
        d->rate = rate;
        d->name = name;
        d->markers = detectOnsets (*d->audio, rate, sensitivity);
        for (auto& s : slice) s.reset();
        selected = 0;
        publish (d);
    }

    void clear()
    {
        for (auto& v : voices) v.active = false;
        publish (nullptr);
    }

    void autoSlice (float sensitivity)
    {
        if (auto d = getData())
            replaceMarkers (d, detectOnsets (*d->audio, d->rate, sensitivity), true);
    }

    void gridSlice (int n)
    {
        auto d = getData();
        if (d == nullptr) return;
        n = juce::jlimit (1, kMaxSlices, n);
        std::vector<int> m;
        for (int i = 0; i < n; ++i) m.push_back ((int) ((juce::int64) d->length() * i / n));
        replaceMarkers (d, m, true);
    }

    void clearSlices()
    {
        if (auto d = getData()) replaceMarkers (d, { d->markers.empty() ? 0 : d->markers.front() }, true);
    }

    void setMarkers (const std::vector<int>& m)
    {
        if (auto d = getData()) replaceMarkers (d, m, false);
    }

    void insertMarker (int pos)
    {
        auto d = getData();
        if (d == nullptr || d->numSlices() >= kMaxSlices) return;
        auto m = d->markers;
        pos = juce::jlimit (m.front() + 256, d->length() - 1, pos);
        for (int x : m) if (std::abs (x - pos) < 256) return;
        const auto it = std::upper_bound (m.begin(), m.end(), pos);
        const int k = (int) (it - m.begin());   // indice de la nouvelle tranche
        m.insert (it, pos);
        for (int s = (int) m.size() - 1; s > k; --s) slice[s].copyFrom (slice[s - 1]);   // la nouvelle tranche hérite de celle qu'elle coupe
        replaceMarkers (d, m, false);
    }

    void removeMarker (int idx)
    {
        auto d = getData();
        if (d == nullptr || idx <= 0 || idx >= d->numSlices()) return;
        auto m = d->markers;
        m.erase (m.begin() + idx);
        for (int s = idx; s < (int) m.size(); ++s) slice[s].copyFrom (slice[s + 1]);
        if (selected.load() >= (int) m.size()) selected = (int) m.size() - 1;
        replaceMarkers (d, m, false);
    }

    void moveMarker (int idx, int pos)
    {
        auto d = getData();
        if (d == nullptr || idx <= 0 || idx >= d->numSlices()) return;
        auto m = d->markers;
        const int lo = m[(size_t) idx - 1] + 256;
        const int hi = (idx + 1 < (int) m.size() ? m[(size_t) idx + 1] : d->length()) - 256;
        if (hi < lo) return;
        m[(size_t) idx] = juce::jlimit (lo, hi, pos);
        replaceMarkers (d, m, false);
    }

    std::shared_ptr<const SampleData> getData() const
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        return current;
    }

    // ------------------------------------------------------------------ thread audio
    void prepare (double sr) { sampleRate = sr; allOff(); }
    void setParams (const Params& p) { P = p; }

    void noteOn (int note, float vel)
    {
        const auto* d = live.load (std::memory_order_acquire);
        if (d == nullptr || d->length() == 0) return;

        int sl = P.chromatic ? selected.load() : note - P.baseNote;
        if (sl < 0 || sl >= d->numSlices()) return;

        const auto& sp = slice[sl];
        const float semis = (P.chromatic ? (float) (note - P.root) : 0.0f) + P.tune + sp.pitch.load() + sp.fine.load() / 100.0f;

        int idx = -1;
        for (int i = 0; i < kVoices; ++i)
            if (! voices[i].active) { idx = i; break; }
        if (idx < 0)
        {
            uint32_t oldest = UINT32_MAX;
            for (int i = 0; i < kVoices; ++i)
                if (voices[i].age < oldest) { oldest = voices[i].age; idx = i; }
        }

        auto& v = voices[idx];
        v.active = true;
        v.releasing = false;
        v.note = note;
        v.slice = sl;
        v.age = ++counter;
        v.reverse = sp.reverse.load();
        v.pos = v.reverse ? (double) d->sliceEnd (sl) - 1.0 : (double) d->sliceStart (sl);
        v.inc = d->rate / sampleRate * std::pow (2.0, (double) semis / 12.0);
        v.vel = vel;
        v.env = 0.0f;
        v.gain = sp.level.load();
        const float pan = juce::jlimit (-1.0f, 1.0f, sp.pan.load());
        v.gl = std::cos ((pan + 1.0f) * kPi * 0.25f);
        v.gr = std::sin ((pan + 1.0f) * kPi * 0.25f);
        v.drive = sp.drive.load();
        v.crush = sp.crush.load();
    }

    void noteOff (int note)
    {
        if (! P.gate) return;
        for (auto& v : voices)
            if (v.active && v.note == note) v.releasing = true;
    }

    void allOff()
    {
        for (auto& v : voices) v.active = false;
    }

    // Ajoute le signal du sampler dans L[0..n) et R[0..n)
    void render (float* L, float* R, int n)
    {
        const auto* d = live.load (std::memory_order_acquire);
        bool any = false;
        for (auto& v : voices) any = any || v.active;

        if (d == nullptr || d->length() == 0 || ! any)
        {
            for (int v = 0; v < kVoices; ++v) uiPos[(size_t) v].store (-1);
            return;
        }

        const float sr = (float) sampleRate;
        const float atkStep = 1.0f / std::max (1.0f, P.attack * sr);
        const float relStep = 1.0f / std::max (1.0f, P.release * sr);
        const float fadeN = std::max (16.0f, 0.002f * (float) d->rate);
        const auto& buf = *d->audio;
        const int len = buf.getNumSamples();
        const bool stereo = buf.getNumChannels() > 1;
        const float* srcL = buf.getReadPointer (0);
        const float* srcR = buf.getReadPointer (stereo ? 1 : 0);

        for (int v = 0; v < kVoices; ++v)
        {
            auto& vo = voices[v];
            if (! vo.active) { uiPos[(size_t) v].store (-1); continue; }
            if (vo.slice >= d->numSlices()) { vo.active = false; continue; }

            const int st = d->sliceStart (vo.slice), en = d->sliceEnd (vo.slice);

            for (int i = 0; i < n; ++i)
            {
                if (vo.releasing)
                {
                    vo.env -= relStep;
                    if (vo.env <= 0.0f) { vo.active = false; break; }
                }
                else if (vo.env < 1.0f)
                {
                    vo.env = std::min (1.0f, vo.env + atkStep);
                }

                if (vo.pos < (double) st || vo.pos >= (double) en)
                {
                    if (P.loop) vo.pos = vo.reverse ? (double) en - 1.0 : (double) st;
                    else { vo.active = false; break; }
                }

                const int ip = (int) vo.pos;
                const float fr = (float) (vo.pos - (double) ip);
                const int i0 = std::max (0, ip - 1), i2 = std::min (len - 1, ip + 1), i3 = std::min (len - 1, ip + 2), i1 = std::min (len - 1, ip);
                float l = hermite (srcL[i0], srcL[i1], srcL[i2], srcL[i3], fr);
                float r = stereo ? hermite (srcR[i0], srcR[i1], srcR[i2], srcR[i3], fr) : l;

                // effets de la tranche : saturation puis réduction de résolution
                if (vo.drive > 0.001f)
                {
                    const float g = 1.0f + vo.drive * 12.0f, k = 1.2f / std::sqrt (g);
                    l = std::tanh (l * g) * k;
                    r = std::tanh (r * g) * k;
                }
                if (vo.crush < 15.5f)
                {
                    const float lv = std::exp2 (vo.crush - 1.0f);
                    l = std::round (l * lv) / lv;
                    r = std::round (r * lv) / lv;
                }

                // fondu court avant la fin de la tranche pour éviter les clics
                const float toEnd = vo.reverse ? (float) (vo.pos - (double) st) : (float) ((double) en - vo.pos);
                const float fade = P.loop ? 1.0f : std::min (1.0f, toEnd / fadeN);

                const float amp = vo.env * vo.vel * vo.gain * fade * P.level;
                L[i] += l * vo.gl * 1.4142f * amp;
                R[i] += r * vo.gr * 1.4142f * amp;

                vo.pos += vo.reverse ? -vo.inc : vo.inc;
            }
            uiPos[(size_t) v].store (vo.active ? (int) vo.pos : -1);
        }
    }

    // ------------------------------------------------------------------ données partagées avec l'interface
    SliceParams slice[kMaxSlices];
    std::atomic<int> selected { 0 };
    std::array<std::atomic<int>, kVoices> uiPos;

    SampleEngine() { for (auto& u : uiPos) u = -1; }

    // détection de transitoires : cherche les hausses d'énergie par rapport à la moyenne des instants précédents
    static std::vector<int> detectOnsets (const juce::AudioBuffer<float>& a, double rate, float sens)
    {
        std::vector<int> out { 0 };
        const int n = a.getNumSamples();
        const int hop = 256;
        const int hops = n / hop;
        if (hops < 4) return out;

        std::vector<float> logE ((size_t) hops);
        for (int h = 0; h < hops; ++h)
        {
            double s = 0.0;
            for (int j = 0; j < hop; ++j)
            {
                float m = a.getSample (0, h * hop + j);
                if (a.getNumChannels() > 1) m = 0.5f * (m + a.getSample (1, h * hop + j));
                s += (double) m * m;
            }
            logE[(size_t) h] = 20.0f * std::log10 (std::sqrt ((float) (s / hop)) + 1.0e-6f);
        }

        const float thr = 12.0f - 9.0f * juce::jlimit (0.0f, 1.0f, sens);
        const int minGap = std::max (1, (int) (0.07 * rate / hop));
        struct Cand { int pos; float strength; };
        std::vector<Cand> cands;
        int last = -minGap;

        for (int h = 1; h < hops; ++h)
        {
            float ref = 0.0f;
            const int from = std::max (0, h - 10);
            for (int j = from; j < h; ++j) ref += logE[(size_t) j];
            ref /= (float) (h - from);
            const float diff = logE[(size_t) h] - ref;

            if (diff > thr && logE[(size_t) h] > -48.0f && h - last >= minGap)
            {
                const int pos = std::max (1, h - 1) * hop;
                if (pos > 1024) cands.push_back ({ pos, diff });
                last = h;
            }
        }

        if ((int) cands.size() > kMaxSlices - 1)
        {
            std::sort (cands.begin(), cands.end(), [] (const Cand& x, const Cand& y) { return x.strength > y.strength; });
            cands.resize ((size_t) kMaxSlices - 1);
            std::sort (cands.begin(), cands.end(), [] (const Cand& x, const Cand& y) { return x.pos < y.pos; });
        }
        // silence au début du fichier : la première tranche commence à la première attaque
        if (! cands.empty())
        {
            bool leadSilent = true;
            const int firstHop = cands.front().pos / hop;
            for (int h = 0; h < firstHop - 1; ++h) leadSilent = leadSilent && logE[(size_t) h] < -50.0f;
            if (leadSilent) out.clear();
        }
        for (const auto& c : cands) out.push_back (c.pos);
        if (out.empty()) out.push_back (0);
        return out;
    }

private:
    struct Voice
    {
        bool active = false, releasing = false, reverse = false;
        int note = 0, slice = 0;
        uint32_t age = 0;
        double pos = 0.0, inc = 1.0;
        float vel = 1.0f, env = 0.0f, gain = 1.0f, gl = 1.0f, gr = 1.0f, drive = 0.0f, crush = 16.0f;
    };

    static float hermite (float xm1, float x0, float x1, float x2, float t)
    {
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }

    void replaceMarkers (const std::shared_ptr<const SampleData>& d, std::vector<int> m, bool resetSlices)
    {
        auto nd = std::make_shared<SampleData> (*d);
        std::sort (m.begin(), m.end());
        m.erase (std::unique (m.begin(), m.end()), m.end());
        if (m.empty()) m.push_back (0);
        while ((int) m.size() > kMaxSlices) m.pop_back();
        nd->markers = std::move (m);
        if (resetSlices)
        {
            for (auto& s : slice) s.reset();
            selected = 0;
        }
        if (selected.load() >= nd->numSlices()) selected = nd->numSlices() - 1;
        publish (nd);
    }

    void publish (std::shared_ptr<const SampleData> nd)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        const auto now = juce::Time::getMillisecondCounter();
        if (current) graveyard.push_back ({ current, now });
        current = std::move (nd);
        live.store (current.get(), std::memory_order_release);
        // l'ancien contenu reste en vie 1,5 s : le thread audio ne peut plus l'utiliser après
        graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                         [now] (const Old& o) { return now - o.time > 1500u; }), graveyard.end());
    }

    struct Old { std::shared_ptr<const SampleData> data; juce::uint32 time; };

    mutable juce::SpinLock lock;
    std::shared_ptr<const SampleData> current;
    std::vector<Old> graveyard;
    std::atomic<const SampleData*> live { nullptr };

    Params P;
    Voice voices[kVoices];
    double sampleRate = 44100.0;
    uint32_t counter = 0;
};
} // namespace dl
