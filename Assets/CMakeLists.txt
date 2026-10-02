set(BINARY_DATA_SOURCES
  basic-knob-bg-bw.svg
  basic-knob-indicator.svg
  Picto_Siren_40x37.png
)

if(COMPOSESIREN_SIREN_WAVES)
  list(APPEND BINARY_DATA_SOURCES shaders/SirenWave.frag)
endif()

juce_add_binary_data(${BaseTargetName}BinaryData
  SOURCES ${BINARY_DATA_SOURCES}
)
