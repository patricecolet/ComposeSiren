//
// Created by joseph larralde on 26/03/2026.
//

#ifndef COMPOSESIREN_UIPARAMETERUTILITIES_H
#define COMPOSESIREN_UIPARAMETERUTILITIES_H

#include "lib/definitions/sirenProperties.h"

// sirenCategory menu utilities ------------------------------------------------

static const
std::map<int, sirenCategory> sirenCategoryByMenuIndex {
    { 1, sirenCategory::Bass    },
    { 2, sirenCategory::Tenor   },
    { 3, sirenCategory::Alto    },
    { 4, sirenCategory::Soprano },
    { 5, sirenCategory::Piccolo }
};

static const
std::map<sirenCategory, int> menuIndexBySirenCategory = []() {
    std::map<sirenCategory, int> res;
    for (auto& p : sirenCategoryByMenuIndex) {
        res[p.second] = p.first;
    }
    return res;
}();

// AnyOrOneBasedMidiChannel menu utilities -------------------------------------

// we use a vector here to force the order of channel menu items during creation
static const
std::vector<std::pair<int, AnyOrOneBasedMidiChannel>> menuIndexMidiChannelPairs {
    { 8, AnyOrOneBasedMidiChannel::any() },
    { 1, AnyOrOneBasedMidiChannel::specific({1}) },
    { 2, AnyOrOneBasedMidiChannel::specific({2}) },
    { 3, AnyOrOneBasedMidiChannel::specific({3}) },
    { 4, AnyOrOneBasedMidiChannel::specific({4}) },
    { 5, AnyOrOneBasedMidiChannel::specific({5}) },
    { 6, AnyOrOneBasedMidiChannel::specific({6}) },
    { 7, AnyOrOneBasedMidiChannel::specific({7}) },
};

static const
std::map<int, AnyOrOneBasedMidiChannel> midiChannelByMenuIndex = []() {
    std::map<int, AnyOrOneBasedMidiChannel> res;
    for (auto& p : menuIndexMidiChannelPairs) {
        res[p.first] = p.second;
    }
    return res;
}();


static const
std::map<AnyOrOneBasedMidiChannel, int> menuIndexByMidiChannel = []() {
    std::map<AnyOrOneBasedMidiChannel, int> res;
    for (auto& p : menuIndexMidiChannelPairs) {
        res[p.second] = p.first;
    }
    return res;
}();

#endif //COMPOSESIREN_UIPARAMETERUTILITIES_H
