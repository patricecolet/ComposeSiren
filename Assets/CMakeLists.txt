juce_add_binary_data(${BaseTargetName}BinaryData
  SOURCES
    Picto_Siren_40x37.png
)

# Fix pour ARM64/aarch64 Linux : forcer -fPIC pour BinaryData
set_target_properties(${BaseTargetName}BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${BaseTargetName} PRIVATE ${BaseTargetName}BinaryData)