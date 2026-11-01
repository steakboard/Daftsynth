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
    void setPage (int newPage);

    struct Ctl
    {
        juce::String id;
        bool isChoice = false;
        bool linear = false;
        juce::Slider slider;
        juce::ComboBox combo;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;

        void show (bool v)
        {
            label.setVisible (v && label.getText().isNotEmpty());
            slider.setVisible (v && ! isChoice);
            combo.setVisible (v && isChoice);
        }
    };

    struct Section
    {
        juce::String title;
        int page = 0, row = 0;
        bool matrix = false;
        std::vector<Ctl*> ctls;
        juce::Rectangle<int> bounds;
    };

    Ctl* addCtl (const juce::String& id, const juce::String& shortLabel, bool linear = false);
    void addSection (const char* title, int page, int row, std::initializer_list<std::pair<const char*, const char*>> items);
    void addMatrix();

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
    juce::TextButton pageBtn[3];
    ScanlineOverlay overlay;

    int page = 0;
    float energy = 0.0f, paintedEnergy = -1.0f;
    int tick = 0;
    juce::Rectangle<int> matrixHeaderArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaftSynthEditor)
};
