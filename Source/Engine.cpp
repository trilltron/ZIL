#include "Engine.h"
using juce::jlimit;
using juce::jmax;
using juce::jmin;

namespace zil
{
static constexpr float kPi = juce::MathConstants<float>::pi;
static constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

// ------------------------------------------------------------------ wavetables
static float harmAmp(int t, int n)
{
    const float x = (float) n;
    auto at = [&](std::initializer_list<std::pair<int, float>> l) { for (auto& p : l) if (p.first == n) return p.second; return 0.f; };
    switch (t)
    {
        case 0:  return std::pow(x, -0.85f) * (1.f - 0.85f * std::exp(-(x - 8.f) * (x - 8.f) / 2.f));          // grand piano
        case 1:  return std::pow(x, -1.7f);                                                                      // felt piano
        case 2:  return at({ {1,1.f},{2,.22f},{3,.08f},{5,.05f},{7,.30f},{9,.12f} });                           // rhodes (tine)
        case 3:  return ((n & 1) ? 1.f : 0.35f) * std::pow(x, -1.25f);                                          // wurlitzer reed
        case 4:  return n <= 28 ? std::pow(x, -0.55f) * ((n & 1) ? 1.f : 0.75f) : 0.f;                          // clav
        case 5:  return at({ {1,1.f},{2,.85f},{3,.6f},{4,.5f},{6,.35f},{8,.3f} });                              // B3 drawbars
        case 6:  return n <= 20 ? std::pow(x, -0.75f) : 0.f;                                                    // pipe organ
        case 7:  return n <= 70 ? std::exp(-x / 55.f) / x : 0.f;                                                // strings
        case 8:  return std::pow(x, -0.35f) * std::exp(-x / 12.f);                                              // brass
        case 9:  return n <= 40 ? ((n & 1) ? 1.f : 0.3f) * std::pow(x, -0.8f) : 0.f;                            // reed
        case 10: return std::pow(x, -1.1f) * std::abs(std::sin(kPi * x * 0.16f)) * 2.f;                         // guitar (pluck pos.)
        case 11: return at({ {1,1.f},{4,.4f},{10,.15f} });                                                      // marimba bar
        case 12: return at({ {1,1.f},{3,.45f},{4,.3f},{6,.22f},{9,.1f} });                                      // kalimba/bell
        case 13: return at({ {1,1.f},{2,.45f},{3,.2f},{4,.1f} });                                               // sub bass
        case 14: return std::pow(x, -0.7f) * (0.2f + std::exp(-(x - 6) * (x - 6) / 8.f) + .6f * std::exp(-(x - 10) * (x - 10) / 10.f)
                                              + .3f * std::exp(-(x - 24) * (x - 24) / 30.f));                    // choir formants
        case 15: { if (n < 2) return 1.f; bool pr = true; for (int d = 2; d * d <= n; ++d) if (n % d == 0) { pr = false; break; }
                   return (pr ? 1.f : 0.06f) * std::pow(x, -0.6f); }                                             // alien
        case 16: return n == 1 ? 1.f : 0.f;                                                                      // sine
        case 17: return (n & 1) ? (((n - 1) / 2) & 1 ? -1.f : 1.f) / (x * x) : 0.f;                              // triangle
        case 18: return 1.f / x;                                                                                 // saw
        case 19: return (n & 1) ? 1.f / x : 0.f;                                                                 // square
        case 20: return at({ {1,1.f},{2,.7f},{3,.6f},{5,.45f},{7,.4f},{11,.3f},{13,.25f},{17,.2f} });            // glass
        default: return 0.f;
    }
}
static int tableMaxHarm(int t) { return (t == 17 || t == 18 || t == 19) ? 1023 : (t >= 16 ? 40 : 80); }

Wavetables::Wavetables()
{
    data.assign((size_t) kNumTables * kMips * (kTableSize + 1), 0.f);
    std::vector<float> sinTab(kTableSize);
    for (int i = 0; i < kTableSize; ++i) sinTab[(size_t) i] = std::sin(kTwoPi * (float) i / (float) kTableSize);
    for (int t = 0; t < kNumTables; ++t)
    {
        float norm = 1.f;
        for (int m = 0; m < kMips; ++m)
        {
            float* d = const_cast<float*>(tbl(t, m));
            int H = jmin(tableMaxHarm(t), 1024 >> m);
            for (int n = 1; n <= H; ++n)
            {
                float a = harmAmp(t, n);
                if (a == 0.f) continue;
                for (int i = 0, idx = 0; i < kTableSize; ++i, idx = (idx + n) & (kTableSize - 1))
                    d[i] += a * sinTab[(size_t) idx];
            }
            if (m == 0)
            {
                double ss = 0; for (int i = 0; i < kTableSize; ++i) ss += d[i] * d[i];
                float rms = (float) std::sqrt(ss / kTableSize);
                norm = rms > 1e-6f ? 0.3f / rms : 1.f;
            }
            for (int i = 0; i < kTableSize; ++i) d[i] *= norm;
            d[kTableSize] = d[0];
        }
    }
}
const Wavetables& Wavetables::get() { static Wavetables w; return w; }

static inline float lut(const float* t, float ph)
{
    float f = ph * (float) kTableSize; int i = (int) f; float fr = f - (float) i;
    return t[i] + (t[i + 1] - t[i]) * fr;
}
static inline float wrap01(float p) { return p - std::floor(p); }
static inline int mipFor(float hz, double sr)
{
    int allowed = (int) (0.45 * sr / jmax(hz, 1.f));
    if (allowed >= 1024) return 0;
    return jlimit(0, kMips - 1, (int) std::ceil(std::log2(1024.0 / jmax(allowed, 1))));
}
static inline float semi(float s) { return std::exp2(s / 12.f); }
static inline float soft(float x) { x = jlimit(-3.f, 3.f, x); return x * (27.f + x * x) / (27.f + 9.f * x * x); }

// ------------------------------------------------------------------ voice
void Voice::start(int n, float v, const Ctx& c, bool legato)
{
    note = n; vel = v; active = true; gate = true; sustainHeld = false; age = 0; tgtNote = (float) n;
    kitOn = c.kit > 0;
    if (legato && !kitOn) return;
    curNote = tgtNote;
    envA.on(); envB.on(); envC.on(); envF.on(); envAmp.on();
    phA = 0; phB1 = rng.nextFloat(); phB2 = rng.nextFloat(); phM = 0; phK = 0; phK2 = 0;
    fa.reset(); fbL.reset(); fbR.reset(); fcL.reset(); fcR.reset(); kn.reset();
    for (auto& f : mfL) f.reset();
    for (auto& f : mfR) f.reset();
    lpL = lpR = 0; driftPh = rng.nextFloat();
    for (auto& g : grains) g.on = false;
    if (kitOn)
    {
        const int style = c.kit; metal = (style == 3); kPe = 1.f;
        auto H = [&](float hz, float pm, float peMs, float tone, float nz, float dec, float nhz, int nt) {
            kToneHz = hz; kPitchMul = pm; kPeCoef = std::exp(-1.f / (peMs * 0.001f * (float) c.sr)); kTone = tone; kNoise = nz;
            kDecay = dec; kNoiseHz = nhz; kNoiseType = nt; };
        const bool e = style == 2;
        if (style == 4)            H(440.f * semi((float) n - 69.f), 0.35f, 25, 0.9f, 0.18f, 0.35f, 2500, 1);
        else if (n <= 36)          H(e ? 48.f : 58.f, e ? 3.5f : 5.f, e ? 55.f : 28.f, 1.f, 0.12f, e ? 1.1f : 0.28f, 3000, 0);
        else if (n == 37 || n == 38 || n == 40) H(style == 1 ? 185.f : 210.f, 0.6f, 12, 0.55f, 0.9f, e ? 0.3f : 0.22f, 2200, 1);
        else if (n == 39)          H(200.f, 0.f, 10, 0.f, 1.f, 0.22f, 1300, 1);
        else if (n == 42)          H(300.f, 0.f, 10, metal ? 0.4f : 0.f, 1.f, 0.045f, 7500, 0);
        else if (n == 44)          H(300.f, 0.f, 10, metal ? 0.4f : 0.f, 1.f, 0.09f, 7000, 0);
        else if (n == 46)          H(300.f, 0.f, 10, metal ? 0.4f : 0.f, 1.f, 0.45f, 6500, 0);
        else if (n == 49 || n == 57) H(420.f, 0.f, 10, metal ? 0.5f : 0.15f, 1.f, 1.4f, 3500, 0);
        else if (n == 51 || n == 53 || n == 59) H(520.f, 0.f, 10, metal ? 0.5f : 0.2f, 0.8f, 0.9f, 4500, 0);
        else if (n >= 41 && n <= 50) H(80.f + (float) (n - 41) * 18.f, 1.2f, 40, 1.f, 0.1f, 0.35f, 2500, 1);
        else                       H(440.f * semi((float) n - 69.f) * 0.5f, 0.3f, 20, 0.9f, 0.15f, 0.35f, 2500, 1);
    }
}

void Voice::render(const Ctx& c, float* L, float* R, int n)
{
    const Snap& s = c.s; const auto& W = Wavetables::get(); const float sr = (float) c.sr;

    if (s.glide > 0.0005f && !kitOn) curNote += (tgtNote - curNote) * (1.f - std::exp(-(float) n / (s.glide * sr * 0.35f)));
    else curNote = tgtNote;
    driftPh += (float) n * 0.15f / sr; if (driftPh > 1.f) driftPh -= 1.f;
    const float semis = curNote + c.tuneSemis + c.bend + c.vib * c.lfo + std::sin(driftPh * kTwoPi) * c.driftCents * 0.01f;
    const float hz = 440.f * semi(semis - 69.f);

    const auto cF = Env::make(s.fAtk, s.fDec, s.fSus, s.fRel, sr);
    const auto cAmp = kitOn ? Env::make(0.0004f, kDecay, 0.f, 0.05f, sr)
                            : Env::make(s.aAtk, s.aDec, juce::Decibels::decibelsToGain(s.aSus, -60.f), s.aRel, sr);
    const auto cA = Env::make(s.lAAtk, s.lADec, s.lASus, s.lARel, sr);
    const auto cB = Env::make(s.lBAtk, s.lBDec, s.lBSus, s.lBRel, sr);
    const auto cC = Env::make(s.lCAtk, s.lCDec, s.lCSus, s.lCRel, sr);

    // main (Filter Modifier) filter
    float cutHz = s.fCut * std::exp2(c.cutOct + envF.v * c.envAmt * 0.01f * 6.f + c.lfo * s.fLfo * 2.5f + s.fKey * (curNote - 60.f) / 12.f);
    SVFCoef mc; mc.set(cutHz, s.fRes, sr);
    const float dg = juce::Decibels::decibelsToGain(c.drive);
    const float dgComp = 1.f / (1.f + (dg - 1.f) * 0.25f);

    // layer filters
    SVFCoef ca, cb, cc;
    auto lf = [&](float cut, float res, float env, SVFCoef& co) {
        if (cut >= 0.995f) return false;
        co.set(100.f * std::pow(200.f, cut) * (0.35f + 0.65f * env), res, sr); return true; };
    const bool useA = lf(s.lACut, s.lARes, envA.v, ca), useB = lf(s.lBCut, s.lBRes, envB.v, cb), useC = lf(s.lCCut, s.lCRes, envC.v, cc);

    const float lvlA = s.lAOn > 0.5f ? s.lALvl : 0.f, lvlB = s.lBOn > 0.5f ? s.lBLvl : 0.f, lvlC = s.lCOn > 0.5f ? c.lvlC : 0.f;
    const float hzA = hz * semi(s.lATune), hzB = hz * semi(s.lBTune);
    const float incA = hzA / sr;
    const float detRatio = semi(c.det * 0.25f + 0.0f);   // up to ~25 cents
    const float incB1 = hzB / detRatio / sr, incB2 = hzB * detRatio / sr, incM = hzB * s.bRat / sr;
    const float* tA = W.tbl((int) s.aInst, mipFor(hzA, sr));
    const float tpos = jlimit(0.f, 1.f, c.tbl) * 4.f; const int ti = jmin(3, (int) tpos); const float tw = tpos - (float) ti;
    const int mipB = mipFor(hzB * detRatio * (1.f + c.fm * 2.f), sr);
    static const int shapeTbl[5] = { 16, 17, 18, 19, 20 };
    const float* tb0 = W.tbl(shapeTbl[ti], mipB); const float* tb1 = W.tbl(shapeTbl[ti + 1], mipB);
    const float* tSin = W.tbl(kSineTable, 0);
    const float spread = 0.25f + 0.75f * jmin(1.f, c.det * 1.5f);
    const float colour = s.cCol;
    const float kc = 0.02f + 0.3f * colour;
    const float velGain = ((1.f - s.aVel) + s.aVel * std::pow(vel, 0.9f)) * 0.42f;

    // grains: spawn at control rate
    const bool grainOn = !kitOn && lvlC > 0.001f && s.cGrn > 0.001f;
    if (grainOn)
        for (auto& g : grains)
            if (!g.on && rng.nextFloat() < 0.01f + 0.45f * s.cDns * ((float) n / 32.f))
            {
                g.on = true; g.age = 0; g.len = (int) ((0.02f + 0.1f * (1.f - 0.7f * s.cDns) * rng.nextFloat()) * sr) + 8;
                g.pan = rng.nextFloat() * 2.f - 1.f;
                g.fc = jlimit(150.f, 12000.f, hz * (float) (1 << rng.nextInt(4)) * (0.97f + 0.06f * rng.nextFloat()));
                g.f.reset();
            }
    SVFCoef gc[4]; if (grainOn) for (int i = 0; i < 4; ++i) gc[i].set(grains[i].fc, 0.9f, sr);

    SVFCoef kc2; if (kitOn) kc2.set(kNoiseHz, kNoiseType == 1 ? 0.55f : 0.2f, sr);

    for (int i = 0; i < n; ++i)
    {
        const float eAmp = envAmp.tick(cAmp); const float eF = envF.tick(cF);
        float aL = 0, aR = 0, bL = 0, bR = 0, cL = 0, cR = 0, bp, hp;

        if (kitOn)
        {
            kPe *= kPeCoef;
            const float f = kToneHz * (1.f + kPitchMul * kPe);
            float tone;
            if (metal) { tone = lut(W.tbl(19, 0), wrap01(phK)) * lut(W.tbl(19, 0), wrap01(phK2)) * 1.6f; phK2 += f * 1.4142f / sr; }
            else tone = lut(tSin, wrap01(phK)) * 2.357f * 0.5f;
            phK += f / sr; if (phK >= 1.f) phK -= 1.f; if (phK2 >= 1.f) phK2 -= 1.f;
            const float nz = noise(); const float lpv = kn.run(nz, kc2.a1, kc2.a2, kc2.a3, kc2.k, bp, hp);
            const float nf = kNoiseType == 1 ? bp * kc2.k : hp;
            (void) lpv;
            cL = cR = tone * kTone + nf * kNoise * 0.9f;
        }
        else
        {
            const float eA = envA.tick(cA), eB = envB.tick(cB), eC = envC.tick(cC);
            // ---- layer A (ROM instrument)
            float a = lut(tA, phA) * lvlA * eA; phA += incA; if (phA >= 1.f) phA -= 1.f;
            if (useA) a = fa.run(a, ca.a1, ca.a2, ca.a3, ca.k, bp, hp);
            aL = aR = a;
            // ---- layer B (wavetable + FM)
            if (lvlB > 0.0005f)
            {
                const float mod = lut(tSin, phM) * 2.357f; phM += incM; if (phM >= 1.f) phM -= 1.f;
                const float off = c.fm * std::sqrt(eB) * 1.3f * mod;
                const float p1 = wrap01(phB1 + off), p2 = wrap01(phB2 + off);
                const float b1 = lut(tb0, p1) * (1.f - tw) + lut(tb1, p1) * tw, b2 = lut(tb0, p2) * (1.f - tw) + lut(tb1, p2) * tw;
                phB1 += incB1; if (phB1 >= 1.f) phB1 -= 1.f; phB2 += incB2; if (phB2 >= 1.f) phB2 -= 1.f;
                const float g = lvlB * eB;
                bL = (b1 * (0.5f + 0.5f * spread) + b2 * (0.5f - 0.5f * spread)) * g;
                bR = (b2 * (0.5f + 0.5f * spread) + b1 * (0.5f - 0.5f * spread)) * g;
                if (useB) { bL = fbL.run(bL, cb.a1, cb.a2, cb.a3, cb.k, bp, hp); bR = fbR.run(bR, cb.a1, cb.a2, cb.a3, cb.k, bp, hp); }
            }
            else { phB1 += incB1; if (phB1 >= 1.f) phB1 -= 1.f; phB2 += incB2; if (phB2 >= 1.f) phB2 -= 1.f; phM += incM; if (phM >= 1.f) phM -= 1.f; }
            // ---- layer C (noise + granular smear)
            if (lvlC > 0.0005f)
            {
                const float nl = noise(), nr = noise();
                lpL += kc * (nl - lpL); lpR += kc * (nr - lpR);
                float xl, xr;
                if (colour < 0.5f) { const float t = colour * 2.f; xl = lpL * 3.f * (1.f - t) + nl * t; xr = lpR * 3.f * (1.f - t) + nr * t; }
                else { const float t = (colour - 0.5f) * 2.f; xl = nl * (1.f - t) + (nl - lpL) * t; xr = nr * (1.f - t) + (nr - lpR) * t; }
                float gl = 0, gr = 0;
                if (grainOn)
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& g = grains[k]; if (!g.on) continue;
                        const float w = 0.5f - 0.5f * std::cos(kTwoPi * (float) g.age / (float) g.len);
                        const float y = g.f.run((nl + nr) * 0.5f, gc[k].a1, gc[k].a2, gc[k].a3, gc[k].k, bp, hp);
                        const float o = bp * gc[k].k * w * 1.4f;
                        gl += o * (0.5f - 0.5f * g.pan); gr += o * (0.5f + 0.5f * g.pan); (void) y;
                        if (++g.age >= g.len) g.on = false;
                    }
                const float gm = s.cGrn;
                const float gg = lvlC * eC;
                cL = (xl * (1.f - gm) + gl * gm * 2.f) * gg; cR = (xr * (1.f - gm) + gr * gm * 2.f) * gg;
                if (useC) { cL = fcL.run(cL, cc.a1, cc.a2, cc.a3, cc.k, bp, hp); cR = fcR.run(cR, cc.a1, cc.a2, cc.a3, cc.k, bp, hp); }
            }
        }

        // ---- Filter Modifier: routes A/B (and always C) through a drive + 24 dB LP
        const bool rA = s.fA > 0.5f, rB = s.fB > 0.5f;
        float inL = (rA ? aL : 0.f) + (rB ? bL : 0.f) + cL, inR = (rA ? aR : 0.f) + (rB ? bR : 0.f) + cR;
        const float dryL = (rA ? 0.f : aL) + (rB ? 0.f : bL), dryR = (rA ? 0.f : aR) + (rB ? 0.f : bR);
        float xl = soft(inL * dg), xr = soft(inR * dg);
        xl = mfL[0].run(xl, mc.a1, mc.a2, mc.a3, mc.k, bp, hp); xl = mfL[1].run(xl, mc.a1, mc.a2, mc.a3, mc.k, bp, hp);
        xr = mfR[0].run(xr, mc.a1, mc.a2, mc.a3, mc.k, bp, hp); xr = mfR[1].run(xr, mc.a1, mc.a2, mc.a3, mc.k, bp, hp);
        const float mix = s.fMix;
        const float outL = dryL + inL * (1.f - mix) + xl * dgComp * mix, outR = dryR + inR * (1.f - mix) + xr * dgComp * mix;
        const float g = eAmp * velGain;
        L[i] += outL * g; R[i] += outR * g;
    }
    age += (uint64_t) n;
    if (envAmp.stage == Env::Idle) active = false;
}

// ------------------------------------------------------------------ engine
void Engine::prepare(double sampleRate, int maxBlock)
{
    sr = sampleRate;
    int dsz = 1; while ((double) dsz < sr * 3.5) dsz <<= 1;
    for (auto& d : dl) d.assign((size_t) dsz, 0.f);
    for (auto& d : smBuf) d.assign(131072, 0.f);
    juce::ignoreUnused(maxBlock);
    reverb.setSampleRate(sr);
    reset();
}
void Engine::reset()
{
    for (auto& v : voices) { v = Voice(); }
    monoStack.clear(); arp = ArpState(); bendNorm = 0; modWheel = 0; pedal = false; limGain = 1; lfoPh = 0;
    for (auto& d : dl) std::fill(d.begin(), d.end(), 0.f);
    for (auto& d : smBuf) std::fill(d.begin(), d.end(), 0.f);
    for (auto& g : sg) g.on = false;
    for (auto& a : fmt) for (auto& f : a) f.reset();
    for (auto& a : mf) for (auto& f : a) f.reset();
    dLp[0] = dLp[1] = 0; reverb.reset();
}

void Engine::noteOn(int note, float vel, const Ctx& c)
{
    midiCount++;
    const int maxV = jlimit(1, 16, (int) c.s.voices);
    const bool mono = maxV == 1 && c.kit == 0;
    if (mono)
    {
        monoStack.erase(std::remove(monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back(note);
        Voice& v = voices[0];
        const bool legato = v.active && v.gate && monoStack.size() > 1;
        v.start(note, vel, c, legato); v.age = counter++;
        return;
    }
    Voice* pick = nullptr;
    for (int i = 0; i < maxV; ++i) if (voices[(size_t) i].active && voices[(size_t) i].note == note) { pick = &voices[(size_t) i]; break; }
    if (!pick) for (int i = 0; i < maxV; ++i) if (!voices[(size_t) i].active) { pick = &voices[(size_t) i]; break; }
    if (!pick)
    {
        float best = 1e9f;
        for (int i = 0; i < maxV; ++i)
        {
            auto& v = voices[(size_t) i];
            float score = (v.gate ? 1000.f : 0.f) + v.envAmp.v * 10.f + (float) (v.age / 100000.0);
            if (score < best) { best = score; pick = &v; }
        }
    }
    pick->start(note, vel, c, false);
    pick->age = counter++;
}

void Engine::noteOff(int note)
{
    if (!monoStack.empty() || voices[0].active)
    {
        auto it = std::find(monoStack.begin(), monoStack.end(), note);
        if (it != monoStack.end())
        {
            monoStack.erase(it);
            if (!monoStack.empty()) { voices[0].tgtNote = (float) monoStack.back(); voices[0].note = monoStack.back(); return; }
        }
    }
    for (auto& v : voices)
        if (v.active && v.gate && v.note == note)
        {
            v.gate = false;
            if (pedal) v.sustainHeld = true; else v.releaseEnvs();
        }
}

void Engine::handleMidi(const juce::MidiMessage& m, const Ctx& c)
{
    if (m.isNoteOn()) noteOn(m.getNoteNumber(), m.getFloatVelocity(), c);
    else if (m.isNoteOff()) { if (c.kit == 0) noteOff(m.getNoteNumber()); }
    else if (m.isPitchWheel()) bendNorm = (float) (m.getPitchWheelValue() - 8192) / 8192.f;
    else if (m.isControllerOfType(1)) modWheel = (float) m.getControllerValue() / 127.f;
    else if (m.isControllerOfType(64))
    {
        const bool d = m.getControllerValue() >= 64;
        if (!d && pedal) for (auto& v : voices) if (v.sustainHeld) { v.sustainHeld = false; if (!v.gate) v.releaseEnvs(); }
        pedal = d;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        monoStack.clear();
        for (auto& v : voices) if (v.active) { v.gate = false; v.sustainHeld = false; v.releaseEnvs(); }
    }
}

void Engine::runArp(const juce::MidiBuffer& in, juce::MidiBuffer& out, const Snap& s, int n, double bpm, double ppq, bool playing)
{
    auto& a = arp;
    if (s.arpOn < 0.5f)
    {
        if (a.wasOn)
        {
            if (a.curNote >= 0) out.addEvent(juce::MidiMessage::noteOff(1, a.curNote), 0);
            a.curNote = -1; a.held.clear(); a.played.clear(); a.physDown = 0; a.init = false;
        }
        a.wasOn = false; out.addEvents(in, 0, n, 0); return;
    }
    a.wasOn = true;
    static const double rateQ[4] = { 1.0, 0.5, 0.25, 0.125 };
    const double stepQ = rateQ[jlimit(0, 3, (int) s.arpRate)];
    const double qps = bpm / 60.0 / sr;
    if (playing && ppq >= 0.0)
    {
        if (!a.init || std::abs(a.posQ - ppq) > 0.02) { a.posQ = ppq; a.nextStep = (int) std::ceil(ppq / stepQ - 1e-6); }
        a.init = true;
    }
    else if (!a.init) { a.posQ = 0; a.nextStep = 0; a.init = true; }

    auto it = in.begin();
    for (int i = 0; i < n; ++i)
    {
        while (it != in.end() && (*it).samplePosition <= i)
        {
            auto m = (*it).getMessage(); ++it;
            if (m.isNoteOn())
            {
                if (a.physDown == 0 && s.arpHold > 0.5f) { a.held.clear(); a.played.clear(); }
                a.physDown++; a.curVel = m.getVelocity();
                if (std::find(a.held.begin(), a.held.end(), m.getNoteNumber()) == a.held.end())
                { a.held.push_back(m.getNoteNumber()); a.played.push_back(m.getNoteNumber()); }
                midiCount++;
            }
            else if (m.isNoteOff())
            {
                a.physDown = jmax(0, a.physDown - 1);
                if (s.arpHold < 0.5f)
                {
                    a.held.erase(std::remove(a.held.begin(), a.held.end(), m.getNoteNumber()), a.held.end());
                    a.played.erase(std::remove(a.played.begin(), a.played.end(), m.getNoteNumber()), a.played.end());
                }
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) { a.held.clear(); a.played.clear(); a.physDown = 0; out.addEvent(m, i); }
            else out.addEvent(m, i);
        }
        if (a.curNote >= 0 && a.posQ >= a.offAtQ) { out.addEvent(juce::MidiMessage::noteOff(1, a.curNote), i); a.curNote = -1; }
        const double T = a.nextStep * stepQ + ((a.nextStep & 1) ? s.arpSwing * stepQ : 0.0);
        if (a.posQ >= T)
        {
            const int k = a.nextStep++;
            if (!a.held.empty())
            {
                const int mode = (int) s.arpMode, octs = jlimit(1, 4, (int) s.arpOct);
                std::vector<int> base = (mode == 4) ? a.played : a.held;
                if (mode != 4) std::sort(base.begin(), base.end());
                std::vector<int> seq;
                for (int o = 0; o < octs; ++o) for (int nn : base) seq.push_back(nn + 12 * o);
                if (mode == 1) std::reverse(seq.begin(), seq.end());
                else if (mode == 2 && seq.size() > 2) for (int q = (int) seq.size() - 2; q >= 1; --q) seq.push_back(seq[(size_t) q]);
                const int idx = mode == 3 ? rng.nextInt((int) seq.size()) : (a.idx++ % (int) seq.size());
                const int step = k & 15;
                bool stepOn = (s.at(P_st0 + step) > 0.5f) != (((a.flips >> step) & 1u) != 0u);
                if (step == 15 && s.arpEvo > 0.001f && rng.nextFloat() < s.arpEvo) a.flips ^= (1u << rng.nextInt(16));
                if (stepOn)
                {
                    if (a.curNote >= 0) out.addEvent(juce::MidiMessage::noteOff(1, a.curNote), i);
                    const int note = jlimit(0, 127, seq[(size_t) idx]);
                    out.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) jlimit(1, 127, a.curVel)), i);
                    a.curNote = note; a.offAtQ = T + s.arpGate * stepQ;
                }
            }
        }
        a.posQ += qps;
    }
}

void Engine::renderVoices(const Ctx& c, float* L, float* R, int n)
{
    for (auto& v : voices) if (v.active) v.render(c, L, R, n);
}

void Engine::process(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, const Snap& s, double bpm, double ppq, bool playing)
{
    const int n = buf.getNumSamples();
    buf.clear();
    if (bpm < 20.0) bpm = 120.0;
    Ctx c; c.s = s; c.sr = sr; c.kit = (int) s.kit;
    c.tuneSemis = s.tune * 0.01f;
    c.cutOct = (s.m1 - 0.5f) * 6.f;
    c.drive = jlimit(0.f, 36.f, s.fDrv + s.m4 * 18.f);
    c.tbl = jlimit(0.f, 1.f, s.bTbl + s.m2);
    c.fm = jlimit(0.f, 1.f, s.bFM + s.m5 * 0.6f);
    c.lvlC = jlimit(0.f, 1.f, s.lCLvl + s.m6 * 0.5f);
    c.det = jlimit(0.f, 1.f, s.bDet + s.m7 * 0.8f);
    c.driftCents = s.m7 * 25.f;
    c.envAmt = jlimit(-100.f, 100.f, s.fEnv + s.m8 * 60.f);
    c.vib = modWheel * 0.5f;
    Snap& sm = c.s; // macro-adjusted effect values live in the snapshot copy
    sm.bcMix = jlimit(0.f, 1.f, s.bcMix + s.m4 * 0.5f);
    sm.rmMix = jlimit(0.f, 1.f, s.rmMix + s.m5 * 0.5f);
    sm.rWet = jlimit(0.f, 1.f, s.rWet + s.m3 * 0.5f);
    sm.dMix = jlimit(0.f, 1.f, s.dMix + s.m3 * 0.35f);

    juce::MidiBuffer evs; runArp(midi, evs, s, n, bpm, ppq, playing);

    float* L = buf.getWritePointer(0); float* R = buf.getWritePointer(1);
    int pos = 0; auto it = evs.begin();
    float lfoAcc = 0; int lfoN = 0;
    while (pos < n)
    {
        c.bend = bendNorm * s.bend; c.vib = modWheel * 0.5f;
        while (it != evs.end() && (*it).samplePosition <= pos) { handleMidi((*it).getMessage(), c); ++it; }
        c.bend = bendNorm * s.bend;
        int next = it != evs.end() ? jmin((*it).samplePosition, n) : n;
        int len = jmin(next - pos, 32); if (len <= 0) len = jmin(32, n - pos);

        double hzLfo = s.lfoSync > 0.5f ? bpm / 60.0 / 4.0 * (double) (1 << jlimit(0, 4, (int) s.lfoDiv)) : (double) s.lfoRate;
        double prev = lfoPh; lfoPh += hzLfo * len / sr; bool wrapped = false; if (lfoPh >= 1.0) { lfoPh -= std::floor(lfoPh); wrapped = true; }
        (void) prev;
        if (wrapped) lfoSH = rng.nextFloat() * 2.f - 1.f;
        const float ph = (float) lfoPh;
        switch ((int) s.lfoShape)
        {
            case 0: c.lfo = std::sin(kTwoPi * ph); break;
            case 1: c.lfo = 4.f * std::abs(ph - 0.5f) - 1.f; break;
            case 2: c.lfo = 2.f * ph - 1.f; break;
            case 3: c.lfo = ph < 0.5f ? 1.f : -1.f; break;
            default: c.lfo = lfoSH; break;
        }
        c.tbl = jlimit(0.f, 1.f, s.bTbl + s.m2 + 0.35f * c.lfo * s.fLfo);
        lfoAcc += std::abs(c.lfo); lfoN++;
        renderVoices(c, L + pos, R + pos, len);
        pos += len;
    }
    fx(c, L, R, n, bpm);

    int act = 0; float envSum = 0;
    for (auto& v : voices) if (v.active) { ++act; envSum += v.envF.v * v.envAmp.v; }
    nVoices.store(act);
    const float target = act ? jmin(1.f, 0.35f + envSum / (float) act * 0.65f + s.fLfo * lfoAcc / (float) jmax(1, lfoN) * 0.3f) : 0.f;
    activity.store(activity.load() * 0.9f + target * 0.1f);
}

void Engine::fx(const Ctx& c, float* L, float* R, int n, double bpm)
{
    const Snap& s = c.s;
    // ---- formant (vowel filter)
    if (s.ftMix > 0.001f)
    {
        static const float V[5][3] = { {800,1150,2900},{400,1700,2600},{270,2140,2950},{450,800,2830},{325,700,2700} };
        const float p = s.ftShift * 4.f; const int i0 = jmin(3, (int) p); const float fr = p - (float) i0;
        for (int j = 0; j < 3; ++j) fmtC[j].set(V[i0][j] * (1.f - fr) + V[i0 + 1][j] * fr, 0.9f, sr);
        static const float w[3] = { 1.f, 0.7f, 0.4f };
        for (int i = 0; i < n; ++i)
        {
            float* ch[2] = { L, R };
            for (int k = 0; k < 2; ++k)
            {
                float x = ch[k][i], sum = 0, bp, hp;
                for (int j = 0; j < 3; ++j) { fmt[k][j].run(x, fmtC[j].a1, fmtC[j].a2, fmtC[j].a3, fmtC[j].k, bp, hp); sum += bp * fmtC[j].k * w[j]; }
                ch[k][i] = x * (1.f - s.ftMix) + sum * s.ftMix * 1.6f;
            }
        }
    }
    // ---- ring mod
    if (s.rmMix > 0.001f)
    {
        const double inc = s.rmFreq / sr;
        for (int i = 0; i < n; ++i)
        {
            const float m = std::sin((float) (rmPh * 6.283185307179586)); rmPh += inc; if (rmPh >= 1.0) rmPh -= 1.0;
            L[i] = L[i] * (1.f - s.rmMix) + L[i] * m * s.rmMix; R[i] = R[i] * (1.f - s.rmMix) + R[i] * m * s.rmMix;
        }
    }
    // ---- bitcrush
    if (s.bcMix > 0.001f)
    {
        const int div = 1 + (int) (s.bcRate * 40.f); const float q = std::exp2(s.bcBits - 1.f);
        for (int i = 0; i < n; ++i)
        {
            if (bcCount++ % div == 0) { bcHoldL = std::round(L[i] * q) / q; bcHoldR = std::round(R[i] * q) / q; }
            L[i] = L[i] * (1.f - s.bcMix) + bcHoldL * s.bcMix; R[i] = R[i] * (1.f - s.bcMix) + bcHoldR * s.bcMix;
        }
    }
    // ---- granular smear
    if (s.smMix > 0.001f)
    {
        const int mask = (int) smBuf[0].size() - 1;
        for (int i = 0; i < n; ++i)
        {
            smBuf[0][(size_t) smW] = L[i]; smBuf[1][(size_t) smW] = R[i];
            if (rng.nextFloat() < (6.f + 14.f * s.smSize) / (float) sr * 3.f)
                for (auto& g : sg) if (!g.on)
                {
                    static const float rates[6] = { 1.f, 1.f, 0.5f, 2.f, 1.5f, 0.75f };
                    g.on = true; g.age = 0; g.rate = rates[rng.nextInt(6)]; g.len = (int) ((0.04f + 0.18f * s.smSize) * (float) sr);
                    g.pos = (float) (smW - (int) ((float) g.len * g.rate) - 64 - (int) (rng.nextFloat() * 0.3f * s.smSize * (float) sr));
                    g.pan = rng.nextFloat() * 2.f - 1.f; break;
                }
            float wl = 0, wr = 0;
            for (auto& g : sg) if (g.on)
            {
                const float w = 0.5f - 0.5f * std::cos(kTwoPi * (float) g.age / (float) g.len);
                float fp = g.pos; int ip = (int) std::floor(fp); float fr = fp - (float) ip;
                int i0 = ip & mask, i1 = (ip + 1) & mask;
                float ml = smBuf[0][(size_t) i0] + (smBuf[0][(size_t) i1] - smBuf[0][(size_t) i0]) * fr;
                float mr = smBuf[1][(size_t) i0] + (smBuf[1][(size_t) i1] - smBuf[1][(size_t) i0]) * fr;
                wl += ml * w * (1.f - 0.5f * g.pan * 0.5f); wr += mr * w * (1.f + 0.5f * g.pan * 0.5f);
                g.pos += g.rate; if (++g.age >= g.len) g.on = false;
            }
            smW = (smW + 1) & mask;
            L[i] = L[i] * (1.f - s.smMix * 0.4f) + wl * s.smMix * 0.9f; R[i] = R[i] * (1.f - s.smMix * 0.4f) + wr * s.smMix * 0.9f;
        }
    }
    // ---- pan / width / gain
    {
        const float gain = juce::Decibels::decibelsToGain(s.aGain);
        const float ang = (s.aPan + 1.f) * kPi * 0.25f; const float gl = std::cos(ang) * 1.4142f * gain, gr = std::sin(ang) * 1.4142f * gain;
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f, sd = (L[i] - R[i]) * 0.5f * s.aWid;
            L[i] = (m + sd) * gl; R[i] = (m - sd) * gr;
        }
    }
    // ---- master filter
    if (s.mfOn > 0.5f && !((int) s.mfType == 0 && s.mfFreq > 19500.f))
    {
        SVFCoef mc; mc.set(s.mfFreq, s.mfQ * 0.92f, sr); const int type = (int) s.mfType;
        for (int i = 0; i < n; ++i)
        {
            float bp, hp, lp;
            lp = mf[0][0].run(L[i], mc.a1, mc.a2, mc.a3, mc.k, bp, hp); L[i] = type == 0 ? lp : type == 1 ? bp * mc.k : hp;
            lp = mf[1][0].run(R[i], mc.a1, mc.a2, mc.a3, mc.k, bp, hp); R[i] = type == 0 ? lp : type == 1 ? bp * mc.k : hp;
        }
    }
    // ---- delay
    if (s.dOn > 0.5f)
    {
        static const double beats[3] = { 1.0, 0.5, 0.25 };
        const double ms = s.dSync > 0.5f ? 60000.0 / bpm * beats[jlimit(0, 2, (int) s.dDiv)] : (double) s.dTime;
        const int size = (int) dl[0].size(), mask = size - 1;
        const float target = (float) jlimit(2.0, (double) size - 8.0, ms * 0.001 * sr);
        const float mix = s.dMix, fb = s.dFb;
        for (int i = 0; i < n; ++i)
        {
            dTimeSm += (target - dTimeSm) * 0.0015f;
            float out[2];
            for (int k = 0; k < 2; ++k)
            {
                const float rp = (float) dW - dTimeSm * (k == 0 ? 1.f : 1.013f);
                int ip = (int) std::floor(rp); float fr = rp - (float) ip;
                out[k] = dl[(size_t) k][(size_t) (ip & mask)] * (1.f - fr) + dl[(size_t) k][(size_t) ((ip + 1) & mask)] * fr;
            }
            dLp[0] += 0.4f * (out[0] - dLp[0]); dLp[1] += 0.4f * (out[1] - dLp[1]);
            dl[0][(size_t) dW] = soft(L[i] + fb * dLp[1]); dl[1][(size_t) dW] = soft(R[i] + fb * dLp[0]);
            dW = (dW + 1) & mask;
            L[i] += out[0] * mix; R[i] += out[1] * mix;
        }
    }
    // ---- reverb
    if (s.rOn > 0.5f && s.rWet > 0.001f)
    {
        juce::Reverb::Parameters p; const int t = (int) s.rType;
        p.roomSize = t == 0 ? 0.6f + 0.4f * s.rSize : t == 1 ? 0.45f + 0.35f * s.rSize : 0.15f + 0.4f * s.rSize;
        p.damping = t == 0 ? 0.35f : t == 1 ? 0.12f : 0.6f; p.width = 1.f; p.freezeMode = 0.f;
        p.wetLevel = s.rWet * 0.5f; p.dryLevel = 1.f - s.rWet * 0.35f;
        reverb.setParameters(p); reverb.processStereo(L, R, n);
    }
    // ---- output stage
    const float vol = juce::Decibels::decibelsToGain(s.master + s.oVol);
    const bool lim = s.oLim > 0.5f, mono = (int) s.oMode == 1;
    float pkL = 0, pkR = 0;
    for (int i = 0; i < n; ++i)
    {
        float l = L[i] * vol, r = R[i] * vol;
        if (!std::isfinite(l)) l = 0; if (!std::isfinite(r)) r = 0;
        if (mono) l = r = (l + r) * 0.5f;
        if (lim)
        {
            const float pk = jmax(std::abs(l), std::abs(r)); const float tg = pk > 0.95f ? 0.95f / pk : 1.f;
            if (tg < limGain) limGain = tg; else limGain += (tg - limGain) * 0.0004f;
            l = jlimit(-1.f, 1.f, l * limGain); r = jlimit(-1.f, 1.f, r * limGain);
        }
        L[i] = l; R[i] = r; pkL = jmax(pkL, std::abs(l)); pkR = jmax(pkR, std::abs(r));
    }
    peakL.store(jmax(pkL, peakL.load() * 0.9f)); peakR.store(jmax(pkR, peakR.load() * 0.9f));
}
}
