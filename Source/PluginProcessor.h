#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "Engine.h"
#include "Presets.h"

void zilLog(const juce::String& message);   // writes to %APPDATA%/ZIL/zil.log

class ZilProcessor : public juce::AudioProcessor
{
public:
    ZilProcessor();
    ~ZilProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ZIL"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return (int) zil::presets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int i) override { loadPreset(i); }
    const juce::String getProgramName(int i) override { auto& p = zil::presets(); return i >= 0 && i < (int) p.size() ? p[(size_t) i].name : juce::String(); }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // --- ZIL specifics
    void loadPreset(int index);
    bool isFavorite(const juce::String& name) const { return favorites.contains(name); }
    void toggleFavorite(const juce::String& name) { if (favorites.contains(name)) favorites.removeString(name); else favorites.add(name); }
    int ccFor(int paramIndex) const { for (int i = 0; i < 128; ++i) if (ccMap[(size_t) i].load() == paramIndex) return i; return -1; }
    void clearCC(int paramIndex) { for (auto& c : ccMap) if (c.load() == paramIndex) c.store(-1); }

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    zil::Engine engine;
    juce::MidiKeyboardState keyboard;
    std::atomic<int> learnIndex { -1 };
    std::atomic<float> cpuLoad { 0 };
    int currentPreset = 0;
    int uiWidth = 1200;
    juce::StringArray favorites;

private:
    std::array<std::atomic<float>*, zil::P_COUNT> raw {};
    std::array<std::atomic<int>, 128> ccMap;
    double lastSr = 44100; int lastBlock = 512;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZilProcessor)
};
