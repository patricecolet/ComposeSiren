//
// Created by joseph larralde on 20/02/2026.
//

#include <apvtsUtilities.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

SirenOrchestraPluginProcessor::SirenOrchestraPluginProcessor() :
    sampleCountForMidiInTimer(0),
    apvts(*this,
          nullptr,
          "PARAMETERS",
          createParameterLayout(std::vector<paramGroupData>{
              mkLayoutGroupData("S1", "Alto 1",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S2", "Alto 2",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S3", "Basse",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S4", "Tenor",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S5", "Soprano 1",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S6", "Soprano 2",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("S7", "Piccolo",
                                ParameterClass::SirenControl,
                                ParameterClass::TrackControl),
              mkLayoutGroupData("R",  "Reverb",
                                ParameterClass::ReverbControl),
              mkLayoutGroupData("M",  "Master",
                                ParameterClass::MasterControl)
          }))
{
    std::cout << apvts.state.getNumChildren() << std::endl;
}

// TODO !