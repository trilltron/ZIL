#pragma once
#include "Params.h"
#include <array>

namespace zil
{
struct Preset
{
    juce::String name, category, sub;
    bool signature = false;
    std::array<float, P_COUNT> v {};
};
const std::vector<Preset>& presets();
const juce::StringArray& categoryNames();   // KEYS, BASS, ...
}
