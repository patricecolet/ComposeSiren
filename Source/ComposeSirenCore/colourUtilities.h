//
// Created by joseph larralde on 10/04/2026.
//

#ifndef COMPOSESIREN_COLOURUTILITIES_H
#define COMPOSESIREN_COLOURUTILITIES_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "lib/definitions/palette.h"
#include "lib/definitions/sirenProperties.h"

static inline std::vector<juce::Colour> makeHslInterpolatedPalette(
    juce::Colour start,
    juce::Colour end,
    std::size_t size
) {
    std::vector<juce::Colour> res;
    float hs, ss, ls, he, se, le;

    start.getHSL(hs, ss, ls);
    end.getHSL(he, se, le);

    for (auto i = 0; i < size; ++i) {
        float h = hs + (he - hs) * i / (size - 1);
        float s = ss + (se - ss) * i / (size - 1);
        float l = ls + (le - ls) * i / (size - 1);
        juce::Colour c(juce::Colour::fromHSL(h, s, l, 1.0f));
        res.push_back(c);
    }
    return res;
}

inline const std::map<sirenId, juce::Colour> sirenColourById = []() {

    auto colours = makeHslInterpolatedPalette(
        juce::Colour(mecaviv::Colours::SirenPalette::darkBlue),
        juce::Colour(mecaviv::Colours::SirenPalette::lightBlue),
        8
    );

    std::map<sirenId, juce::Colour> res = {
        { S3, colours[0] },
        { S4, colours[1] },
        { S1, colours[2] },
        { S2, colours[3] },
        { S5, colours[4] },
        { S6, colours[5] },
        { S7, colours[6] },
    };

    // std::map<sirenId, juce::Colour> res = {
    //     { S3, colours[0] },
    //     { S4, colours[1] },
    //     { S1, colours[0] },
    //     { S2, colours[1] },
    //     { S5, colours[0] },
    //     { S6, colours[1] },
    //     { S7, colours[0] },
    // };

    return res;
}();


#endif //COMPOSESIREN_COLOURUTILITIES_H