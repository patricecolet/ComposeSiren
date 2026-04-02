//
// Created by joseph larralde on 20/02/2026.
//

#include "PluginEditor.h"

static std::vector<std::unique_ptr<SirenStripComponent>>
makeStripComponents(juce::AudioProcessorValueTreeState& vts,
                    std::vector<parameterLayoutGroupData>& paramGroupData)
{
    std::vector<std::unique_ptr<SirenStripComponent>> strips;
    for (auto& group : paramGroupData) {
        if (sirenIddByStrId.contains(group.id)) {
            strips.push_back(std::make_unique<SirenStripComponent>(vts, group));
        }
    }
    return strips;
}

static std::map<sirenId, std::unique_ptr<SirenStripComponent>>
makeStrips(juce::AudioProcessorValueTreeState& vts,
           std::vector<parameterLayoutGroupData>& paramGroupData)
{
    std::map<sirenId, std::unique_ptr<SirenStripComponent>> strips;
    for (auto& group : paramGroupData) {
        if (sirenIddByStrId.contains(group.id)) {
            auto sid = sirenIddByStrId.at(group.id);
            strips.emplace(sid, std::make_unique<SirenStripComponent>(vts, group));
            strips.at(sid)->setBackgroundColour(juce::Colour(sirenColourById.at(sid)));
        }
    }
    return strips;
}

SirenOrchestraPluginEditor::SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor& p) :
    AudioProcessorEditor(p),
    audioProcessor(p),
    voiceManager(p.getVoiceManagerState()),
    // sirenStripComponents(
    //     makeStripComponents(
    //         p.getAudioProcessorValueTreeState(),
    //         p.getParameterLayoutData()
    //     )
    // ),
    sirenStrips(
        makeStrips(
            p.getAudioProcessorValueTreeState(),
            p.getParameterLayoutData()
        )
    ),
    rvbStrip(p.getAudioProcessorValueTreeState()),
    midiKeyboard(p.getMidiKeyboardState(),
                 p.getVoiceManagerState())
{
    auto& vts = p.getAudioProcessorValueTreeState();
    for (auto& group : p.getParameterLayoutData()) {
        auto id = group.id;
        if (sirenIddByStrId.contains(id)) {
            // auto sid = sirenIddByStrId.at(id);
            // sirenStripComponents[sid] = std::make_unique<SirenStripComponent>(vts, group);
            // sirenStripComponents.push_back(std::make_unique<SirenStripComponent>({ vts, group }));
        }
     }

    for (std::size_t i = 0; i < sirenOrder.size(); ++i) {
        addAndMakeVisible(sirenStrips.at(sirenOrder[i]).get());
    }

    addAndMakeVisible(rvbStrip);
    addAndMakeVisible(midiKeyboard);

    wood = juce::Drawable::createFromImageData(
        BinaryData::boiserie_png,
        BinaryData::boiserie_pngSize
    );

    // for (std::size_t i = 0; i < sirenStripComponents.size(); ++i) {
    //     auto& strip = sirenStripComponents[i];
    //     addAndMakeVisible(*strip.get());
    // }

    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    setSize(sirenWidth + 60, 600);
}

SirenOrchestraPluginEditor::~SirenOrchestraPluginEditor()
{
    audioProcessor.getVoiceManagerState().removeListener(this);
}
//==============================================================================
// AudioProcessorEditor
void SirenOrchestraPluginEditor::paint (juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds().toFloat());
    g.setColour(juce::Colour{mecaviv::Colours::darkTransparentBackground});
    g.fillRect(getLocalBounds().toFloat());

    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    const int ensembleHeight = (sirenOrder.size() - 1) * (55 + spacer) + 200;

    auto rect1 = juce::Rectangle<float>(0, 0, 30, ensembleHeight);
    wood->drawWithin(g, rect1, juce::RectanglePlacement::stretchToFit, 1.0f);
    auto rect2 = rect1.withX(sirenWidth + 30);
    wood->drawWithin(g, rect2, juce::RectanglePlacement::stretchToFit, 1.0f);

    juce::Path rectPath;
    rectPath.addRectangle(rect1);
    rectPath.addRectangle(rect2);

    // invert the path's fill shape and enlarge it,
    // so it casts a shadow
    juce::Path shadowPath(rectPath);
    shadowPath.addRectangle(shadowPath.getBounds().expanded(10));
    shadowPath.setUsingNonZeroWinding(false);
    // reduce clip region to avoid the shadow
    // being drawn outside of the shape to cast the shadow on
    g.reduceClipRegion(rectPath);

    juce::DropShadow ds(juce::Colours::black, 5, {0, 0});
    ds.drawForPath(g, shadowPath);
}

void SirenOrchestraPluginEditor::resized()
{
    //*
    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    int minSirenHeight = 55;

    for (std::size_t i = 0; i < sirenOrder.size(); ++i) {
        auto& strip = sirenStrips.at(sirenOrder[i]);
        strip->setBackgroundColour(juce::Colour(sirenColourById.at(sirenOrder[i])));
        strip->setTitle(sirenTitleById.at(sirenOrder[i]));
        strip->setShowTitle(true);

        if (i==0) {
            // strip->setShowTitle(true);
            strip->setShowGroupLabels(true);
            strip->setShowKnobLabels(true);
            strip->setShowTextBox(true);
            strip->setBounds(30, 0, sirenWidth, 100);
        } else {
            // strip->setShowTitle(true);
            strip->setShowGroupLabels(false);
            strip->setShowKnobLabels(false);
            strip->setShowTextBox(true);
            strip->setBounds(
                30, (i - 1) * (minSirenHeight + spacer) + 100 + spacer,
                sirenWidth, minSirenHeight
            );
        }
    }

    int nexty = (sirenOrder.size() - 1) * (minSirenHeight + spacer) + 100;

    // rvbStrip.setTitle("Reverb");
    // rvbStrip.setShowTitle(true);
    // rvbStrip.setShowGroupLabels(false);
    // rvbStrip.setShowKnobLabels(true);
    // rvbStrip.setShowTextBox(true);
    // rvbStrip.setBounds(0, nexty, sirenWidth, 70);
    // nexty = nexty + 70;

    midiKeyboard.setBounds(30, nexty, sirenWidth, 100);

    // rvbStrip.setBounds(0,130,getWidth(),70);
    // mainButtons.setBounds(0,0,sirenWidth,25);
    // voiceManager.setBounds(0,0,sirenWidth/2,30);
    // sirenStrip.setBounds(0,30,sirenWidth,100);
    // midiKeyboard.setBounds(0, 130, getWidth(), 70);
    //*/
}

// void SirenOrchestraPluginEditor::categoryChanged(sirenCategory c)
// {
//     // sirenStrip.setBackgroundColour(
//     //     juce::Colour(sirenCategoriesData.at(c).colour)
//     // );
// }
