configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
target_sources(${BaseTargetName}
  PRIVATE 
    CS_midiIN.cpp

    PluginEditor.cpp

    PluginProcessor.cpp

#    lib/definitions/parameterDefinitions.cpp
#    lib/definitions/sirenProperties.cpp

    lib/dsp/Sirene.cpp
    lib/dsp/allpass.cpp
    lib/dsp/comb.cpp
    lib/dsp/mareverbe.cpp

    synth.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Components/MainCommandsComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/MixerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/MixerStripComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/ReverbComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/SirenStripComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/BasicKnobComponent.cpp
)