#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include <atomic>
#include <array>
#include <memory>

namespace zil
{
// ---- wavetables: 16 "ROM" instrument spectra + 5 synth shapes, mip-mapped (band-limited) ----
constexpr int kTableSize = 2048, kMips = 11, kNumTables = 21, kSineTable = 16;
struct Wavetables
{
    static const Wavetables& get();
    const float* tbl(int t, int m) const { return &data[(size_t) (t * kMips + m) * (kTableSize + 1)]; }
    std::vector<float> data;
private:
    Wavetables();
};

struct SVF
{
    float s1 = 0, s2 = 0;
    inline void reset() { s1 = s2 = 0; }
    // returns lp; bp/hp via out params
    inline float run(float x, float a1, float a2, float a3, float k, float& bp, float& hp)
    {
        float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2 * v1 - s1; s2 = 2 * v2 - s2;
        bp = v1; hp = x - k * v1 - v2; return v2;
    }
};
struct SVFCoef
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1;
    void set(float cutoff, float res01, double sr)
    {
        cutoff = juce::jlimit(15.f, (float) (sr * 0.45), cutoff);
        float g = std::tan(juce::MathConstants<float>::pi * cutoff / (float) sr);
        k = 2.f - 1.96f * juce::jlimit(0.f, 1.f, res01);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
};

struct Env
{
    enum Stage { Idle, Att, Dec, Sus, Rel } stage = Idle;
    float v = 0;
    struct C { float att = 1, dec = 0.9f, sus = 1, rel = 0.9f; };
    static C make(float a, float d, float s, float r, double sr)
    {
        C c; c.att = 1.f / (std::max(a, 0.0005f) * (float) sr); c.dec = std::exp(-4.6f / (std::max(d, 0.002f) * (float) sr));
        c.sus = s; c.rel = std::exp(-4.6f / (std::max(r, 0.002f) * (float) sr)); return c;
    }
    void on() { stage = Att; }
    void off() { if (stage != Idle) stage = Rel; }
    inline float tick(const C& c)
    {
        switch (stage)
        {
            case Att: v += c.att; if (v >= 1.f) { v = 1.f; stage = Dec; } break;
            case Dec: v = c.sus + (v - c.sus) * c.dec; if (v - c.sus < 1e-4f) { v = c.sus; stage = Sus; } break;
            case Sus: v = c.sus; if (c.sus < 1e-4f) { v = 0; stage = Idle; } break;
            case Rel: v *= c.rel; if (v < 1e-5f) { v = 0; stage = Idle; } break;
            default: v = 0; break;
        }
        return v;
    }
};

struct Ctx   // per-block constants shared by all voices
{
    Snap s; double sr = 44100; float bend = 0, modWheel = 0, lfo = 0, tuneSemis = 0;
    // macro-resolved values
    float cutOct = 0, drive = 0, tbl = 0, fm = 0, lvlC = 0, det = 0, envAmt = 0, vib = 0, driftCents = 0;
    int kit = 0;
};

class Voice
{
public:
    void start(int note, float vel, const Ctx& c, bool legato);
    void release() { gate = false; if (!sustainHeld) releaseEnvs(); }
    void releaseEnvs() { envA.off(); envB.off(); envC.off(); envF.off(); envAmp.off(); }
    void render(const Ctx& c, float* L, float* R, int n);
    bool active = false, gate = false, sustainHeld = false;
    int note = 60; float vel = 1; uint64_t age = 0;
    float curNote = 60, tgtNote = 60;
    Env envA, envB, envC, envF, envAmp;
private:
    float phA = 0, phB1 = 0, phB2 = 0, phM = 0, phK = 0, phK2 = 0;
    SVF fa, fbL, fbR, fcL, fcR, mfL[2], mfR[2], kn;
    float lpL = 0, lpR = 0, drift = 0, driftPh = 0;
    struct Grain { bool on = false; int age = 0, len = 1; float pan = 0; SVF f; float fc = 1000; } grains[4];
    // kit
    bool kitOn = false, metal = false; float kToneHz = 100, kPitchMul = 0, kPe = 1, kPeCoef = 0.99f, kTone = 1, kNoise = 0, kDecay = 0.3f, kNoiseHz = 5000; int kNoiseType = 0;
    juce::Random rng;
    float noise() { return rng.nextFloat() * 2.f - 1.f; }
};

struct ArpState
{
    std::vector<int> held, played;
    int physDown = 0, idx = 0, dir = 1, curNote = -1, curVel = 100;
    double posQ = 0, offAtQ = -1; int nextStep = 0, stepCounter = 0; uint32_t flips = 0; bool wasOn = false, init = false;
};

class Engine
{
public:
    void prepare(double sr, int maxBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, const Snap& s, double bpm, double ppq, bool playing);
    std::atomic<float> peakL { 0 }, peakR { 0 }, activity { 0 };
    std::atomic<int> nVoices { 0 }, midiCount { 0 };

private:
    void handleMidi(const juce::MidiMessage& m, const Ctx& c);
    void noteOn(int note, float vel, const Ctx& c);
    void noteOff(int note);
    void renderVoices(const Ctx& c, float* L, float* R, int n);
    void runArp(const juce::MidiBuffer& in, juce::MidiBuffer& out, const Snap& s, int n, double bpm, double ppq, bool playing);
    void fx(const Ctx& c, float* L, float* R, int n, double bpm);

    double sr = 44100;
    std::array<Voice, 16> voices;
    std::vector<int> monoStack;
    uint64_t counter = 0;
    float bendNorm = 0, modWheel = 0; bool pedal = false;
    double lfoPh = 0; float lfoSH = 0;
    ArpState arp; juce::Random rng;
    std::vector<float> bufL, bufR;

    // FX state
    SVF fmt[2][3]; SVFCoef fmtC[3]; float fmtGain = 1;
    double rmPh = 0; float bcHoldL = 0, bcHoldR = 0; int bcCount = 0;
    std::vector<float> smBuf[2]; int smW = 0;
    struct SGrain { bool on = false; float pos = 0, rate = 1; int age = 0, len = 1; float pan = 0; } sg[6];
    SVF mf[2][2]; float mfRelease = 1;
    std::vector<float> dl[2]; int dW = 0; float dTimeSm = 22050, dLp[2] { 0, 0 };
    juce::Reverb reverb; float limGain = 1;
    float lastWidth = 1;
};
}
