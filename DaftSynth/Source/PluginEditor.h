#pragma once
#include "PluginProcessor.h"
#include "CyberLookAndFeel.h"
#include "Visualizer.h"

class DaftSynthEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit DaftSynthEditor (DaftSynthProcessor&);
    ~DaftSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectPreset (int index);
    void repaintPanelBorders();

    struct Ctl
    {
        juce::String id;
        bool isChoice = false;
        juce::Slider slider;
        juce::ComboBox combo;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
    };

    struct Section
    {
        juce::String title;
        int row = 0;
        std::vector<Ctl*> ctls;
        juce::Rectangle<int> bounds;
    };

    Ctl* addCtl (const char* id, const char* shortLabel);
    void addSection (const char* title, int row, std::initializer_list<std::pair<const char*, const char*>> items);

    DaftSynthProcessor& proc;
    CyberLookAndFeel lnf;

    std::vector<std::unique_ptr<Ctl>> ctls;
    std::vector<Section> sections;
    Ctl* modeCtl = nullptr;

    LogoComponent logo;
    VisualizerComponent viz;
    juce::Label modeLabel;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    ScanlineOverlay overlay;

    float energy = 0.0f, paintedEnergy = -1.0f;
    int tick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaftSynthEditor)
};
