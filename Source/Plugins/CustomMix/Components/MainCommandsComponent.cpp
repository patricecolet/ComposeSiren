//
// Created by joseph larralde on 25/01/2026.
//


#include "../../../ComposeSirenCore/Components/MainCommandsComponent.h"

//=============================================================================
// Component on the top of the main window
// MainCommandsComponent::MainCommandsComponent(Listener* l/*SirenePlugAudioProcessor& p*/)
MainCommandsComponent::MainCommandsComponent(juce::AudioProcessorValueTreeState& vts)
    : apvts(vts)
    // : listener(l)
    // : audioProcessor(p)
{
    // Master Volume (CC7 canal 16)
    masterVolumeLabel.setText("Master Vol (CC7 ch16)", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType(juce::Justification::centredLeft);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(masterVolumeLabel);

    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setRange(0.0, 127.0, 1.0);
    // Convertir le gain actuel en valeur CC (formule inverse de dbtorms)
    // float currentGain = audioProcessor.mySynth->getGlobalGain();
    float currentGain = 1;
    float dB = 20.0f * std::log10(currentGain);
    int ccValue = static_cast<int>(dB + 100.0f);
    masterVolumeSlider.setValue(ccValue);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    masterVolumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkblue);
    masterVolumeSlider.addListener(this);
    addAndMakeVisible(masterVolumeSlider);

    resetButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    // resetButton.setColour(juce::TextButton::buttonOnColourId , juce::Colours::grey);
    resetButton.setColour(juce::TextButton::textColourOffId , juce::Colours::black);
    resetButton.setButtonText ("Reset");
    /*
    resetButton.onClick = [this]()
    {
        std::cout << "Reset"<<std::endl;
        audioProcessor.myMidiInHandler -> resetSireneCh(1);
        audioProcessor.myMidiInHandler -> resetSireneCh(2);
        audioProcessor.myMidiInHandler -> resetSireneCh(3);
        audioProcessor.myMidiInHandler -> resetSireneCh(4);
        audioProcessor.myMidiInHandler -> resetSireneCh(5);
        audioProcessor.myMidiInHandler -> resetSireneCh(6);
        audioProcessor.myMidiInHandler -> resetSireneCh(7);
    };
    */
    addAndMakeVisible(resetButton);

    showResourcesButton.setColour(juce::TextButton::buttonColourId, juce::Colours::whitesmoke);
    showResourcesButton.setColour(juce::TextButton::textColourOffId , juce::Colours::black);
    showResourcesButton.setButtonText("Set resources directory");
    showResourcesButton.onClick = [this]()
    {
        // std::string resourcesPath = audioProcessor.mySynth->getResourcesPath();
        /*
        std::cout << "current resources path : " << resourcesPath << std::endl;
        // return;
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a file", juce::File(resourcesPath), ""
        );
        auto flags =
            juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectDirectories;

        fileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            // get the result to update resourcesPath
            juce::File newResourcesPath = chooser.getResult();
            std::cout << "new resources path : " << newResourcesPath.getFullPathName() << std::endl;
        });
        //*/
    };
#ifdef DEBUG
    addAndMakeVisible(showResourcesButton);
#endif

    // Démarrer le timer pour synchroniser l'UI avec les changements MIDI
    startTimer(50); // 50 ms = 20 Hz
}

MainCommandsComponent::~MainCommandsComponent()
{
    stopTimer();
}

void MainCommandsComponent::paint (juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 10);
    g.setColour(juce::Colour (71, 71, 71));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 10);
}


void MainCommandsComponent::resized()
{
    auto area = getLocalBounds().reduced(10, 5);

    // Reset button à gauche
    resetButton.setBounds(10, 10, 60, 30);

    // Master Volume à droite du reset button
    auto masterArea = area.removeFromLeft(220);
    masterArea.removeFromLeft(80); // Espace après le reset button
    masterVolumeLabel.setBounds(masterArea.removeFromTop(15));
    masterVolumeSlider.setBounds(masterArea.removeFromTop(20));

    showResourcesButton.setBounds(area.removeFromRight(220));
}

void MainCommandsComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &masterVolumeSlider)
    {
        int ccValue = static_cast<int>(masterVolumeSlider.getValue());
        // audioProcessor.mySynth->setGlobalGain(ccValue);
    }
}

void MainCommandsComponent::buttonClicked(juce::Button* button) {

}

void MainCommandsComponent::timerCallback()
{
    /*
    // Synchroniser Master Volume (CC7)
    float currentGain = audioProcessor.mySynth->getGlobalGain();
    float dB = 20.0f * std::log10(currentGain);
    int ccValue = static_cast<int>(dB + 100.0f);
    if (std::abs(masterVolumeSlider.getValue() - ccValue) > 1.0)
    {
        masterVolumeSlider.setValue(ccValue, juce::dontSendNotification);
    }
    //*/
}
