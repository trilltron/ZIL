#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Gui.h"

class ZilEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ZilEditor(ZilProcessor&);
    ~ZilEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void showPage(int index);
    juce::Image renderSnapshot() { return createComponentSnapshot(getLocalBounds(), true, 1.f); }

private:
    struct Impl;
    void timerCallback() override;
    void applyTransform();

    ZilProcessor& proc;
    zil::ui::ZilLF lf;
    std::unique_ptr<Impl> impl;
    juce::TooltipWindow tooltip { this, 450 };
    float targetTx = 0, targetTy = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZilEditor)
};
