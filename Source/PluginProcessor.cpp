#include "PluginProcessor.h"
#include "PluginEditor.h"

// Diagnostic log: %APPDATA%\ZIL\zil.log (Windows) - records startup steps so load problems in a host can be traced.
static void zilLog(const juce::String& m)
{
    static std::unique_ptr<juce::FileLogger> logger = [] {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("ZIL");
        dir.createDirectory();
        auto f = dir.getChildFile("zil.log");
        if (f.getSize() > 200000) f.deleteFile();
        return std::make_unique<juce::FileLogger>(f, "ZIL log", 0);
    }();
    if (logger) logger->logMessage(juce::Time::getCurrentTime().toString(true, true, true, true) + "  " + m);
}

ZilProcessor::ZilProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undo, "ZIL", zil::createLayout())
{
    zilLog("processor ctor start");
    for (int i = 0; i < zil::P_COUNT; ++i) raw[(size_t) i] = apvts.getRawParameterValue(zil::defs()[(size_t) i].id);
    for (auto& c : ccMap) c.store(-1);
    zilLog("params ok; loading preset bank");
    loadPreset(0);
    zilLog("processor ctor done");
}

void ZilProcessor::prepareToPlay(double sr, int block)
{
    zilLog("prepareToPlay sr=" + juce::String(sr) + " block=" + juce::String(block));
    lastSr = sr; lastBlock = block;
    engine.prepare(sr, block);
}

void ZilProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const auto t0 = juce::Time::getHighResolutionTicks();
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) { buffer.setSize(2, n, true, true, true); }

    keyboard.processNextMidiBuffer(midi, 0, n, true);

    for (const auto meta : midi)
    {
        auto m = meta.getMessage();
        if (!m.isController()) continue;
        const int cc = m.getControllerNumber();
        if (cc == 1 || cc == 64 || cc >= 120) continue;
        const int li = learnIndex.load();
        if (li >= 0) { clearCC(li); ccMap[(size_t) cc].store(li); learnIndex.store(-1); }
        const int pi = ccMap[(size_t) cc].load();
        if (pi >= 0)
            if (auto* p = apvts.getParameter(zil::defs()[(size_t) pi].id)) p->setValueNotifyingHost((float) m.getControllerValue() / 127.f);
    }

    zil::Snap s;
    for (int i = 0; i < zil::P_COUNT; ++i) s.at(i) = raw[(size_t) i]->load();

    double bpm = 120.0, ppq = -1.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    engine.process(buffer, midi, s, bpm, ppq, playing);
    midi.clear();

    const double used = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - t0);
    const float load = (float) (used / ((double) n / lastSr));
    cpuLoad.store(cpuLoad.load() * 0.9f + load * 0.1f);
}

void ZilProcessor::loadPreset(int index)
{
    const auto& all = zil::presets();
    if (index < 0 || index >= (int) all.size()) return;
    currentPreset = index;
    const auto& p = all[(size_t) index];
    for (int i = 0; i < zil::P_COUNT; ++i)
        if (auto* prm = apvts.getParameter(zil::defs()[(size_t) i].id))
            prm->setValueNotifyingHost(prm->convertTo0to1(p.v[(size_t) i]));
    engine.midiCount++;
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
}

void ZilProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("preset", currentPreset, nullptr);
    state.setProperty("favs", favorites.joinIntoString("|"), nullptr);
    state.setProperty("uiW", uiWidth, nullptr);
    juce::String map;
    for (int i = 0; i < 128; ++i) if (ccMap[(size_t) i].load() >= 0) map << i << ":" << zil::defs()[(size_t) ccMap[(size_t) i].load()].id << ",";
    state.setProperty("midiMap", map, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void ZilProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml(*xml);
            currentPreset = juce::jlimit(0, getNumPrograms() - 1, (int) vt.getProperty("preset", 0));
            favorites = juce::StringArray::fromTokens(vt.getProperty("favs").toString(), "|", "");
            favorites.removeEmptyStrings();
            uiWidth = juce::jlimit(600, 3200, (int) vt.getProperty("uiW", 1200));
            for (auto& c : ccMap) c.store(-1);
            for (auto& tok : juce::StringArray::fromTokens(vt.getProperty("midiMap").toString(), ",", ""))
            {
                const int colon = tok.indexOfChar(':');
                if (colon > 0) { int pi = zil::indexOf(tok.substring(colon + 1)); int cc = tok.substring(0, colon).getIntValue(); if (pi >= 0 && cc >= 0 && cc < 128) ccMap[(size_t) cc].store(pi); }
            }
            apvts.replaceState(vt);
        }
}

juce::AudioProcessorEditor* ZilProcessor::createEditor()
{
    zilLog("createEditor");
    auto* e = new ZilEditor(*this);
    zilLog("editor created");
    return e;
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZilProcessor(); }

#include "Vst2Wrapper.inl"
