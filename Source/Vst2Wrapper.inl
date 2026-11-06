// VST2 wrapper for ZilProcessor. Included at the end of PluginProcessor.cpp so it is always linked into the plugin binary.
#include "Vst2.h"
#if !JUCE_MAC
namespace zilvst2
{
enum { effOpen = 0, effClose, effSetProgram, effGetProgram, effSetProgramName, effGetProgramName, effGetParamLabel, effGetParamDisplay, effGetParamName,
       effSetSampleRate = 10, effSetBlockSize, effMainsChanged, effEditGetRect, effEditOpen, effEditClose, effEditIdle = 19,
       effGetChunk = 23, effSetChunk, effProcessEvents, effCanBeAutomated, effGetPlugCategory = 35, effGetOutputProperties = 34,
       effGetEffectName = 45, effGetVendorString = 47, effGetProductString, effGetVendorVersion, effCanDo = 51, effGetVstVersion = 58,
       effGetProgramNameIndexed = 29 };
enum { audioMasterGetTime = 7, audioMasterSizeWindow = 15 };
enum { flagHasEditor = 1, flagCanReplacing = 16, flagProgramChunks = 32, flagIsSynth = 256 };

static void copyStr(void* dst, const juce::String& s, int maxLen) { if (dst) std::strncpy((char*) dst, s.toRawUTF8(), (size_t) maxLen - 1), ((char*) dst)[maxLen - 1] = 0; }

struct JuceLifetime
{
    static inline int count = 0; static inline bool started = false;
    static void acquire() { if (count++ == 0 && !started) { juce::initialiseJuce_GUI(); started = true; } }
    static void release() { if (--count == 0 && started) { juce::shutdownJuce_GUI(); started = false; } }
};

class Plugin : private juce::AudioPlayHead, private juce::ComponentListener
{
public:
    explicit Plugin(HostCb h) : host(h)
    {
        JuceLifetime::acquire();
        proc = std::make_unique<ZilProcessor>();
        proc->setPlayConfigDetails(0, 2, sampleRate, blockSize);
        proc->setPlayHead(this);
        std::memset(&fx, 0, sizeof(fx));
        fx.magic = 0x56737450; // 'VstP'
        fx.dispatcher = &dispatch; fx.process = &processAcc; fx.setParameter = &setParam; fx.getParameter = &getParam;
        fx.numPrograms = proc->getNumPrograms(); fx.numParams = zil::P_COUNT; fx.numInputs = 0; fx.numOutputs = 2;
        fx.flags = flagHasEditor | flagCanReplacing | flagProgramChunks | flagIsSynth;
        fx.uniqueID = 0x5a696c53; fx.version = 1000; fx.object = this; fx.processReplacing = &processRepl;
    }
    ~Plugin() override { closeEditor(); proc.reset(); JuceLifetime::release(); }
    AEffect fx;

private:
    static Plugin* self(AEffect* e) { return (Plugin*) e->object; }
    static void setParam(AEffect* e, int32_t i, float v)
    { auto* p = self(e); if (i >= 0 && i < zil::P_COUNT) if (auto* prm = p->proc->apvts.getParameter(zil::defs()[(size_t) i].id)) prm->setValueNotifyingHost(v); }
    static float getParam(AEffect* e, int32_t i)
    { auto* p = self(e); if (i >= 0 && i < zil::P_COUNT) if (auto* prm = p->proc->apvts.getParameter(zil::defs()[(size_t) i].id)) return prm->getValue(); return 0.f; }

    static void processRepl(AEffect* e, float**, float** out, int32_t n) { self(e)->render(out, n); }
    static void processAcc(AEffect* e, float**, float** out, int32_t n)
    {
        auto* p = self(e); std::vector<float> a((size_t) n), b((size_t) n); float* tmp[2] = { a.data(), b.data() };
        p->render(tmp, n); for (int i = 0; i < n; ++i) { out[0][i] += a[(size_t) i]; out[1][i] += b[(size_t) i]; }
    }
    void render(float** out, int n)
    {
        juce::AudioBuffer<float> buf(out, 2, n); juce::MidiBuffer m;
        { const juce::SpinLock::ScopedLockType l(lock); m.swapWith(pending); }
        proc->processBlock(buf, m);
    }

    // host transport -> AudioPlayHead
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo pi;
        if (host)
            if (auto* t = (VstTimeInfo*) host(const_cast<AEffect*>(&fx), audioMasterGetTime, 0, 512 | 1024, nullptr, 0.f))
            {
                if (t->flags & 1024) pi.setBpm(t->tempo);
                if (t->flags & 512) pi.setPpqPosition(t->ppqPos);
                pi.setIsPlaying((t->flags & 2) != 0);
            }
        return pi;
    }

    void closeEditor() { if (editor) { editor->removeComponentListener(this); editor->removeFromDesktop(); editor.reset(); } }
    void componentMovedOrResized(juce::Component& c, bool, bool wasResized) override
    { if (wasResized && host) host(&fx, audioMasterSizeWindow, c.getWidth(), c.getHeight(), nullptr, 0.f); }

    static intptr dispatch(AEffect* e, int32_t op, int32_t index, intptr value, void* ptr, float opt)
    {
        auto* p = self(e);
        switch (op)
        {
            case effOpen: return 0;
            case effClose: delete p; return 0;
            case effSetProgram: p->proc->loadPreset((int) value); return 0;
            case effGetProgram: return p->proc->getCurrentProgram();
            case effGetProgramName: copyStr(ptr, p->proc->getProgramName(p->proc->getCurrentProgram()), 24); return 0;
            case effGetProgramNameIndexed: if (index < 0 || index >= p->proc->getNumPrograms()) return 0; copyStr(ptr, p->proc->getProgramName(index), 24); return 1;
            case effGetParamLabel: copyStr(ptr, "", 8); return 0;
            case effGetParamDisplay:
                if (index >= 0 && index < zil::P_COUNT) copyStr(ptr, p->proc->apvts.getParameter(zil::defs()[(size_t) index].id)->getCurrentValueAsText(), 24);
                return 0;
            case effGetParamName: if (index >= 0 && index < zil::P_COUNT) copyStr(ptr, zil::defs()[(size_t) index].name, 24); return 0;
            case effSetSampleRate: p->sampleRate = opt; return 0;
            case effSetBlockSize: p->blockSize = juce::jmax(1, (int) value); return 0;
            case effMainsChanged:
                if (value) { p->proc->setPlayConfigDetails(0, 2, p->sampleRate, p->blockSize); p->proc->prepareToPlay(p->sampleRate, p->blockSize); }
                else p->proc->releaseResources();
                return 0;
            case effEditGetRect: p->rect = { 0, 0, (int16_t) 675, (int16_t) 1200 }; *(ERect**) ptr = &p->rect; return 1;
           #if JUCE_WINDOWS
            case effEditOpen:
            {
                p->closeEditor(); p->editor.reset(p->proc->createEditor());
                if (!p->editor) return 0;
                p->editor->setOpaque(true); p->editor->setVisible(false); p->editor->addToDesktop(0, ptr); p->editor->setVisible(true);
                p->editor->addComponentListener(p);
                p->rect = { 0, 0, (int16_t) p->editor->getHeight(), (int16_t) p->editor->getWidth() };
                return 1;
            }
            case effEditClose: p->closeEditor(); return 0;
           #endif
            case effGetChunk:
            {
                p->chunk.reset(); p->proc->getStateInformation(p->chunk); *(void**) ptr = p->chunk.getData(); return (intptr) p->chunk.getSize();
            }
            case effSetChunk: p->proc->setStateInformation(ptr, (int) value); return 0;
            case effProcessEvents:
            {
                auto* ev = (VstEvents*) ptr; const juce::SpinLock::ScopedLockType l(p->lock);
                for (int i = 0; i < ev->numEvents; ++i)
                    if (ev->events[i]->type == 1)
                    {
                        auto* m = (VstMidiEvent*) ev->events[i];
                        p->pending.addEvent(juce::MidiMessage((juce::uint8) m->midiData[0], (juce::uint8) m->midiData[1], (juce::uint8) m->midiData[2]), juce::jmax(0, m->deltaFrames));
                    }
                return 1;
            }
            case effCanBeAutomated: return 1;
            case effGetPlugCategory: return 2;
            case effGetOutputProperties:
            {
                if (index > 1) return 0; auto* pp = (VstPinProperties*) ptr; std::memset(pp, 0, sizeof(*pp));
                copyStr(pp->label, index == 0 ? "ZIL L" : "ZIL R", 64); pp->flags = 1 | 2; pp->arrangementType = 1; return 1;
            }
            case effGetEffectName: case effGetProductString: copyStr(ptr, "ZIL", 64); return 1;
            case effGetVendorString: copyStr(ptr, "Trill Tron", 64); return 1;
            case effGetVendorVersion: return 1000;
            case effGetVstVersion: return 2400;
            case effCanDo:
            {
                juce::String s((const char*) ptr);
                return (s == "receiveVstEvents" || s == "receiveVstMidiEvent" || s == "receiveVstTimeInfo") ? 1 : 0;
            }
            default: return 0;
        }
    }

    HostCb host; std::unique_ptr<ZilProcessor> proc; std::unique_ptr<juce::AudioProcessorEditor> editor;
    double sampleRate = 44100.0; int blockSize = 512; ERect rect { 0, 0, 675, 1200 };
    juce::SpinLock lock; juce::MidiBuffer pending; juce::MemoryBlock chunk;
};
}

ZIL_VST2_EXPORT zilvst2::AEffect* VSTPluginMain(zilvst2::HostCb host)
{
    if (host && host(nullptr, 1 /* audioMasterVersion */, 0, 0, nullptr, 0.f) == 0) return nullptr;   // not a VST2 host
    return &(new zilvst2::Plugin(host))->fx;
}
#endif
