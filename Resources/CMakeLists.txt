juce_add_binary_data(${BaseTargetName}BinaryData
  SOURCES
    #[[
    dataAmpS1
    dataAmpS3
    dataAmpS4
    dataAmpS5
    dataAmpS7
    
    datadureTabsS1
    datadureTabsS3
    datadureTabsS4
    datadureTabsS5
    datadureTabsS7

    dataFreqS1
    dataFreqS3
    dataFreqS4
    dataFreqS5
    dataFreqS7

    dataVectorIntervalS1
    dataVectorIntervalS3
    dataVectorIntervalS4
    dataVectorIntervalS5
    dataVectorIntervalS7
    #]]

    Picto_Siren_40x37.png
)

target_link_libraries(${BaseTargetName} PRIVATE ${BaseTargetName}BinaryData)