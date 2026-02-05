juce_add_binary_data(${BaseTargetName}BinaryData
        SOURCES
        basic-knob-bg-bw.svg
        basic-knob-indicator.svg
        basic-knob-outline.svg
        Picto_Siren_40x37.png
)

target_link_libraries(${BaseTargetName} PRIVATE ${BaseTargetName}BinaryData)