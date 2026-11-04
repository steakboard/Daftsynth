#pragma once
#include "PluginProcessor.h"
#include "CyberLookAndFeel.h"
#include "Visualizer.h"
#include "WaveDisplay.h"
#include "SampleView.h"
#include "MorphPad.h"

class DaftSynthEditor : public juce::AudioProcessorEditor,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit DaftSynthEditor (DaftSynthProcessor&);
    ~DaftSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

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
        bool wave = false;
        bool grid = false;
        bool algo = false;
        bool morph = false;
        bool steps = false;
        int extra = 0;
        std::vector<Ctl*> ctls;
        juce::Rectangle<int> bounds;
    };

    Ctl* addCtl (const juce::String& id, const juce::String& shortLabel, bool linear = false, bool vertical = false);
    Ctl* addManualCtl (const juce::String& label, double lo, double hi, double def, double interval, const juce::String& suffix,
                       std::function<void (double)> onChange);
    Ctl* addManualChoice (const juce::String& label, const juce::StringArray& items, std::function<void (int)> onChange);
    void importSample (const juce::File& f);
    void syncSliceControls();
    void updateSampleInfo();
    void addSection (const char* title, int page, int row, std::initializer_list<std::pair<const char*, const char*>> items);
    void addMatrix();

    DaftSynthProcessor& proc;
    CyberLookAndFeel lnf;

    std::vector<std::unique_ptr<Ctl>> ctls;
    std::vector<Section> sections;
    Ctl* modeCtl = nullptr;

    WaveDisplay waveView;
    LogoComponent logo;
    VisualizerComponent viz;
    juce::Label modeLabel;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::TextButton pageBtn[8];
    int pageOf[8] = { 7, 0, 1, 2, 3, 4, 5, 6 };
    MorphPad morphPad;
    juce::TextButton snapBtn[4] = { juce::TextButton ("A"), juce::TextButton ("B"), juce::TextButton ("C"), juce::TextButton ("D") };
    juce::TextButton saveBtn { "SAVE" }, menuBtn { "..." };
    juce::Array<juce::File> userFiles;
    void rebuildPresetMenu();
    void showPresetMenu();
    void askSavePreset();
    SampleView sampleView;
    juce::TextButton importBtn { "IMPORT" }, autoBtn { "AUTO" }, gridBtn { "GRID" }, clearBtn { "CLEAR" }, recBtn { "REC" };
    juce::Label sampleInfo;
    std::unique_ptr<juce::FileChooser> chooser;
    Ctl* slCtl[7] = {};
    bool syncing = false;
    int lastSelShown = -1;
    juce::ComboBox algoBox;
    juce::Rectangle<int> gridArea;
    juce::TextButton diceBtn { "DICE" }, mutBtn { "MUT" };
    juce::TooltipWindow tooltipWindow { this, 500 };
    ScanlineOverlay overlay;

    int page = 0;
    float energy = 0.0f, paintedEnergy = -1.0f;
    int tick = 0;
    juce::Rectangle<int> matrixHeaderArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaftSynthEditor)
};
