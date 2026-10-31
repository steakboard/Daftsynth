#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Palette rétro / cyberpunk, de la plus présente à la moins présente :
// noir > gris foncé > violet > rose > gris clair > vert fluo
namespace theme
{
const juce::Colour black     (0xff050507);
const juce::Colour darkGray  (0xff14141a);
const juce::Colour darkGray2 (0xff262631);
const juce::Colour purple    (0xff9d3cff);
const juce::Colour pink      (0xffff2e97);
const juce::Colour lightGray (0xffc9ccd6);
const juce::Colour green     (0xff39ff14);

inline juce::Font mono (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace theme
