/*
  ==============================================================================

    PluginEditor_S1.cpp
    ComposeSiren S1 Alto - Interface utilisateur Solo

  ==============================================================================
*/

#include "PluginProcessor_S1.h"
#include "PluginEditor_S1.h"

//==============================================================================
SireneS1AudioProcessorEditor::SireneS1AudioProcessorEditor (SireneS1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      lastNoteOn(false), lastNote(0), lastVelocity(0)
{
    setSize (500, 350);
    
    // Démarrer le timer pour rafraîchir l'affichage (60 FPS)
    startTimerHz(60);
}

SireneS1AudioProcessorEditor::~SireneS1AudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void SireneS1AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fond gris foncé
    g.fillAll(juce::Colour(0xff2a2a2a));
    
    // En-tête avec titre
    g.setColour(juce::Colour(0xff404040));
    g.fillRect(0, 0, getWidth(), 80);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("ComposeSiren S1", 20, 15, getWidth() - 40, 30, juce::Justification::centred);
    
    g.setFont(juce::Font(18.0f));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText("Alto", 20, 48, getWidth() - 40, 25, juce::Justification::centred);
    
    // Section affichage MIDI
    int yPos = 100;
    
    // LED Note On/Off
    g.setColour(lastNoteOn ? juce::Colours::green : juce::Colour(0xff333333));
    g.fillEllipse(30, yPos, 30, 30);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f));
    g.drawText("NOTE", 70, yPos, 100, 30, juce::Justification::left);
    
    // Numéro de note actuelle
    yPos += 50;
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(14.0f));
    g.drawText("Note MIDI:", 30, yPos, 120, 25, juce::Justification::left);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    if (lastNoteOn)
    {
        juce::String noteStr = juce::String(lastNote);
        // Ajouter le nom de la note
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int octave = (lastNote / 12) - 1;
        juce::String noteName = juce::String(noteNames[lastNote % 12]) + juce::String(octave);
        noteStr += " (" + noteName + ")";
        g.drawText(noteStr, 30, yPos + 25, 200, 35, juce::Justification::left);
    }
    else
    {
        g.setColour(juce::Colour(0xff666666));
        g.drawText("---", 30, yPos + 25, 200, 35, juce::Justification::left);
    }
    
    // Vélocité
    yPos += 75;
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(14.0f));
    g.drawText("Vélocité:", 30, yPos, 120, 25, juce::Justification::left);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    if (lastNoteOn)
    {
        g.drawText(juce::String(lastVelocity), 30, yPos + 25, 100, 35, juce::Justification::left);
        
        // Barre de vélocité
        float velocityNorm = lastVelocity / 127.0f;
        g.setColour(juce::Colour(0xff444444));
        g.fillRect(150, yPos + 30, 320, 20);
        g.setColour(juce::Colours::cyan);
        g.fillRect(150, yPos + 30, (int)(320 * velocityNorm), 20);
    }
    else
    {
        g.setColour(juce::Colour(0xff666666));
        g.drawText("---", 30, yPos + 25, 100, 35, juce::Justification::left);
        g.setColour(juce::Colour(0xff444444));
        g.fillRect(150, yPos + 30, 320, 20);
    }
    
    // CC actifs
    yPos += 85;
    g.setColour(juce::Colour(0xffaaaaaa));
    g.setFont(juce::Font(14.0f));
    g.drawText("Contrôleurs MIDI actifs:", 30, yPos, 200, 25, juce::Justification::left);
    
    yPos += 30;
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    
    if (lastCC.empty())
    {
        g.setColour(juce::Colour(0xff666666));
        g.drawText("(aucun)", 30, yPos, 200, 20, juce::Justification::left);
    }
    else
    {
        int ccIndex = 0;
        for (const auto& pair : lastCC)
        {
            if (ccIndex >= 5) // Limiter à 5 CC affichés
            {
                g.drawText("...", 30 + (ccIndex % 2) * 230, yPos + (ccIndex / 2) * 20, 200, 20, juce::Justification::left);
                break;
            }
            
            juce::String ccText = "CC" + juce::String(pair.first) + ": " + juce::String(pair.second);
            g.drawText(ccText, 30 + (ccIndex % 2) * 230, yPos + (ccIndex / 2) * 20, 200, 20, juce::Justification::left);
            ccIndex++;
        }
    }
}

void SireneS1AudioProcessorEditor::resized()
{
    // Pas de composants redimensionnables dans cette interface
}

void SireneS1AudioProcessorEditor::timerCallback()
{
    // Récupérer l'état MIDI depuis le processeur
    auto midiState = audioProcessor.getMidiState();
    
    bool needsRepaint = false;
    
    if (midiState.noteOn != lastNoteOn ||
        midiState.currentNote != lastNote ||
        midiState.currentVelocity != lastVelocity ||
        midiState.activeCC != lastCC)
    {
        lastNoteOn = midiState.noteOn;
        lastNote = midiState.currentNote;
        lastVelocity = midiState.currentVelocity;
        lastCC = midiState.activeCC;
        needsRepaint = true;
    }
    
    if (needsRepaint)
        repaint();
}

