#pragma once
#include <utility>
#include <vector>

// Presets d'usine. Chaque preset ne liste que les paramètres qui diffèrent des valeurs par défaut.
// Les valeurs de "mode" : 0 = Poly (Juno), 1 = Mono (Moog), 2 = Acid (303), 3 = Drums
// Une assignation de modulation : source (ex. "LFO 1"), destination (id de paramètre, ex. "cutoff"), quantité -1..1
struct PresetMod
{
    const char* src;
    const char* dst;
    float amount;
};

struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values;
    std::vector<PresetMod> mods;
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets = {
        // ------------------------------------------------------------ BASSES
        { "Bass", "Acid 303 Squelch",
          { { "mode", 2 }, { "saw", 1.0 }, { "cutoff", 450 }, { "res", 0.82 }, { "fenv", 0.85 }, { "keytrack", 0.35 },
            { "fA", 0.002 }, { "fD", 0.22 }, { "fS", 0.0 }, { "aA", 0.002 }, { "aD", 0.4 }, { "aS", 0.85 }, { "aR", 0.06 },
            { "accent", 0.85 }, { "drive", 0.25 }, { "dist", 0.2 }, { "delayMix", 0.18 }, { "delayTime", 375 }, { "delayFb", 0.35 } } },

        { "Bass", "Acid 303 Square",
          { { "mode", 2 }, { "saw", 0.0 }, { "pulse", 1.0 }, { "cutoff", 600 }, { "res", 0.75 }, { "fenv", 0.8 }, { "keytrack", 0.35 },
            { "fA", 0.002 }, { "fD", 0.3 }, { "fS", 0.0 }, { "aA", 0.002 }, { "aS", 0.85 }, { "aR", 0.06 },
            { "accent", 0.7 }, { "dist", 0.35 } } },

        { "Bass", "Acid Rubber Bass",
          { { "mode", 2 }, { "saw", 1.0 }, { "cutoff", 300 }, { "res", 0.5 }, { "fenv", 0.6 }, { "drive", 0.1 },
            { "fA", 0.002 }, { "fD", 0.18 }, { "fS", 0.0 }, { "aA", 0.002 }, { "aD", 0.2 }, { "aS", 0.6 }, { "aR", 0.08 }, { "accent", 0.5 } } },

        { "Bass", "Moog Funk Bass",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.7 }, { "pulse", 0.3 }, { "sub", 0.5 }, { "osc2", 0.5 }, { "osc2Detune", 6 },
            { "cutoff", 450 }, { "res", 0.3 }, { "drive", 0.2 }, { "fenv", 0.55 }, { "fD", 0.2 }, { "fS", 0.15 },
            { "aD", 0.3 }, { "aS", 0.6 }, { "aR", 0.1 }, { "keytrack", 0.4 } } },

        { "Bass", "Sub Pump Bass",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.3 }, { "sub", 1.0 }, { "cutoff", 350 }, { "res", 0.15 }, { "fenv", 0.3 },
            { "fD", 0.25 }, { "aS", 0.9 }, { "aR", 0.12 }, { "pump", 0.7 }, { "pumpRate", 1 }, { "pumpRel", 0.55 } } },

        { "Bass", "MC-202 Mono",
          { { "mode", 1 }, { "saw", 0.5 }, { "pulse", 0.8 }, { "pw", 0.45 }, { "cutoff", 900 }, { "res", 0.55 }, { "fenv", 0.5 },
            { "fD", 0.25 }, { "fS", 0.1 }, { "glide", 0.08 }, { "aS", 0.8 }, { "aR", 0.1 }, { "dist", 0.15 } } },

        { "Bass", "Distorted House Bass",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 1.0 }, { "cutoff", 700 }, { "res", 0.45 }, { "drive", 0.3 }, { "fenv", 0.4 },
            { "fD", 0.15 }, { "fS", 0.3 }, { "aS", 0.8 }, { "dist", 0.3 } } },

        // ------------------------------------------------------------ LEADS
        { "Lead", "Da Funk Style Lead",
          { { "mode", 1 }, { "saw", 0.9 }, { "pulse", 0.3 }, { "pw", 0.35 }, { "osc2", 0.6 }, { "osc2Detune", 8 },
            { "cutoff", 1600 }, { "res", 0.55 }, { "drive", 0.5 }, { "fenv", 0.6 }, { "fD", 0.35 }, { "fS", 0.3 },
            { "aS", 0.9 }, { "aR", 0.15 }, { "glide", 0.08 }, { "dist", 0.5 },
            { "phaserMix", 0.35 }, { "phaserRate", 0.3 }, { "phaserDepth", 0.8 }, { "phaserFb", 0.5 },
            { "delayMix", 0.15 }, { "delayTime", 300 }, { "delayFb", 0.3 } } },

        { "Lead", "Smooth Vibrato Lead",
          { { "mode", 1 }, { "saw", 0.6 }, { "pulse", 0.5 }, { "pw", 0.4 }, { "osc2", 0.5 }, { "osc2Detune", 7 },
            { "cutoff", 5000 }, { "res", 0.25 }, { "fenv", 0.3 }, { "aA", 0.01 }, { "aS", 0.85 }, { "aR", 0.25 },
            { "lfoRate", 5.5 }, { "lfoPitch", 0.12 }, { "glide", 0.05 }, { "chorus", 1 },
            { "delayMix", 0.22 }, { "delayTime", 380 }, { "delayFb", 0.35 }, { "reverbMix", 0.2 } } },

        { "Lead", "Glass PWM Lead",
          { { "mode", 0 }, { "saw", 0.0 }, { "pulse", 0.8 }, { "pw", 0.3 }, { "osc2", 0.6 }, { "osc2Semi", 12 }, { "osc2Detune", 4 },
            { "cutoff", 7000 }, { "res", 0.2 }, { "lfoShape", 1 }, { "lfoRate", 0.5 }, { "lfoPwm", 0.6 },
            { "aA", 0.02 }, { "aR", 0.5 }, { "chorus", 2 }, { "reverbMix", 0.25 } } },

        { "Lead", "Mono Square Lead",
          { { "mode", 1 }, { "saw", 0.0 }, { "pulse", 1.0 }, { "cutoff", 3000 }, { "res", 0.3 }, { "fenv", 0.4 },
            { "fD", 0.2 }, { "fS", 0.5 }, { "glide", 0.04 }, { "aS", 0.9 }, { "delayMix", 0.15 } } },

        // ------------------------------------------------------------ ACCORDS ET PADS
        { "Chords / Pads", "Juno Chords (Chorus II)",
          { { "mode", 0 }, { "saw", 0.7 }, { "pulse", 0.5 }, { "sub", 0.3 }, { "cutoff", 4500 }, { "res", 0.1 }, { "fenv", 0.15 },
            { "aA", 0.02 }, { "aD", 0.6 }, { "aS", 0.6 }, { "aR", 0.5 }, { "hpf", 0.15 },
            { "lfoShape", 1 }, { "lfoRate", 0.6 }, { "lfoPwm", 0.5 }, { "chorus", 2 }, { "volume", 0.65 } } },

        { "Chords / Pads", "Juno Warm Pad",
          { { "mode", 0 }, { "saw", 0.6 }, { "pulse", 0.4 }, { "sub", 0.2 }, { "cutoff", 2200 }, { "res", 0.1 }, { "fenv", 0.1 },
            { "aA", 0.7 }, { "aD", 1.0 }, { "aS", 0.8 }, { "aR", 1.4 }, { "chorus", 3 },
            { "lfoShape", 1 }, { "lfoRate", 0.4 }, { "lfoPwm", 0.5 }, { "reverbMix", 0.3 }, { "reverbSize", 0.7 } } },

        { "Chords / Pads", "Super Jupiter Strings",
          { { "mode", 0 }, { "saw", 0.8 }, { "osc2", 0.8 }, { "osc2Detune", 12 }, { "cutoff", 6000 }, { "res", 0.05 }, { "fenv", 0.1 },
            { "aA", 0.35 }, { "aS", 0.9 }, { "aR", 0.9 }, { "chorus", 2 },
            { "phaserMix", 0.3 }, { "phaserRate", 0.2 }, { "phaserDepth", 0.6 }, { "phaserFb", 0.3 }, { "reverbMix", 0.3 } } },

        { "Chords / Pads", "Prophet 600 Brass",
          { { "mode", 0 }, { "saw", 1.0 }, { "osc2", 1.0 }, { "osc2Detune", 5 }, { "cutoff", 1200 }, { "res", 0.15 }, { "fenv", 0.55 },
            { "fA", 0.08 }, { "fD", 0.4 }, { "fS", 0.6 }, { "aA", 0.04 }, { "aS", 0.9 }, { "aR", 0.15 }, { "chorus", 1 } } },

        { "Chords / Pads", "Filtered Disco Chords",
          { { "mode", 0 }, { "saw", 0.8 }, { "pulse", 0.4 }, { "cutoff", 1400 }, { "res", 0.35 }, { "fenv", 0.2 },
            { "aA", 0.01 }, { "aS", 0.8 }, { "aR", 0.3 }, { "lfoRate", 0.25 }, { "lfoCutoff", 0.6 }, { "chorus", 2 },
            { "pump", 0.7 }, { "pumpRate", 1 }, { "pumpRel", 0.5 }, { "phaserMix", 0.3 }, { "phaserRate", 0.15 }, { "volume", 0.65 } } },

        { "Chords / Pads", "Tape Phaser Pad",
          { { "mode", 0 }, { "saw", 0.7 }, { "osc2", 0.5 }, { "osc2Detune", 9 }, { "cutoff", 3000 }, { "aA", 0.5 }, { "aR", 1.2 },
            { "phaserMix", 0.9 }, { "phaserRate", 0.15 }, { "phaserDepth", 0.9 }, { "phaserFb", 0.75 }, { "chorus", 3 } } },

        // ------------------------------------------------------------ STABS
        { "Stabs", "House Stab",
          { { "mode", 0 }, { "saw", 0.7 }, { "pulse", 0.5 }, { "osc2", 0.5 }, { "osc2Detune", 8 }, { "cutoff", 900 }, { "res", 0.3 },
            { "fenv", 0.6 }, { "fA", 0.002 }, { "fD", 0.2 }, { "fS", 0.0 }, { "aA", 0.002 }, { "aD", 0.25 }, { "aS", 0.0 }, { "aR", 0.15 },
            { "chorus", 1 }, { "delayMix", 0.25 }, { "delayTime", 375 }, { "delayFb", 0.4 } } },

        { "Stabs", "Phased Stab",
          { { "mode", 0 }, { "saw", 0.7 }, { "pulse", 0.5 }, { "osc2", 0.5 }, { "osc2Detune", 8 }, { "cutoff", 900 }, { "res", 0.3 },
            { "fenv", 0.6 }, { "fA", 0.002 }, { "fD", 0.2 }, { "fS", 0.0 }, { "aA", 0.002 }, { "aD", 0.25 }, { "aS", 0.0 }, { "aR", 0.15 },
            { "phaserMix", 0.7 }, { "phaserRate", 0.25 }, { "phaserDepth", 0.8 }, { "phaserFb", 0.6 } } },

        // ------------------------------------------------------------ SAMPLER / LO-FI
        { "Lo-Fi", "SP-1200 Grit Chord",
          { { "mode", 0 }, { "saw", 0.6 }, { "pulse", 0.5 }, { "cutoff", 3500 }, { "aA", 0.01 }, { "aD", 0.5 }, { "aS", 0.5 }, { "aR", 0.3 },
            { "crushBits", 12 }, { "crushRate", 26040 }, { "dist", 0.15 }, { "chorus", 1 } } },

        { "Lo-Fi", "8-Bit Crunch Stab",
          { { "mode", 0 }, { "saw", 0.0 }, { "pulse", 1.0 }, { "cutoff", 6000 }, { "aD", 0.15 }, { "aS", 0.0 },
            { "crushBits", 8 }, { "crushRate", 11025 } } },

        // ------------------------------------------------------------ FX
        { "FX", "Noise Sweep",
          { { "mode", 0 }, { "saw", 0.0 }, { "noise", 1.0 }, { "cutoff", 300 }, { "res", 0.6 }, { "fenv", 1.0 },
            { "fA", 3.0 }, { "fD", 1.0 }, { "fS", 1.0 }, { "aA", 2.0 }, { "aS", 1.0 }, { "aR", 1.5 }, { "reverbMix", 0.3 } } },

        // ------------------------------------------------------------ MODULATION (LFO / enveloppes / synchro tempo)
        { "Modulation", "Wobble Bass (LFO 1/8)",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 1.0 }, { "sub", 0.5 }, { "cutoff", 300 }, { "res", 0.6 }, { "drive", 0.3 },
            { "fenv", 0.0 }, { "aS", 0.9 }, { "dist", 0.4 }, { "distType", 2 },
            { "lfo1Shape", 1 }, { "lfo1Sync", 4 } },
          { { "LFO 1", "cutoff", 0.55f } } },

        { "Modulation", "Trance Gate Pad (1/16)",
          { { "mode", 0 }, { "saw", 0.7 }, { "osc2", 0.7 }, { "osc2Detune", 10 }, { "cutoff", 5000 }, { "aA", 0.1 }, { "aR", 0.6 },
            { "chorus", 2 }, { "reverbMix", 0.25 }, { "lfo1Shape", 4 }, { "lfo1Sync", 5 }, { "volume", 0.7 } },
          { { "LFO 1", "volume", 0.45f } } },

        { "Modulation", "Evolving Pad (random)",
          { { "mode", 0 }, { "saw", 0.5 }, { "pulse", 0.5 }, { "osc2", 0.5 }, { "osc2Detune", 8 }, { "cutoff", 2500 }, { "res", 0.25 },
            { "aA", 0.8 }, { "aR", 1.5 }, { "chorus", 3 }, { "reverbMix", 0.3 }, { "reverbSize", 0.8 },
            { "lfo1Shape", 6 }, { "lfo1Rate", 0.3 }, { "lfo2Shape", 1 }, { "lfo2Rate", 0.11 } },
          { { "LFO 1", "cutoff", 0.3f }, { "LFO 2", "pw", 0.35f }, { "LFO 1", "osc2Detune", 0.25f } } },

        { "Modulation", "Envelope Pluck",
          { { "mode", 0 }, { "saw", 0.8 }, { "pulse", 0.3 }, { "cutoff", 1800 }, { "res", 0.3 }, { "fenv", 0.0 },
            { "aD", 0.35 }, { "aS", 0.0 }, { "aR", 0.2 }, { "env1A", 0.001 }, { "env1D", 0.3 }, { "env1S", 0.0 },
            { "delayMix", 0.2 }, { "delaySync", 5 }, { "delayFb", 0.4 } },
          { { "Env 1", "cutoff", 0.6f }, { "Env 1", "res", 0.15f } } },

        { "Modulation", "Velocity Brass",
          { { "mode", 0 }, { "saw", 1.0 }, { "osc2", 1.0 }, { "osc2Detune", 5 }, { "cutoff", 700 }, { "res", 0.15 },
            { "aA", 0.04 }, { "aS", 0.9 }, { "aR", 0.15 }, { "chorus", 1 } },
          { { "Velocity", "cutoff", 0.5f }, { "Mod Wheel", "cutoff", 0.3f } } },

        { "Modulation", "Macro Morph Lead",
          { { "mode", 1 }, { "saw", 0.8 }, { "pulse", 0.4 }, { "osc2", 0.5 }, { "osc2Detune", 9 }, { "cutoff", 1200 }, { "res", 0.4 },
            { "aS", 0.9 }, { "glide", 0.06 }, { "delayMix", 0.2 }, { "delaySync", 4 }, { "macro1", 0.0 } },
          { { "Macro 1", "cutoff", 0.7f }, { "Macro 1", "dist", 0.5f }, { "Macro 1", "res", 0.3f } } },

        // ------------------------------------------------------------ GROWL / FM / WOW (techniques type dubstep : FM + voyelle qui wobble + distorsion)
        { "Growl / FM", "Growl Bass (FM + Vowel)",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.2 }, { "sub", 0.5 }, { "fmLevel", 1.0 }, { "fmAmt", 5.0 }, { "fmRatio", 1.0 }, { "fmFb", 0.3 },
            { "cutoff", 6000 }, { "aA", 0.002 }, { "aS", 0.9 }, { "aR", 0.1 }, { "dist", 0.6 }, { "distType", 2 },
            { "wowType", 7 }, { "wowFreq", 1000 }, { "wowRes", 0.6 }, { "wowVowel", 0.5 }, { "wowDrive", 0.4 }, { "wowMix", 1.0 },
            { "wowLfoShape", 1 }, { "wowLfoSync", 4 }, { "wowLfoDepth", 0.8 }, { "lfo1Shape", 1 }, { "lfo1Sync", 4 } },
          { { "LFO 1", "fmAmt", 0.3f } } },

        { "Growl / FM", "Vowel Talker Saw",
          { { "mode", 1 }, { "saw", 1.0 }, { "osc2", 0.5 }, { "osc2Detune", 6 }, { "cutoff", 8000 }, { "aS", 0.9 }, { "aR", 0.08 },
            { "wowType", 7 }, { "wowFreq", 1100 }, { "wowRes", 0.7 }, { "wowVowel", 0.3 }, { "wowMix", 1.0 },
            { "wowLfoShape", 0 }, { "wowLfoSync", 3 }, { "wowLfoDepth", 1.0 }, { "dist", 0.35 }, { "distType", 4 } } },

        { "Growl / FM", "Scary Style FM Lead",
          { { "mode", 1 }, { "saw", 0.4 }, { "fmLevel", 1.0 }, { "fmAmt", 3.0 }, { "fmRatio", 2.0 }, { "fmFb", 0.5 }, { "fmEnv", 0.5 },
            { "fenv", 0.5 }, { "fD", 0.3 }, { "fS", 0.4 }, { "cutoff", 3500 }, { "res", 0.3 }, { "aS", 0.9 }, { "glide", 0.05 },
            { "dist", 0.5 }, { "distType", 1 }, { "delayMix", 0.2 }, { "delaySync", 4 }, { "delayFb", 0.35 } } },

        { "Growl / FM", "Wobble LP 1/8 (WOW)",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 1.0 }, { "sub", 0.5 }, { "cutoff", 12000 }, { "aS", 0.9 },
            { "wowType", 1 }, { "wowFreq", 400 }, { "wowRes", 0.7 }, { "wowMix", 1.0 }, { "wowDrive", 0.3 },
            { "wowLfoShape", 1 }, { "wowLfoSync", 4 }, { "wowLfoDepth", 0.9 }, { "dist", 0.4 }, { "distType", 2 }, { "wowPos", 0 } } },

        { "Growl / FM", "Bit Growl",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.3 }, { "sub", 0.4 }, { "fmLevel", 1.0 }, { "fmAmt", 7.0 }, { "fmRatio", 1.0 }, { "fmFb", 0.5 },
            { "aS", 0.9 }, { "wowType", 7 }, { "wowFreq", 900 }, { "wowRes", 0.75 }, { "wowVowel", 0.2 }, { "wowMix", 1.0 },
            { "wowLfoSync", 5 }, { "wowLfoDepth", 1.0 }, { "wowLfoShape", 2 }, { "crushBits", 9 }, { "crushRate", 18000 }, { "dist", 0.5 }, { "distType", 3 } } },

        { "Growl / FM", "Comb Metal Lead",
          { { "mode", 1 }, { "saw", 1.0 }, { "cutoff", 9000 }, { "aS", 0.9 }, { "glide", 0.04 },
            { "wowType", 6 }, { "wowFreq", 300 }, { "wowRes", 0.8 }, { "wowMix", 0.8 }, { "wowLfoShape", 1 }, { "wowLfoRate", 0.4 }, { "wowLfoDepth", 0.5 },
            { "dist", 0.3 }, { "reverbMix", 0.2 } } },

        { "Growl / FM", "FM Bell Pad (DX style)",
          { { "mode", 0 }, { "saw", 0.0 }, { "sub", 0.2 }, { "fmLevel", 1.0 }, { "fmAmt", 4.0 }, { "fmRatio", 3.5 }, { "fmEnv", 0.8 },
            { "fenv", 0.6 }, { "fD", 0.8 }, { "fS", 0.1 }, { "cutoff", 9000 }, { "aA", 0.005 }, { "aD", 1.2 }, { "aS", 0.3 }, { "aR", 1.0 },
            { "chorus", 2 }, { "reverbMix", 0.35 }, { "reverbSize", 0.7 } } },

        { "Growl / FM", "Vowel Pad (A-E-I-O-U)",
          { { "mode", 0 }, { "saw", 0.8 }, { "osc2", 0.7 }, { "osc2Detune", 9 }, { "cutoff", 9000 }, { "aA", 0.4 }, { "aS", 0.9 }, { "aR", 1.0 },
            { "wowType", 7 }, { "wowFreq", 1000 }, { "wowRes", 0.45 }, { "wowVowel", 0.5 }, { "wowMix", 1.0 },
            { "wowLfoShape", 1 }, { "wowLfoRate", 0.25 }, { "wowLfoDepth", 1.0 }, { "chorus", 3 }, { "reverbMix", 0.3 } } },

        // ------------------------------------------------------------ WAVETABLES + UNISON (wtTable : 0 Basic, 1 Growl, 2 Digital, 3 PWM, 4 Vowels, 5 FM Sweep, 6 Sync)
        { "Wavetable", "Supersaw Lead",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 0 }, { "wtPos", 0.67 }, { "uniVoices", 7 }, { "uniDetune", 28 },
            { "cutoff", 9000 }, { "aA", 0.01 }, { "aS", 0.8 }, { "aR", 0.3 }, { "chorus", 2 }, { "reverbMix", 0.2 } } },

        { "Wavetable", "Super Square Pad",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 0 }, { "wtPos", 1.0 }, { "uniVoices", 5 }, { "uniDetune", 15 },
            { "cutoff", 5000 }, { "aA", 0.3 }, { "aS", 0.8 }, { "aR", 1.0 }, { "chorus", 3 }, { "reverbMix", 0.35 } } },

        { "Wavetable", "Hyper Reese Bass",
          { { "mode", 1 }, { "saw", 0.0 }, { "sub", 0.5 }, { "wtLevel", 1.0 }, { "wtTable", 0 }, { "wtPos", 0.67 }, { "wtOct", -1 },
            { "uniVoices", 7 }, { "uniDetune", 30 }, { "cutoff", 2500 }, { "res", 0.2 }, { "aS", 0.9 }, { "dist", 0.5 }, { "distType", 1 } } },

        { "Wavetable", "Growl Table Bass",
          { { "mode", 1 }, { "saw", 0.0 }, { "sub", 0.4 }, { "wtLevel", 1.0 }, { "wtTable", 1 }, { "wtPos", 0.2 }, { "wtOct", -1 },
            { "uniVoices", 3 }, { "uniDetune", 8 }, { "cutoff", 8000 }, { "aS", 0.9 }, { "dist", 0.55 }, { "distType", 2 },
            { "lfo1Shape", 1 }, { "lfo1Sync", 4 } },
          { { "LFO 1", "wtPos", 0.4f } } },

        { "Wavetable", "PWM Strings",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 3 }, { "wtPos", 0.5 }, { "uniVoices", 5 }, { "uniDetune", 14 },
            { "cutoff", 7000 }, { "aA", 0.35 }, { "aS", 0.9 }, { "aR", 0.8 }, { "chorus", 2 }, { "lfo2Shape", 1 }, { "lfo2Rate", 0.3 } },
          { { "LFO 2", "wtPos", 0.4f } } },

        { "Wavetable", "Vowel Table Lead",
          { { "mode", 1 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 4 }, { "wtPos", 0.5 }, { "uniVoices", 2 }, { "uniDetune", 6 },
            { "cutoff", 9000 }, { "aS", 0.9 }, { "glide", 0.05 }, { "dist", 0.3 }, { "lfo1Sync", 3 } },
          { { "LFO 1", "wtPos", 0.45f } } },

        { "Wavetable", "Digital Pluck",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 2 }, { "wtPos", 0.35 }, { "uniVoices", 2 }, { "uniDetune", 5 },
            { "cutoff", 12000 }, { "aD", 0.4 }, { "aS", 0.0 }, { "aR", 0.2 }, { "env1D", 0.3 }, { "env1S", 0.0 },
            { "delayMix", 0.2 }, { "delaySync", 4 }, { "delayFb", 0.35 } },
          { { "Env 1", "wtPos", 0.5f } } },

        { "Wavetable", "FM Sweep Keys",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 5 }, { "wtPos", 0.2 }, { "cutoff", 8000 },
            { "aD", 0.8 }, { "aS", 0.3 }, { "aR", 0.6 }, { "env1D", 0.5 }, { "env1S", 0.0 }, { "chorus", 2 }, { "reverbMix", 0.3 } },
          { { "Velocity", "wtPos", 0.5f }, { "Env 1", "wtPos", 0.4f } } },

        { "Wavetable", "Sync Lead (molette)",
          { { "mode", 1 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 6 }, { "wtPos", 0.3 }, { "cutoff", 6000 }, { "aS", 0.9 },
            { "glide", 0.04 }, { "dist", 0.35 }, { "distType", 1 } },
          { { "Mod Wheel", "wtPos", 0.7f }, { "Macro 1", "cutoff", 0.3f } } },

        // ------------------------------------------------------------ MIX : OTT, EQ, boost + limiteur
        { "Boosted", "OTT Supersaw (boosted)",
          { { "mode", 0 }, { "saw", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 0 }, { "wtPos", 0.67 }, { "uniVoices", 7 }, { "uniDetune", 30 },
            { "cutoff", 12000 }, { "aA", 0.01 }, { "aS", 0.85 }, { "aR", 0.3 }, { "chorus", 2 },
            { "ottMix", 0.8 }, { "ottDepth", 0.85 }, { "ottHigh", 3.0 }, { "eqLowGain", 2.0 }, { "eqHighGain", 3.0 }, { "boost", 6.0 } } },

        { "Boosted", "Boosted Growl (OTT + Limiter)",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.2 }, { "sub", 0.5 }, { "fmLevel", 1.0 }, { "fmAmt", 5.0 }, { "fmFb", 0.3 },
            { "cutoff", 6000 }, { "aS", 0.9 }, { "aR", 0.1 }, { "dist", 0.6 }, { "distType", 2 },
            { "wowType", 7 }, { "wowRes", 0.6 }, { "wowVowel", 0.5 }, { "wowDrive", 0.4 }, { "wowMix", 1.0 },
            { "wowLfoShape", 1 }, { "wowLfoSync", 4 }, { "wowLfoDepth", 0.8 },
            { "ottMix", 0.7 }, { "ottDepth", 0.8 }, { "ottLow", 2.0 }, { "boost", 8.0 }, { "limCeil", -0.5 } } },

        { "Boosted", "Wide Flanger Pad",
          { { "mode", 0 }, { "saw", 0.7 }, { "osc2", 0.7 }, { "osc2Detune", 12 }, { "cutoff", 5000 }, { "aA", 0.5 }, { "aS", 0.9 }, { "aR", 1.2 },
            { "flMix", 0.7 }, { "flRate", 0.15 }, { "flDepth", 0.8 }, { "flFb", 0.6 }, { "flDelay", 3.0 },
            { "chorus", 3 }, { "reverbMix", 0.35 }, { "reverbSize", 0.8 }, { "reverbPre", 25 }, { "reverbDamp", 0.5 } } },

        { "Boosted", "Dark Ping-Pong Keys",
          { { "mode", 0 }, { "saw", 0.6 }, { "pulse", 0.4 }, { "cutoff", 2500 }, { "aD", 0.5 }, { "aS", 0.3 }, { "aR", 0.4 },
            { "delayMix", 0.4 }, { "delaySync", 4 }, { "delayFb", 0.55 }, { "delayPing", 1 }, { "delayTone", 1800 },
            { "eqHighGain", -4.0 }, { "eqLowGain", 2.0 }, { "reverbMix", 0.2 } } },

        // ------------------------------------------------------------ FM 6 OPERATEURS (fmMrc : operateur r module par operateur c)
        { "FM 6-Op", "6-Op Growl Stack",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.0 }, { "fm6Level", 1.0 }, { "fm6Index", 3.0 }, { "fmO1", 1.0 },
            { "fmR1", 1.0 }, { "fmR2", 1.0 }, { "fmR3", 2.0 }, { "fmR4", 0.5 },
            { "fmM12", 0.8 }, { "fmM23", 0.8 }, { "fmM34", 0.6 }, { "fmM44", 0.5 },
            { "cutoff", 9000 }, { "aS", 0.9 }, { "aR", 0.1 }, { "dist", 0.5 }, { "distType", 2 },
            { "wowType", 7 }, { "wowRes", 0.6 }, { "wowMix", 1.0 }, { "wowLfoSync", 4 }, { "wowLfoDepth", 0.8 }, { "wowLfoShape", 1 },
            { "lfo1Shape", 1 }, { "lfo1Sync", 4 } },
          { { "LFO 1", "fm6Index", 0.3f } } },

        { "FM 6-Op", "6-Op E-Piano",
          { { "mode", 0 }, { "saw", 0.0 }, { "fm6Level", 1.0 }, { "fm6Index", 2.5 }, { "fmO1", 0.8 }, { "fmO3", 0.5 },
            { "fmR1", 1.0 }, { "fmR2", 1.0 }, { "fmR3", 1.0 }, { "fmR4", 14.0 }, { "fmM12", 0.35 }, { "fmM34", 0.25 },
            { "fmE2", 0.9 }, { "fmE4", 1.0 }, { "fenv", 0.0 }, { "fD", 0.5 }, { "fS", 0.05 }, { "cutoff", 14000 },
            { "aD", 1.2 }, { "aS", 0.25 }, { "aR", 0.5 }, { "chorus", 2 }, { "reverbMix", 0.2 } } },

        { "FM 6-Op", "6-Op Glass Bell",
          { { "mode", 0 }, { "saw", 0.0 }, { "fm6Level", 1.0 }, { "fm6Index", 3.0 }, { "fmO1", 1.0 }, { "fmO3", 0.5 },
            { "fmR1", 1.0 }, { "fmR2", 3.5 }, { "fmR3", 1.0 }, { "fmR4", 7.0 }, { "fmM12", 0.7 }, { "fmM22", 0.5 }, { "fmM34", 0.4 },
            { "fmE2", 0.8 }, { "fmE4", 0.9 }, { "fD", 0.9 }, { "fS", 0.05 }, { "cutoff", 14000 },
            { "aD", 1.5 }, { "aS", 0.2 }, { "aR", 1.2 }, { "reverbMix", 0.35 }, { "reverbSize", 0.7 } } },

        { "FM 6-Op", "6-Op Metal Hit",
          { { "mode", 0 }, { "saw", 0.0 }, { "fm6Level", 1.0 }, { "fm6Index", 4.0 }, { "fmO1", 0.6 }, { "fmO3", 0.6 }, { "fmO5", 0.5 },
            { "fmR1", 1.0 }, { "fmR2", 1.41 }, { "fmR3", 2.75 }, { "fmR4", 3.5 }, { "fmR5", 5.2 }, { "fmR6", 1.0 },
            { "fmM12", 0.6 }, { "fmM34", 0.6 }, { "fmM56", 0.5 }, { "fmM21", 0.2 }, { "fmE2", 0.9 }, { "fmE4", 0.9 }, { "fmE6", 0.9 },
            { "fD", 0.3 }, { "fS", 0.0 }, { "cutoff", 14000 }, { "aD", 0.5 }, { "aS", 0.0 }, { "aR", 0.3 }, { "dist", 0.25 } } },

        { "FM 6-Op", "6-Op Pluck Bass",
          { { "mode", 1 }, { "octave", -1 }, { "saw", 0.0 }, { "sub", 0.4 }, { "fm6Level", 1.0 }, { "fm6Index", 3.5 }, { "fmO1", 1.0 },
            { "fmR1", 1.0 }, { "fmR2", 1.0 }, { "fmM12", 0.9 }, { "fmE2", 0.9 }, { "fD", 0.25 }, { "fS", 0.1 }, { "cutoff", 12000 },
            { "aD", 0.3 }, { "aS", 0.8 }, { "aR", 0.1 }, { "dist", 0.3 } } },

        // ------------------------------------------------------------ ARPEGIATEUR / PAS (joue un accord et laisse tourner)
        { "Arp / Steps", "Arp Pluck 1/16",
          { { "mode", 0 }, { "saw", 0.7 }, { "pulse", 0.4 }, { "cutoff", 2500 }, { "res", 0.3 }, { "fenv", 0.5 }, { "fD", 0.2 }, { "fS", 0.0 },
            { "aD", 0.18 }, { "aS", 0.0 }, { "aR", 0.1 }, { "arpOn", 1 }, { "arpMode", 0 }, { "arpRate", 2 }, { "arpGate", 0.5 }, { "arpOct", 2 },
            { "delayMix", 0.25 }, { "delaySync", 4 }, { "delayFb", 0.4 }, { "chorus", 1 } } },

        { "Arp / Steps", "Arp Up/Down Lead",
          { { "mode", 1 }, { "saw", 0.6 }, { "pulse", 0.5 }, { "cutoff", 5000 }, { "aS", 0.8 }, { "aR", 0.15 },
            { "arpOn", 1 }, { "arpMode", 2 }, { "arpRate", 1 }, { "arpGate", 0.8 }, { "arpOct", 3 }, { "arpSwing", 0.15 },
            { "reverbMix", 0.25 }, { "phaserMix", 0.3 } } },

        { "Arp / Steps", "Step Trance Gate Pad",
          { { "mode", 0 }, { "saw", 0.7 }, { "osc2", 0.7 }, { "osc2Detune", 10 }, { "cutoff", 6000 }, { "aA", 0.05 }, { "aR", 0.4 }, { "volume", 0.3 },
            { "stepLen", 8 }, { "stepSync", 5 }, { "step1", 1.0 }, { "step2", 0.0 }, { "step3", 0.7 }, { "step4", 0.0 },
            { "step5", 1.0 }, { "step6", 0.3 }, { "step7", 0.7 }, { "step8", 0.0 }, { "chorus", 2 }, { "reverbMix", 0.2 } },
          { { "Step Seq", "volume", 0.7f } } },

        { "Arp / Steps", "Step Filter Seq (acid)",
          { { "mode", 2 }, { "saw", 1.0 }, { "cutoff", 300 }, { "res", 0.7 }, { "fenv", 0.3 }, { "fD", 0.2 }, { "fS", 0.0 }, { "aS", 0.85 }, { "aR", 0.06 },
            { "stepLen", 8 }, { "stepSync", 5 }, { "step1", 0.1 }, { "step2", 0.6 }, { "step3", 0.2 }, { "step4", 1.0 },
            { "step5", 0.3 }, { "step6", 0.8 }, { "step7", 0.1 }, { "step8", 0.5 }, { "stepGlide", 0.2 }, { "dist", 0.3 } },
          { { "Step Seq", "cutoff", 0.55f } } },

        // ------------------------------------------------------------ VINTAGE VIVANT (derive analogique, hard sync, ring mod)
        { "Vintage+", "Drifting Juno Brass",
          { { "mode", 0 }, { "saw", 0.8 }, { "pulse", 0.4 }, { "osc2", 0.7 }, { "osc2Detune", 5 }, { "cutoff", 1400 }, { "res", 0.2 }, { "fenv", 0.5 },
            { "fA", 0.06 }, { "fD", 0.4 }, { "fS", 0.5 }, { "aA", 0.03 }, { "aS", 0.9 }, { "aR", 0.25 }, { "drift", 1.0 }, { "chorus", 3 }, { "reverbMix", 0.15 } } },

        { "Vintage+", "Hard Sync Lead (molette)",
          { { "mode", 1 }, { "saw", 0.6 }, { "osc2", 0.9 }, { "osc2Semi", 5 }, { "sync2", 1 }, { "cutoff", 6000 }, { "res", 0.2 },
            { "aS", 0.9 }, { "glide", 0.04 }, { "drift", 0.4 }, { "dist", 0.25 } },
          { { "Mod Wheel", "osc2Semi", 0.45f }, { "LFO 1", "osc2Semi", 0.1f } } },

        { "Vintage+", "Ring Mod Bell",
          { { "mode", 0 }, { "saw", 0.0 }, { "pulse", 0.0 }, { "sub", 0.0 }, { "wtLevel", 1.0 }, { "wtTable", 0 }, { "wtPos", 0.0 },
            { "ringMix", 0.9 }, { "ringRatio", 3.51 }, { "cutoff", 9000 }, { "aD", 1.2 }, { "aS", 0.15 }, { "aR", 1.0 },
            { "drift", 0.3 }, { "reverbMix", 0.35 }, { "reverbSize", 0.7 } } },

        { "Vintage+", "Old Disco Strings (wide)",
          { { "mode", 0 }, { "saw", 0.8 }, { "osc2", 0.8 }, { "osc2Detune", 9 }, { "cutoff", 5500 }, { "aA", 0.4 }, { "aS", 0.9 }, { "aR", 0.9 },
            { "drift", 0.8 }, { "chorus", 2 }, { "phaserMix", 0.3 }, { "phaserRate", 0.2 }, { "flMix", 0.25 }, { "width", 1.5 }, { "reverbMix", 0.3 } } },

        // ------------------------------------------------------------ DRUMS (notes MIDI 35-57)
        { "Drums", "TR-808 Kit",         { { "mode", 3 }, { "kit", 0 } } },
        { "Drums", "TR-909 Kit",         { { "mode", 3 }, { "kit", 1 } } },
        { "Drums", "LinnDrum Kit",       { { "mode", 3 }, { "kit", 2 } } },
        { "Drums", "Oberheim DMX Kit",   { { "mode", 3 }, { "kit", 3 } } },
        { "Drums", "909 Tight",          { { "mode", 3 }, { "kit", 1 }, { "drumDecay", 0.6 }, { "drumTone", 0.6 } } },
        { "Drums", "808 Long Boom",      { { "mode", 3 }, { "kit", 0 }, { "drumDecay", 1.6 }, { "drumTune", -3 } } },
        { "Drums", "909 Pumping Kit",    { { "mode", 3 }, { "kit", 1 }, { "pump", 0.6 }, { "pumpRate", 1 } } },
    };
    return presets;
}
