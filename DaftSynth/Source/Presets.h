#pragma once
#include <utility>
#include <vector>

// Presets d'usine. Chaque preset ne liste que les paramètres qui diffèrent des valeurs par défaut.
// Les valeurs de "mode" : 0 = Poly (Juno), 1 = Mono (Moog), 2 = Acid (303), 3 = Drums
struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values;
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
