configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
target_sources(${BaseTargetName}
  PRIVATE 
    CS_midiIN.cpp
    # CS_midiIN.h
    PluginEditor.cpp
    # PluginEditor.h
    PluginProcessor.cpp
    # PluginProcessor.h
    Sirene.cpp
    # Sirene.h
    synth.cpp
    # synth.h
)