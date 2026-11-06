#pragma once
#include <JuceHeader.h>

// Every parameter ID, in host order. The ID string is the identifier itself.
#define ZIL_IDS(X) \
 X(master) \
 X(fCut) X(fRes) X(fDrv) X(fKey) X(fEnv) X(fLfo) X(lfoRate) X(fMix) X(fA) X(fB) \
 X(fAtk) X(fDec) X(fSus) X(fRel) X(lfoShape) X(lfoSync) X(lfoDiv) \
 X(aAtk) X(aDec) X(aSus) X(aRel) X(aVel) X(aPan) X(aWid) X(aGain) \
 X(lAOn) X(lBOn) X(lCOn) \
 X(lALvl) X(lATune) X(lACut) X(lARes) X(lAAtk) X(lADec) X(lASus) X(lARel) X(aInst) \
 X(lBLvl) X(lBTune) X(lBCut) X(lBRes) X(lBAtk) X(lBDec) X(lBSus) X(lBRel) X(bTbl) X(bFM) X(bRat) X(bDet) \
 X(lCLvl) X(lCTune) X(lCCut) X(lCRes) X(lCAtk) X(lCDec) X(lCSus) X(lCRel) X(cCol) X(cGrn) X(cDns) \
 X(ftMix) X(ftShift) X(rmMix) X(rmFreq) X(bcMix) X(bcBits) X(bcRate) X(smMix) X(smSize) \
 X(mfOn) X(mfType) X(mfFreq) X(mfQ) \
 X(dOn) X(dSync) X(dDiv) X(dTime) X(dFb) X(dMix) \
 X(rOn) X(rType) X(rSize) X(rWet) \
 X(oLim) X(oMode) X(oVol) \
 X(m1) X(m2) X(m3) X(m4) X(m5) X(m6) X(m7) X(m8) \
 X(arpOn) X(arpMode) X(arpRate) X(arpOct) X(arpGate) X(arpSwing) X(arpHold) X(arpEvo) \
 X(st0) X(st1) X(st2) X(st3) X(st4) X(st5) X(st6) X(st7) X(st8) X(st9) X(st10) X(st11) X(st12) X(st13) X(st14) X(st15) \
 X(voices) X(glide) X(bend) X(tune) X(tilt) X(kit)

namespace zil
{
enum ParamIndex {
#define X(n) P_##n,
    ZIL_IDS(X)
#undef X
    P_COUNT
};

// Plain snapshot of every parameter's current (real-unit) value.
struct Snap {
#define X(n) float n = 0.f;
    ZIL_IDS(X)
#undef X
    float& at(int i) { return (&master)[i]; }
    float at(int i) const { return (&master)[i]; }
};

enum class Fmt { Raw, Hz, Db, Sec, Pct, Semi, Pan, Cents, Signed, Int, HzF, Ms };

struct PDef {
    juce::String id, name;
    int type = 0;                 // 0 float, 1 bool, 2 choice, 3 int
    float mn = 0, mx = 1, def = 0, mid = 0;
    Fmt fmt = Fmt::Raw;
    juce::StringArray choices;
};

const std::vector<PDef>& defs();                       // indexed by ParamIndex
int indexOf(const juce::String& id);                   // -1 if unknown
juce::String formatValue(Fmt, float v);
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
