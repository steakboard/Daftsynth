#pragma once
#include "Theme.h"

class CyberLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CyberLookAndFeel()
    {
        using namespace theme;
        setColour (juce::Slider::textBoxTextColourId, lightGray);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

        setColour (juce::Label::textColourId, lightGray);

        setColour (juce::ComboBox::backgroundColourId, black);
        setColour (juce::ComboBox::textColourId, lightGray);
        setColour (juce::ComboBox::outlineColourId, purple.withAlpha (0.7f));
        setColour (juce::ComboBox::arrowColourId, pink);

        setColour (juce::PopupMenu::backgroundColourId, black);
        setColour (juce::PopupMenu::textColourId, lightGray);
        setColour (juce::PopupMenu::headerTextColourId, pink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, purple.withAlpha (0.55f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextButton::buttonColourId, black);
        setColour (juce::TextButton::textColourOffId, pink);
        setColour (juce::TextButton::textColourOnId, pink);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace theme;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float size = juce::jmin (b.getWidth(), b.getHeight());
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float r = size * 0.5f - 3.0f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        // rail
        juce::Path track;
        track.addCentredArc (cx, cy, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (darkGray2);
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // arc de valeur avec halo néon (violet -> rose)
        if (pos > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (cx, cy, r, r, 0.0f, startAngle, angle, true);
            const auto col = purple.interpolatedWith (pink, pos);
            g.setColour (col.withAlpha (0.07f));
            g.strokePath (val, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col.withAlpha (0.16f));
            g.strokePath (val, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col);
            g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // corps du potard
        const float br = r * 0.70f;
        g.setColour (black);
        g.fillEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f);
        g.setColour (darkGray2);
        g.drawEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f, 1.2f);

        // aiguille
        const float s = std::sin (angle), c = std::cos (angle);
        const juce::Point<float> p1 (cx + br * 0.25f * s, cy - br * 0.25f * c);
        const juce::Point<float> p2 (cx + br * 0.92f * s, cy - br * 0.92f * c);
        g.setColour (lightGray);
        g.drawLine (juce::Line<float> (p1, p2), 2.0f);
        g.setColour (pink);
        g.fillEllipse (p2.x - 1.8f, p2.y - 1.8f, 3.6f, 3.6f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (theme::mono (11.0f));
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        return l;
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return theme::mono (12.0f); }
    juce::Font getPopupMenuFont() override { return theme::mono (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return theme::mono (15.0f, true); }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace theme;
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const float c = 6.0f;
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getY());
        p.lineTo (r.getRight() - c, r.getY());
        p.lineTo (r.getRight(), r.getY() + c);
        p.lineTo (r.getRight(), r.getBottom());
        p.lineTo (r.getX() + c, r.getBottom());
        p.lineTo (r.getX(), r.getBottom() - c);
        p.closeSubPath();

        g.setColour (black);
        g.fillPath (p);
        g.setColour (purple.withAlpha (0.8f));
        g.strokePath (p, juce::PathStrokeType (1.2f));

        juce::Path arrow;
        const float ax = r.getRight() - 14.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 4.0f, ay - 2.5f, ax + 4.0f, ay - 2.5f, ax, ay + 3.0f);
        g.setColour (pink);
        g.fillPath (arrow);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down) override
    {
        using namespace theme;
        const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? purple.withAlpha (0.45f) : black);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (highlighted ? pink : purple.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 3.0f, 1.2f);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (theme::black);
        g.setColour (theme::purple.withAlpha (0.8f));
        g.drawRect (0, 0, width, height, 1);
    }
};
