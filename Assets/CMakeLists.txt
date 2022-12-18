juce_add_binary_data(${BaseTargetName}BinaryData
  SOURCES
    Picto_Siren_40x37.png
)

target_link_libraries(${BaseTargetName} PRIVATE ${BaseTargetName}BinaryData)