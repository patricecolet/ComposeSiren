#include <map>
#include "sirenProperties.h"

ZeroBasedMidiChannel::operator OneBasedMidiChannel() const                      { return {.oneBased = this->zeroBased+1};    }
bool ZeroBasedMidiChannel::operator< (const ZeroBasedMidiChannel& other) const  { return this->zeroBased < other.zeroBased;  }
bool ZeroBasedMidiChannel::operator== (const ZeroBasedMidiChannel& other) const { return this->zeroBased == other.zeroBased; }

OneBasedMidiChannel::operator ZeroBasedMidiChannel() const                    { return {.zeroBased = this->oneBased-1}; }
bool OneBasedMidiChannel::operator< (const OneBasedMidiChannel& other) const  { return this->oneBased < other.oneBased; }
bool OneBasedMidiChannel::operator== (const OneBasedMidiChannel& other) const { return this->oneBased == other.oneBased; }

bool AnyOrOneBasedMidiChannel::operator< (const AnyOrOneBasedMidiChannel& other) const {
    return isAny < other.isAny
           || (isAny == other.isAny && channel < other.channel);
}

bool AnyOrOneBasedMidiChannel::operator== (const AnyOrOneBasedMidiChannel& other) const {
    return (isAny && other.isAny) // if isAny we have equality regardless of the channel
           || (!isAny && !other.isAny && channel == other.channel);
}

std::map<int, std::tuple<int, int>> VelocityRanges::makeForCategory(const sirenCategory& c)
{
    // std::cout << "making velocity ranges for category " << c << std::endl;
    std::map<int, std::tuple<int, int>> res;

    const auto& d = sirenCategoriesData.at(c);
    auto [ minNote, maxNote ] = d.minMaxNote;
    auto dbPerNote = d.dbPerNote;
    assert(minNote < maxNote && dbPerNote.size() == maxNote - minNote + 1);

    auto it = dbPerNote.begin();
    int i = minNote, min = 0, max = 0;
    int dbRangeIndex = 0;

    for (auto& db : dbThresholds) {
        bool minIsSet = false;

        while (it != dbPerNote.end() && *it < db) {
            if (!minIsSet) {
                minIsSet = true;
                min = i;
            }

            it++;
            i++;
        }

        if (minIsSet) {
            max = i - 1; // whether it == dbPerNote.end() or not, we are good
            // std::cout << "adding range " << dbRangeIndex << " " << min << " " << max << std::endl;
            res[dbRangeIndex] = std::tuple(min, max);
        }

        dbRangeIndex++;
    }

    if (it != dbPerNote.end()) {
        min = i;
        max = maxNote;
        // std::cout << "adding range " << dbRangeIndex << " " << min << " " << max << std::endl;
        res[dbRangeIndex] = std::tuple(min, max);
    }

    return res;
}
