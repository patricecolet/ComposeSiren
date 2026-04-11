//
// Created by joseph larralde on 20/02/2026.
//

#include "PluginEditor.h"
#include <colourUtilities.h>

static std::map<sirenId, std::unique_ptr<SirenStripComponent>>
makeSirenStrips(juce::AudioProcessorValueTreeState& vts,
           std::vector<parameterLayoutGroupData>& paramGroupData)
{
    std::map<sirenId, std::unique_ptr<SirenStripComponent>> strips;
    for (auto& group : paramGroupData) {
        if (sirenIddByStrId.contains(group.id)) {
            auto sid = sirenIddByStrId.at(group.id);
            strips.emplace(sid, std::make_unique<SirenStripComponent>(vts, group, true));
            strips.at(sid)->setBackgroundColour(juce::Colour(sirenColourById.at(sid)));
        }
    }
    return std::move(strips);
}

SirenOrchestraPluginEditor::SirenOrchestraPluginEditor(SirenOrchestraPluginProcessor& p) :
    AudioProcessorEditor(p),
    audioProcessor(p),
    voiceManager(p.getVoiceManagerState()),
    sirenStrips(
        makeSirenStrips(
            p.getAudioProcessorValueTreeState(),
            p.getParameterLayoutData()
        )
    ),
    rvbStrip(p.getAudioProcessorValueTreeState()),
    midiKeyboard(p.getMidiKeyboardState(),
                 p.getVoiceManagerState())
{
    for (auto i : sirenOrder) {
        addAndMakeVisible(sirenStrips.at(i).get());
    }

    addAndMakeVisible(rvbStrip);
    addAndMakeVisible(midiKeyboard);

    wood = juce::Drawable::createFromImageData(
        BinaryData::boiserie_png,
        BinaryData::boiserie_pngSize
    );

    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    setSize(sirenWidth + 2 * woodThickness, 600);
}

SirenOrchestraPluginEditor::~SirenOrchestraPluginEditor()
{
    audioProcessor.getVoiceManagerState().removeListener(this);
}
//==============================================================================
// AudioProcessorEditor
void SirenOrchestraPluginEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds().toFloat());
    g.setColour(juce::Colour{mecaviv::Colours::darkTransparentBackground});
    g.fillRect(getLocalBounds().toFloat());

    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    const int ensembleHeight = (sirenOrder.size() - 1) * (55 + spacer) + 200;

    bool drawWood = woodThickness > 0;
    if (drawWood) {
        // wood borders
        auto rect1 = juce::Rectangle<float>(0, 0, woodThickness, ensembleHeight);
        wood->drawWithin(g, rect1, juce::RectanglePlacement::stretchToFit, 1.0f);
        auto rect2 = rect1.withX(sirenWidth + woodThickness);
        wood->drawWithin(g, rect2, juce::RectanglePlacement::stretchToFit, 1.0f);

        // wood borders inner shadows
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
}

void SirenOrchestraPluginEditor::resized()
{
    const int& spacer = controlStripLayout::spacerSize;
    int sirenWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getMinWidth());
    int sirenTitleWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getTitleWidth());
    int sirenParametersWidth = static_cast<int>(sirenStrips.at(sirenOrder[0])->getParametersWidth(true));

    for (std::size_t i = 0; i < sirenOrder.size(); ++i) {
        auto& strip = sirenStrips.at(sirenOrder[i]);
        strip->setBackgroundColour(juce::Colour(sirenColourById.at(sirenOrder[i])));
        // strip->setBackgroundColour(juce::Colour(sirenColourById.at(sirenOrder[0])));
        strip->setTitle(sirenTitleById.at(sirenOrder[i]));
        strip->setShowTitle(true);

        if (i==0) {
            strip->setShowGroupLabels(true);
            strip->setShowKnobLabels(true);
            strip->setShowTextBox(true);
            strip->setBounds(woodThickness, 0, sirenWidth, 100);
        } else {
            strip->setShowGroupLabels(false);
            strip->setShowKnobLabels(false);
            strip->setShowTextBox(true);
            strip->setBounds(
                woodThickness, (i - 1) * (minSirenHeight + spacer) + 100 + spacer,
                sirenWidth, minSirenHeight
            );
        }
    }

    int nexty = (sirenOrder.size() - 1) * (minSirenHeight + spacer) + 100 + spacer;

    rvbStrip.setTitle("Reverb");
    rvbStrip.setShowTitle(true);
    rvbStrip.setShowGroupLabels(false);
    rvbStrip.setShowKnobLabels(true);
    rvbStrip.setShowTextBox(true);
    rvbStrip.setBounds(woodThickness + sirenTitleWidth, nexty, sirenParametersWidth, 65);
    rvbStrip.setBackgroundColour(juce::Colour{mecaviv::Colours::SirenPalette::lightGreen});
    nexty = nexty + 65 + spacer;

    midiKeyboard.setBounds(woodThickness + sirenTitleWidth, nexty, sirenParametersWidth, 70);
}
