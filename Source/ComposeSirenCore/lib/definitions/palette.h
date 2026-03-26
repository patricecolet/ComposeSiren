//
// Created by joseph larralde on 31/01/2026.
//

#ifndef COMPOSESIREN_PALETTE_H
#define COMPOSESIREN_PALETTE_H

#pragma once
#include "stdint.h"

namespace mecaviv {
    namespace Colours {

        static uint32_t orangeMecanique = 0xffff9900;
        static uint32_t darkTransparentBackground = 0xf2283541;

        namespace SirenPalette {
            static uint32_t darkBlue = 0xff4650c8;
            static uint32_t lightBlue = 0xff00b4c8;
            static uint32_t darkGreen = 0xff3c8c28;
            static uint32_t lightGreen = 0xff78b428;
            static uint32_t sunnyYellow = 0xffd7b700;
            static uint32_t lightOrange = 0xffff7f00;
            static uint32_t darkOrange = 0xffff4500;
        }

        namespace MidiKeyboard {
            static uint32_t lowLevelRed = 0xff800000;
            static uint32_t midLevelRed = 0xffff0000;
            static uint32_t highLevelRed = 0xffff6600;
            static uint32_t dbRangeSeparatorBlue = 0xff0000ff;
        }
    }
}

#endif //COMPOSESIREN_PALETTE_H