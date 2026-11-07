#pragma once
#include "Params.h"
#include <array>

namespace zil
{
struct Preset
{
    juce::String name, category, sub;
    bool signature = false, user = false;
    juce::File file;
    std::array<float, P_COUNT> v {};
};
const std::vector<Preset>& presets();        // factory presets first, then user presets from userPresetDir()
int factoryCount();
juce::File userPresetDir();                  // default: <Documents>/ZIL/Presets  (*.zilpreset files)
void setUserPresetDir(const juce::File&);
int reloadUserPresets();                     // rescans the folder (message thread); returns number loaded
bool savePreset(const juce::String& name, const std::array<float, P_COUNT>& values, juce::File* written = nullptr);
int exportPack(const juce::File& dir);       // writes the starter pack as .zilpreset files
const juce::StringArray& categoryNames();   // KEYS, BASS, ...
}
