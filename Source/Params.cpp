#include "Params.h"

namespace zil
{
juce::String formatValue(Fmt f, float v)
{
    using juce::String;
    switch (f)
    {
        case Fmt::Hz:    return v >= 1000.f ? String(v / 1000.f, 1) + "k" : String((int) std::round(v)) + "Hz";
        case Fmt::HzF:   return String(v, v < 10.f ? 2 : 1) + "Hz";
        case Fmt::Db:    return (v > 0.05f ? "+" : "") + String(v, 1) + "dB";
        case Fmt::Sec:   return v < 1.f ? String((int) std::round(v * 1000.f)) + "ms" : String(v, 1) + "s";
        case Fmt::Ms:    return String((int) std::round(v)) + "ms";
        case Fmt::Pct:   return String((int) std::round(v * 100.f)) + "%";
        case Fmt::Semi:  return (v > 0.05f ? "+" : "") + String(v, 1) + "st";
        case Fmt::Cents: return (v > 0.5f ? "+" : "") + String((int) std::round(v)) + "ct";
        case Fmt::Pan:   return std::abs(v) < 0.01f ? String("C") : String(v < 0 ? "L" : "R") + String((int) std::round(std::abs(v) * 100.f));
        case Fmt::Signed:return (v > 0.5f ? "+" : "") + String((int) std::round(v));
        case Fmt::Int:   return String((int) std::round(v));
        default:         return String(v, 2);
    }
}

static std::vector<PDef> buildDefs()
{
    std::vector<PDef> t;   // unordered table, re-sorted below
    auto F = [&](const char* id, const char* nm, float mn, float mx, float def, float mid, Fmt fm) {
        PDef d; d.id = id; d.name = nm; d.type = 0; d.mn = mn; d.mx = mx; d.def = def; d.mid = mid; d.fmt = fm; t.push_back(d); };
    auto B = [&](const char* id, const char* nm, bool def) {
        PDef d; d.id = id; d.name = nm; d.type = 1; d.mn = 0; d.mx = 1; d.def = def ? 1.f : 0.f; t.push_back(d); };
    auto C = [&](const char* id, const char* nm, const char* list, int def) {
        PDef d; d.id = id; d.name = nm; d.type = 2; d.choices = juce::StringArray::fromTokens(list, "|", ""); d.mn = 0;
        d.mx = (float) (d.choices.size() - 1); d.def = (float) def; t.push_back(d); };
    auto I = [&](const char* id, const char* nm, int mn, int mx, int def, Fmt fm) {
        PDef d; d.id = id; d.name = nm; d.type = 3; d.mn = (float) mn; d.mx = (float) mx; d.def = (float) def; d.fmt = fm; t.push_back(d); };

    F("master", "Master", -60, 6, 0, 0, Fmt::Db);

    F("fCut", "Filter Cutoff", 20, 20000, 2400, 1000, Fmt::Hz);
    F("fRes", "Filter Resonance", 0, 1, 0.42f, 0, Fmt::Pct);
    F("fDrv", "Filter Drive", 0, 24, 3.1f, 0, Fmt::Db);
    F("fKey", "Filter Key Track", 0, 1, 0.5f, 0, Fmt::Pct);
    F("fEnv", "Filter Env Amount", -100, 100, 74, 0, Fmt::Signed);
    F("fLfo", "Filter LFO Amount", 0, 1, 0.22f, 0, Fmt::Pct);
    F("lfoRate", "LFO Rate", 0.05f, 20, 4, 1.5f, Fmt::HzF);
    F("fMix", "Filter Mix", 0, 1, 0.86f, 0, Fmt::Pct);
    B("fA", "Filter Route A", true);
    B("fB", "Filter Route B", true);
    F("fAtk", "Filter Env Attack", 0.001f, 5, 0.005f, 0.3f, Fmt::Sec);
    F("fDec", "Filter Env Decay", 0.005f, 5, 0.6f, 0.5f, Fmt::Sec);
    F("fSus", "Filter Env Sustain", 0, 1, 0.3f, 0, Fmt::Pct);
    F("fRel", "Filter Env Release", 0.005f, 8, 0.5f, 0.5f, Fmt::Sec);
    C("lfoShape", "LFO Shape", "SIN|TRI|SAW|SQR|S&H", 0);
    B("lfoSync", "LFO Sync", false);
    C("lfoDiv", "LFO Division", "1/1|1/2|1/4|1/8|1/16", 3);

    F("aAtk", "Amp Attack", 0.001f, 8, 0.012f, 0.4f, Fmt::Sec);
    F("aDec", "Amp Decay", 0.005f, 8, 0.46f, 0.6f, Fmt::Sec);
    F("aSus", "Amp Sustain", -60, 0, -3, 0, Fmt::Db);
    F("aRel", "Amp Release", 0.005f, 12, 1.2f, 0.8f, Fmt::Sec);
    F("aVel", "Velocity Sens", 0, 1, 0.64f, 0, Fmt::Pct);
    F("aPan", "Pan", -1, 1, 0, 0, Fmt::Pan);
    F("aWid", "Stereo Width", 0, 2, 1.6f, 0, Fmt::Pct);
    F("aGain", "Amp Gain", -24, 24, 2, 0, Fmt::Db);
    B("lAOn", "Layer A On", true);
    B("lBOn", "Layer B On", true);
    B("lCOn", "Layer C On", true);

    const char* L[3] = { "A", "B", "C" };
    const float lvl[3] = { 0.8f, 0.5f, 0.15f };
    for (int i = 0; i < 3; ++i)
    {
        juce::String p = juce::String("l") + L[i], n = juce::String("Layer ") + L[i] + " ";
        auto id = [&](const char* s) { return (p + s).toStdString(); };
        auto nm = [&](const char* s) { return (n + s).toStdString(); };
        F(id("Lvl").c_str(), nm("Level").c_str(), 0, 1, lvl[i], 0, Fmt::Pct);
        F(id("Tune").c_str(), nm("Tune").c_str(), -24, 24, 0, 0, Fmt::Semi);
        F(id("Cut").c_str(), nm("Cutoff").c_str(), 0, 1, 1, 0, Fmt::Pct);
        F(id("Res").c_str(), nm("Resonance").c_str(), 0, 1, 0.1f, 0, Fmt::Pct);
        F(id("Atk").c_str(), nm("Attack").c_str(), 0.001f, 8, 0.005f, 0.3f, Fmt::Sec);
        F(id("Dec").c_str(), nm("Decay").c_str(), 0.005f, 10, 1.5f, 1.0f, Fmt::Sec);
        F(id("Sus").c_str(), nm("Sustain").c_str(), 0, 1, 1, 0, Fmt::Pct);
        F(id("Rel").c_str(), nm("Release").c_str(), 0.005f, 12, 0.4f, 0.6f, Fmt::Sec);
    }
    C("aInst", "ROM Instrument",
      "GRAND PIANO|FELT PIANO|RHODES|WURLITZER|CLAVINET|B3 ORGAN|PIPE ORGAN|STRINGS|BRASS|REED|GUITAR|MARIMBA|KALIMBA|SUB BASS|CHOIR|ALIEN", 0);
    F("bTbl", "Wavetable Position", 0, 1, 0.3f, 0, Fmt::Pct);
    F("bFM", "FM Amount", 0, 1, 0, 0, Fmt::Pct);
    F("bRat", "FM Ratio", 0.5f, 8, 2, 2, Fmt::Raw);
    F("bDet", "Detune / Spread", 0, 1, 0.2f, 0, Fmt::Pct);
    F("cCol", "Noise Colour", 0, 1, 0.5f, 0, Fmt::Pct);
    F("cGrn", "Grain Amount", 0, 1, 0.3f, 0, Fmt::Pct);
    F("cDns", "Grain Density", 0, 1, 0.5f, 0, Fmt::Pct);

    F("ftMix", "Formant Mix", 0, 1, 0, 0, Fmt::Pct);
    F("ftShift", "Formant Shift", 0, 1, 0.5f, 0, Fmt::Pct);
    F("rmMix", "Ring Mod Mix", 0, 1, 0, 0, Fmt::Pct);
    F("rmFreq", "Ring Mod Freq", 20, 4000, 440, 400, Fmt::Hz);
    F("bcMix", "Bitcrush Mix", 0, 1, 0, 0, Fmt::Pct);
    F("bcBits", "Bitcrush Bits", 2, 16, 8, 0, Fmt::Raw);
    F("bcRate", "Bitcrush Rate", 0, 1, 0.3f, 0, Fmt::Pct);
    F("smMix", "Smear Mix", 0, 1, 0, 0, Fmt::Pct);
    F("smSize", "Smear Size", 0, 1, 0.5f, 0, Fmt::Pct);

    B("mfOn", "Master Filter On", true);
    C("mfType", "Master Filter Type", "LP|BP|HP", 0);
    F("mfFreq", "Master Filter Freq", 20, 20000, 20000, 1000, Fmt::Hz);
    F("mfQ", "Master Filter Q", 0, 1, 0.1f, 0, Fmt::Pct);

    B("dOn", "Delay On", false);
    B("dSync", "Delay Sync", true);
    C("dDiv", "Delay Division", "1/4|1/8|1/16", 1);
    F("dTime", "Delay Time", 1, 1500, 350, 300, Fmt::Ms);
    F("dFb", "Delay Feedback", 0, 0.95f, 0.4f, 0, Fmt::Pct);
    F("dMix", "Delay Mix", 0, 1, 0.25f, 0, Fmt::Pct);

    B("rOn", "Reverb On", true);
    C("rType", "Reverb Type", "HALL|PLATE|ROOM", 0);
    F("rSize", "Reverb Size", 0, 1, 0.78f, 0, Fmt::Pct);
    F("rWet", "Reverb Wet", 0, 1, 0.3f, 0, Fmt::Pct);

    B("oLim", "Limiter", true);
    C("oMode", "Output Mode", "ST|MONO", 0);
    F("oVol", "Output Volume", -60, 6, -2, 0, Fmt::Db);

    const char* mn[8] = { "CUT", "MORPH", "SPACE", "GRIT", "WARP", "AIR", "DRIFT", "PUNCH" };
    for (int i = 0; i < 8; ++i)
    {
        auto id = ("m" + juce::String(i + 1)).toStdString();
        auto nm = ("Macro " + juce::String(i + 1) + " " + mn[i]).toStdString();
        F(id.c_str(), nm.c_str(), 0, 1, i == 0 ? 0.5f : 0.f, 0, Fmt::Pct);
    }

    B("arpOn", "Arp On", false);
    C("arpMode", "Arp Mode", "UP|DOWN|UP/DN|RANDOM|PLAYED", 0);
    C("arpRate", "Arp Rate", "1/4|1/8|1/16|1/32", 2);
    I("arpOct", "Arp Octaves", 1, 4, 2, Fmt::Int);
    F("arpGate", "Arp Gate", 0.05f, 1, 0.6f, 0, Fmt::Pct);
    F("arpSwing", "Arp Swing", 0, 0.6f, 0, 0, Fmt::Pct);
    B("arpHold", "Arp Hold", false);
    F("arpEvo", "Arp Evolve", 0, 1, 0, 0, Fmt::Pct);
    for (int i = 0; i < 16; ++i)
    {
        auto id = ("st" + juce::String(i)).toStdString();
        B(id.c_str(), ("Arp Step " + juce::String(i + 1)).toStdString().c_str(), true);
    }

    I("voices", "Voices", 1, 16, 8, Fmt::Int);
    F("glide", "Glide", 0, 2, 0, 0.2f, Fmt::Sec);
    I("bend", "Bend Range", 1, 24, 2, Fmt::Int);
    F("tune", "Master Tune", -100, 100, 0, 0, Fmt::Cents);
    B("tilt", "UI Tilt", true);
    C("kit", "Keymap", "OFF|ACOU|ELEC|METAL|PERC", 0);

    // order by ZIL_IDS
    std::vector<PDef> out;
    const char* ids[] = {
#define X(n) #n,
        ZIL_IDS(X)
#undef X
    };
    for (auto* id : ids)
    {
        auto it = std::find_if(t.begin(), t.end(), [&](const PDef& d) { return d.id == id; });
        jassert(it != t.end());
        out.push_back(*it);
    }
    jassert(out.size() == (size_t) P_COUNT);
    return out;
}

const std::vector<PDef>& defs() { static const std::vector<PDef> d = buildDefs(); return d; }

int indexOf(const juce::String& id)
{
    const auto& d = defs();
    for (size_t i = 0; i < d.size(); ++i) if (d[i].id == id) return (int) i;
    return -1;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : defs())
    {
        juce::ParameterID pid { d.id, 1 };
        if (d.type == 0)
        {
            juce::NormalisableRange<float> r(d.mn, d.mx);
            if (d.mid > 0.f) r.setSkewForCentre(d.mid);
            auto fmt = d.fmt;
            layout.add(std::make_unique<juce::AudioParameterFloat>(pid, d.name, r, d.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction([fmt](float v, int) { return formatValue(fmt, v); })));
        }
        else if (d.type == 1)
            layout.add(std::make_unique<juce::AudioParameterBool>(pid, d.name, d.def > 0.5f));
        else if (d.type == 2)
            layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, d.choices, (int) d.def));
        else
            layout.add(std::make_unique<juce::AudioParameterInt>(pid, d.name, (int) d.mn, (int) d.mx, (int) d.def));
    }
    return layout;
}
}
