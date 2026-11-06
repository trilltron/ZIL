// Headless checks: renders every factory preset offline, validates the output, writes WAVs and GUI screenshots.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Vst2.h"

static void writeWav(const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat fmt;
    if (auto os = f.createOutputStream())
        if (auto w = std::unique_ptr<juce::AudioFormatWriter>(fmt.createWriterFor(os.release(), sr, 2, 24, {}, 0))) w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "audio";
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 2 ? argv[2] : "out");
    out.createDirectory();
    ZilProcessor p;

    if (mode == "audio")
    {
        const double sr = 48000; const int blk = 512, total = (int) (sr * 4.0), holdN = (int) (sr * 1.5);
        int bad = 0, silent = 0; float worstPeak = 0;
        std::vector<int> wavs;
        for (int i = 0; i < (int) zil::presets().size(); i += 1)
            if (i % 37 == 0 || zil::presets()[(size_t) i].name.startsWith("ZIL Pulse") || zil::presets()[(size_t) i].name.startsWith("808") || zil::presets()[(size_t) i].name.startsWith("Concert Grand")) wavs.push_back(i);
        for (int i = 0; i < (int) zil::presets().size(); ++i)
        {
            p.prepareToPlay(sr, blk); p.loadPreset(i);
            const bool kit = p.apvts.getRawParameterValue("kit")->load() > 0.5f;
            const bool arp = p.apvts.getRawParameterValue("arpOn")->load() > 0.5f;
            const bool mono = p.apvts.getRawParameterValue("voices")->load() < 1.5f;
            juce::AudioBuffer<float> all(2, total); all.clear(); juce::AudioBuffer<float> b(2, blk);
            std::vector<int> notes = kit ? std::vector<int> { 36, 42, 38, 46 } : (mono ? std::vector<int> { 40 } : std::vector<int> { 48, 52, 55 });
            bool nan = false;
            for (int pos = 0; pos < total; pos += blk)
            {
                juce::MidiBuffer m;
                if (pos == 0) for (int n : notes) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8) 100), 0);
                if (kit && pos == (int) (sr * 0.5) / blk * blk) m.addEvent(juce::MidiMessage::noteOn(1, 38, (juce::uint8) 110), 0);
                if (!kit && pos <= holdN && pos + blk > holdN) for (int n : notes) m.addEvent(juce::MidiMessage::noteOff(1, n), holdN - pos);
                p.processBlock(b, m);
                for (int c = 0; c < 2; ++c) { auto* d = b.getReadPointer(c); for (int k = 0; k < blk; ++k) if (!std::isfinite(d[k])) nan = true; all.copyFrom(c, pos, b, c, 0, juce::jmin(blk, total - pos)); }
            }
            const float peak = juce::jmax(all.getMagnitude(0, 0, total), all.getMagnitude(1, 0, total));
            const float rms = all.getRMSLevel(0, 0, holdN);
            worstPeak = juce::jmax(worstPeak, peak);
            if (nan) { ++bad; std::cout << "NaN in preset " << zil::presets()[(size_t) i].name << "\n"; }
            if (rms < 1e-4f) { ++silent; std::cout << "SILENT: " << zil::presets()[(size_t) i].name << (arp ? " (arp)" : "") << "\n"; }
            if (peak > 1.0001f) std::cout << "CLIP: " << zil::presets()[(size_t) i].name << " " << peak << "\n";
            if (std::find(wavs.begin(), wavs.end(), i) != wavs.end()) writeWav(out.getChildFile(zil::presets()[(size_t) i].name.replaceCharacter(' ', '_') + ".wav"), all, sr);
        }
        std::cout << "presets=" << zil::presets().size() << " nan=" << bad << " silent=" << silent << " worstPeak=" << worstPeak << "\n";
        return bad || silent ? 1 : 0;
    }
    if (mode == "vst2")
    {
        using zilvst2::HostCb; using zilvst2::intptr; using zilvst2::VstMidiEvent; using zilvst2::VstEvent;
        using VAEffect = zilvst2::AEffect;
        HostCb host = [](VAEffect*, int32_t op, int32_t, intptr, void*, float) -> intptr { return op == 1 ? 2400 : 0; };
        VAEffect* e = VSTPluginMain(host);
        if (!e) { std::cout << "VSTPluginMain returned null\n"; return 1; }
        auto d = [&](int op, int idx = 0, intptr v = 0, void* ptr = nullptr, float o = 0) { return e->dispatcher(e, op, idx, v, ptr, o); };
        char name[64] = {}; d(45, 0, 0, name); std::cout << "name=" << name << " params=" << e->numParams << " programs=" << e->numPrograms << " flags=" << e->flags << " magic=" << std::hex << e->magic << std::dec << "\n";
        d(0); d(10, 0, 0, nullptr, 48000.f); d(11, 0, 512); d(12, 0, 1);
        d(2, 0, 4); char pn[64] = {}; d(5, 0, 0, pn); std::cout << "program 4 = " << pn << "\n";
        VstMidiEvent me {}; me.type = 1; me.byteSize = sizeof(me); me.midiData[0] = (char) 0x90; me.midiData[1] = 60; me.midiData[2] = 100;
        struct { int32_t n; intptr r; VstEvent* ev[1]; } evs { 1, 0, { (VstEvent*) &me } };
        d(25, 0, 0, &evs);
        std::vector<float> L(512), R(512); float* out[2] = { L.data(), R.data() }; float peak = 0;
        for (int i = 0; i < 40; ++i) { e->processReplacing(e, nullptr, out, 512); for (int k = 0; k < 512; ++k) peak = std::max(peak, std::abs(L[(size_t) k])); }
        std::cout << "vst2 audio peak after note-on: " << peak << "\n";
        void* chunk = nullptr; auto sz = d(23, 0, 0, &chunk); std::cout << "chunk bytes=" << sz << "\n";
        e->setParameter(e, zil::P_fCut, 0.25f); std::cout << "param roundtrip: " << e->getParameter(e, zil::P_fCut) << "\n";
        char disp[64] = {}; d(7, zil::P_fCut, 0, disp); std::cout << "display fCut: " << disp << "\n";
        d(12, 0, 0); d(1);
        return peak > 0.01f && sz > 100 ? 0 : 1;
    }
    if (mode == "state")
    {
        // 1) state round-trip  2) arp produces notes from a held chord  3) live editor with timers + synthetic mouse events
        p.prepareToPlay(48000, 512); p.loadPreset(40); p.apvts.getParameter("fCut")->setValueNotifyingHost(0.37f); p.toggleFavorite("Pulsar Bloom");
        juce::MemoryBlock mb; p.getStateInformation(mb);
        ZilProcessor q; q.prepareToPlay(48000, 512); q.setStateInformation(mb.getData(), (int) mb.getSize());
        bool same = q.currentPreset == p.currentPreset && q.isFavorite("Pulsar Bloom");
        for (int i = 0; i < zil::P_COUNT; ++i) { auto id = zil::defs()[(size_t) i].id; if (std::abs(q.apvts.getRawParameterValue(id)->load() - p.apvts.getRawParameterValue(id)->load()) > 1e-4f) { same = false; std::cout << "mismatch " << id << "\n"; } }
        std::cout << "state roundtrip: " << (same ? "OK" : "FAIL") << " (" << mb.getSize() << " bytes)\n";
        int arpIdx = 0; for (int i = 0; i < (int) zil::presets().size(); ++i) if (zil::presets()[(size_t) i].name == "Arp Pluck Sequence") arpIdx = i;
        p.loadPreset(arpIdx); p.prepareToPlay(48000, 512);
        auto setP = [&](const char* id, float real) { auto* pr = p.apvts.getParameter(id); pr->setValueNotifyingHost(pr->convertTo0to1(real)); };
        setP("lBDec", 0.02f); setP("lBSus", 0.f); setP("lADec", 0.02f); setP("lASus", 0.f); setP("aRel", 0.01f); setP("aSus", -60.f); setP("dOn", 0); setP("rOn", 0); setP("arpGate", 0.3f); setP("bFM", 0); setP("fEnv", 0);
        p.prepareToPlay(48000, 64);
        juce::AudioBuffer<float> b(2, 64); int onsets = 0; float prev = 1;
        for (int k = 0; k < 3000; ++k) { juce::MidiBuffer m; if (k == 0) for (int n : { 60, 64, 67 }) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8) 100), 0);
            p.processBlock(b, m); float e = b.getMagnitude(0, 0, 64); if (e > 0.005f && prev < 0.001f) { ++onsets; std::cout << k * 64 / 48 << "ms "; } prev = e; }
        std::cout << "\n";
        std::cout << "arp onsets in 4.0s @120bpm 1/16: " << onsets << " (expect ~32)\n";
        p.loadPreset(0);
        ZilEditor ed(p); ed.setSize(1200, 675); ed.setVisible(true);
        for (int i = 0; i < 6; ++i) ed.showPage(i);
        ed.showPage(0);
        ed.mouseMove(juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), juce::Point<float>(100, 100), {}, 0, 0, 0, 0, 0, &ed, &ed, juce::Time::getCurrentTime(), {}, juce::Time::getCurrentTime(), 1, false));
        juce::MessageManager::getInstance()->runDispatchLoopUntil(1500);
        std::cout << "live editor loop OK\n";
        return same ? 0 : 1;
    }
    if (mode == "shots")
    {
        int w = 1600; if (argc > 3) w = juce::String(argv[3]).getIntValue();
        p.prepareToPlay(48000, 512);
        const juce::String start = argc > 4 ? argv[4] : "Pulsar Bloom";
        for (int i = 0; i < (int) zil::presets().size(); ++i) if (zil::presets()[(size_t) i].name == start) { p.loadPreset(i); break; }
        // play a chord so meters / voices light up
        { juce::AudioBuffer<float> b(2, 512); for (int k = 0; k < 40; ++k) { juce::MidiBuffer m; if (k == 0) for (int n : { 48, 55, 60 }) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8) 100), 0); p.processBlock(b, m); } }
        ZilEditor ed(p); ed.setSize(w, w * 9 / 16);
        const char* names[6] = { "osc", "lay", "mod", "fx", "arp", "set" };
        for (int i = 0; i < 6; ++i)
        {
            ed.showPage(i);
            auto img = ed.renderSnapshot();
            juce::PNGImageFormat png; auto f = out.getChildFile(juce::String(names[i]) + ".png"); f.deleteFile();
            if (auto os = f.createOutputStream()) png.writeImageToStream(img, *os);
        }
        return 0;
    }
    return 0;
}
