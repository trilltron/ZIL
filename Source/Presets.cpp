#include "Presets.h"
#include <map>

namespace zil
{
namespace
{
using KV = std::vector<std::pair<std::string, float>>;
KV operator+(KV a, const KV& b) { a.insert(a.end(), b.begin(), b.end()); return a; }

KV L(char l, float lvl, float tune, float cut, float a, float d, float s, float r, float res = 0.1f)
{
    std::string p = std::string("l") + l;
    return { { p + "Lvl", lvl }, { p + "Tune", tune }, { p + "Cut", cut }, { p + "Res", res },
             { p + "Atk", a }, { p + "Dec", d }, { p + "Sus", s }, { p + "Rel", r } };
}
KV AMP(float a, float d, float sDb, float r) { return { { "aAtk", a }, { "aDec", d }, { "aSus", sDb }, { "aRel", r } }; }
KV FLT(float cut, float res, float env, float a, float d, float s, float r)
{ return { { "fCut", cut }, { "fRes", res }, { "fEnv", env }, { "fAtk", a }, { "fDec", d }, { "fSus", s }, { "fRel", r } }; }
KV REV(float wet, int type, float size) { return { { "rOn", wet > 0 ? 1.f : 0.f }, { "rWet", wet }, { "rType", (float) type }, { "rSize", size } }; }
KV DLY(float mix, float fb, int div) { return { { "dOn", 1 }, { "dMix", mix }, { "dFb", fb }, { "dSync", 1 }, { "dDiv", (float) div } }; }
KV T(KV k) { return k; }

KV base()
{
    KV k = L('A', 0, 0, 1, .003f, 1.f, 1, .3f) + L('B', 0, 0, 1, .003f, 1.f, 1, .3f) + L('C', 0, 0, 1, .003f, 1.f, 1, .3f);
    k = k + FLT(18000, .1f, 0, .005f, .6f, .3f, .5f) + AMP(.003f, .5f, 0, .35f) + REV(.15f, 0, .6f);
    k = k + KV{ { "lAOn", 1 }, { "lBOn", 1 }, { "lCOn", 1 }, { "fKey", .3f }, { "fDrv", 0 }, { "fLfo", 0 }, { "fMix", 1 }, { "fA", 1 }, { "fB", 1 },
                { "aVel", .65f }, { "aPan", 0 }, { "aWid", 1.2f }, { "aGain", 0 }, { "bTbl", 0 }, { "bFM", 0 }, { "bRat", 2 }, { "bDet", .15f },
                { "cCol", .5f }, { "cGrn", 0 }, { "cDns", .5f }, { "ftMix", 0 }, { "rmMix", 0 }, { "bcMix", 0 }, { "smMix", 0 }, { "mfOn", 0 }, { "mfFreq", 20000 },
                { "dOn", 0 }, { "oLim", 1 }, { "m1", .5f }, { "m2", 0 }, { "m3", 0 }, { "m4", 0 }, { "m5", 0 }, { "m6", 0 }, { "m7", 0 }, { "m8", 0 },
                { "arpOn", 0 }, { "arpMode", 0 }, { "arpRate", 2 }, { "arpOct", 2 }, { "arpGate", .6f }, { "arpSwing", 0 }, { "arpHold", 0 }, { "arpEvo", 0 },
                { "voices", 8 }, { "glide", 0 }, { "bend", 2 }, { "kit", 0 }, { "lfoRate", 4 }, { "lfoShape", 0 }, { "lfoSync", 0 }, { "aInst", 0 } };
    for (int i = 0; i < 16; ++i) k.push_back({ "st" + std::to_string(i), 1 });
    return k;
}

const std::map<std::string, KV>& archetypes()
{
    static const std::map<std::string, KV> m = [] {
        std::map<std::string, KV> a; const KV b = base();
        auto A = [&](const char* key, KV k) { a[key] = b + k; };
        A("grand",  L('A', .9f, 0, 1, .001f, 3.5f, 0, .35f) + L('C', .07f, 0, .55f, .001f, .03f, 0, .05f) + KV{ { "aInst", 0 }, { "cCol", .8f } } + AMP(.002f, .5f, 0, .4f) + FLT(7000, .12f, 25, .001f, 1.5f, 0, .5f) + REV(.25f, 1, .7f));
        A("felt",   L('A', .9f, 0, .8f, .002f, 2.6f, 0, .4f) + KV{ { "aInst", 1 } } + AMP(.003f, .5f, 0, .5f) + FLT(3500, .1f, 20, .001f, 1.2f, 0, .5f) + REV(.3f, 0, .6f));
        A("ep",     L('A', .85f, 0, 1, .002f, 2.5f, 0, .4f) + L('B', .18f, 12, 1, .001f, .4f, 0, .2f) + KV{ { "aInst", 2 }, { "aWid", 1.5f } } + FLT(6000, .12f, 25, .001f, 1.2f, 0, .4f) + DLY(.12f, .3f, 1) + REV(.2f, 1, .5f));
        A("wurli",  L('A', .8f, 0, .9f, .002f, 1.8f, .15f, .3f) + KV{ { "aInst", 3 }, { "fDrv", 5 } } + FLT(4500, .2f, 25, .001f, .8f, .1f, .3f) + REV(.18f, 2, .5f));
        A("clav",   L('A', .8f, 0, 1, .001f, .6f, 0, .08f) + KV{ { "aInst", 4 }, { "aWid", .9f } } + FLT(5000, .25f, 60, .001f, .22f, 0, .1f) + AMP(.001f, .3f, 0, .08f) + REV(.1f, 2, .3f));
        A("organ",  L('A', .8f, 0, 1, .004f, 1, 1, .06f) + L('B', .15f, 12, 1, .004f, 1, 1, .06f) + KV{ { "aInst", 5 }, { "fDrv", 2 }, { "aWid", 1.4f } } + AMP(.004f, .2f, 0, .08f) + FLT(12000, .1f, 0, .005f, .5f, 1, .3f) + REV(.22f, 2, .5f));
        A("pipe",   L('A', .75f, 0, .9f, .06f, 1, 1, .5f) + L('B', .3f, 12, .9f, .08f, 1, 1, .5f) + KV{ { "aInst", 6 }, { "aWid", 1.6f } } + AMP(.05f, .3f, 0, .6f) + FLT(9000, .1f, 0, .005f, .5f, 1, .3f) + REV(.45f, 0, .95f));
        A("subsine",L('A', .8f, 0, .5f, .004f, 1, 1, .12f) + KV{ { "aInst", 13 }, { "voices", 1 }, { "glide", .05f }, { "aWid", .3f } } + AMP(.004f, .2f, 0, .12f) + FLT(600, .1f, 0, .005f, .5f, 1, .3f) + REV(0, 0, .5f));
        A("subtri", L('B', .7f, 0, .6f, .004f, 1, 1, .12f) + KV{ { "bTbl", .25f }, { "bDet", 0 }, { "voices", 1 }, { "glide", .03f }, { "aWid", .3f } } + AMP(.004f, .2f, 0, .12f) + FLT(800, .1f, 0, .005f, .5f, 1, .3f) + REV(0, 0, .5f));
        A("analogbass", L('B', .8f, 0, 1, .003f, 1, 1, .12f) + L('A', .35f, -12, .5f, .003f, 1, 1, .12f) + KV{ { "bTbl", .5f }, { "bDet", .25f }, { "aInst", 13 }, { "fDrv", 6 }, { "voices", 1 }, { "glide", .04f }, { "aWid", .7f } } + AMP(.003f, .2f, 0, .12f) + FLT(380, .38f, 70, .002f, .28f, .2f, .15f) + REV(.06f, 2, .3f));
        A("acid",   L('B', .8f, 0, 1, .001f, 1, 1, .08f) + KV{ { "bTbl", .5f }, { "bDet", 0 }, { "fDrv", 9 }, { "voices", 1 }, { "glide", .06f }, { "aWid", .5f } } + AMP(.001f, .15f, -6, .08f) + FLT(300, .75f, 85, .001f, .18f, 0, .1f) + DLY(.2f, .4f, 2) + REV(.08f, 2, .3f));
        A("slap",   L('A', .8f, 0, 1, .001f, .35f, .2f, .1f) + L('B', .3f, 0, .7f, .001f, .2f, 0, .1f) + KV{ { "aInst", 10 }, { "bTbl", .5f }, { "voices", 1 }, { "aWid", .5f } } + AMP(.001f, .3f, -3, .12f) + FLT(1800, .3f, 70, .001f, .14f, 0, .1f) + REV(.05f, 2, .3f));
        A("fretless", L('A', .7f, 0, .7f, .01f, 1.5f, .5f, .3f) + L('B', .35f, 0, .6f, .01f, 1, 1, .3f) + KV{ { "aInst", 10 }, { "bTbl", .25f }, { "voices", 1 }, { "glide", .03f }, { "aWid", .5f } } + AMP(.005f, .3f, 0, .25f) + FLT(1200, .15f, 40, .005f, .5f, .5f, .3f) + REV(.1f, 2, .4f));
        A("reese",  L('B', .8f, 0, 1, .003f, 1, 1, .2f) + L('A', .3f, -12, .5f, .003f, 1, 1, .2f) + KV{ { "bTbl", .5f }, { "bDet", .9f }, { "aInst", 13 }, { "fLfo", .3f }, { "lfoRate", .3f }, { "voices", 4 }, { "aWid", 1.5f } } + AMP(.003f, .2f, 0, .2f) + FLT(520, .25f, 10, .005f, .5f, 1, .3f) + REV(.05f, 2, .3f));
        A("pluckbass", L('B', .8f, 0, 1, .001f, .25f, 0, .1f) + L('A', .4f, 0, .5f, .001f, .35f, 0, .1f) + KV{ { "bTbl", .5f }, { "bDet", .1f }, { "aInst", 13 }, { "voices", 1 }, { "aWid", .6f } } + AMP(.001f, .3f, 0, .1f) + FLT(900, .25f, 80, .001f, .2f, 0, .1f) + REV(.06f, 2, .3f));
        A("analoglead", L('B', .7f, 0, 1, .003f, 1, 1, .25f) + KV{ { "bTbl", .5f }, { "bDet", .35f }, { "voices", 4 }, { "glide", .05f } } + AMP(.003f, .3f, 0, .25f) + FLT(3500, .2f, 35, .003f, .4f, .5f, .3f) + DLY(.2f, .35f, 1) + REV(.2f, 1, .5f));
        A("supersaw", L('B', .6f, 0, 1, .004f, 1, 1, .35f) + L('A', .28f, .12f, .8f, .004f, 1, 1, .35f) + KV{ { "bTbl", .5f }, { "bDet", .95f }, { "aInst", 7 }, { "aWid", 1.9f } } + AMP(.004f, .3f, 0, .35f) + FLT(7000, .1f, 15, .005f, .5f, .6f, .3f) + DLY(.15f, .3f, 1) + REV(.3f, 0, .7f));
        A("pluckLead", L('B', .8f, 0, 1, .001f, .3f, 0, .2f) + KV{ { "bTbl", .75f }, { "bFM", .3f }, { "bRat", 2.0f }, { "bDet", .3f }, { "aWid", 1.5f } } + AMP(.001f, .35f, 0, .25f) + FLT(6000, .2f, 40, .001f, .25f, .1f, .2f) + DLY(.25f, .4f, 1) + REV(.2f, 1, .5f));
        A("analogPluck", L('B', .8f, 0, 1, .001f, .35f, 0, .2f) + KV{ { "bTbl", .5f }, { "bDet", .4f }, { "aWid", 1.5f } } + AMP(.001f, .35f, 0, .25f) + FLT(2500, .3f, 70, .001f, .3f, .05f, .2f) + DLY(.2f, .35f, 1) + REV(.2f, 1, .5f));
        A("sync",   L('B', .7f, 0, 1, .003f, 1, 1, .2f) + KV{ { "bTbl", .5f }, { "bFM", .55f }, { "bRat", 3.01f }, { "bDet", .1f }, { "voices", 1 }, { "glide", .05f } } + AMP(.003f, .3f, 0, .2f) + FLT(4500, .25f, 45, .003f, .5f, .6f, .3f) + DLY(.18f, .3f, 1) + REV(.15f, 2, .4f));
        A("fmbell", L('B', .7f, 0, 1, .001f, 2.5f, 0, 1.5f) + L('A', .3f, 12, 1, .001f, 1.5f, 0, 1.f) + KV{ { "bTbl", 0 }, { "bFM", .6f }, { "bRat", 3.5f }, { "aInst", 12 }, { "bDet", .1f }, { "aWid", 1.5f } } + AMP(.001f, .5f, 0, 1.2f) + FLT(9000, .1f, 20, .001f, 1.5f, 0, .5f) + REV(.35f, 0, .8f));
        A("warmpad",L('A', .55f, 0, .8f, .5f, 1, 1, 1.5f) + L('B', .35f, -12, .8f, .5f, 1, 1, 1.5f) + KV{ { "aInst", 7 }, { "bTbl", .5f }, { "bDet", .6f }, { "fLfo", .15f }, { "lfoRate", .3f }, { "aWid", 1.8f } } + AMP(.6f, .5f, 0, 2.2f) + FLT(2200, .15f, 20, .6f, 1.5f, .7f, 1.5f) + REV(.5f, 0, .85f));
        A("chorpad",L('B', .6f, 0, .9f, .4f, 1, 1, 1.2f) + L('A', .3f, 0, .7f, .4f, 1, 1, 1.2f) + KV{ { "bTbl", .5f }, { "bDet", .85f }, { "aInst", 7 }, { "aWid", 2.0f }, { "fLfo", .1f }, { "lfoRate", .5f } } + AMP(.45f, .5f, 0, 1.8f) + FLT(3000, .12f, 15, .5f, 1.f, .7f, 1.f) + DLY(.2f, .35f, 1) + REV(.4f, 1, .8f));
        A("wtpad",  L('B', .7f, 0, .9f, .8f, 1, 1, 2.f) + L('A', .2f, 12, .7f, 1.2f, 1, 1, 2.f) + KV{ { "bTbl", .4f }, { "bDet", .5f }, { "aInst", 15 }, { "fLfo", .5f }, { "lfoRate", .12f }, { "aWid", 1.8f } } + AMP(.9f, .5f, 0, 2.5f) + FLT(3500, .2f, 15, .8f, 1.f, .8f, 1.5f) + REV(.45f, 0, .9f));
        A("drone",  L('B', .6f, -12, .6f, 3, 1, 1, 5) + L('A', .4f, -24, .5f, 4, 1, 1, 5) + L('C', .25f, 0, .7f, 4, 1, 1, 5) + KV{ { "bTbl", .5f }, { "bDet", .8f }, { "aInst", 15 }, { "cGrn", .4f }, { "cCol", .3f }, { "fLfo", .5f }, { "lfoRate", .08f }, { "aWid", 2.f }, { "voices", 4 } } + AMP(3, .5f, 0, 5.5f) + FLT(900, .35f, 10, 2, 3, .8f, 3) + REV(.7f, 0, .95f));
        A("solostr",L('A', .85f, 0, .9f, .12f, 1, 1, .3f) + KV{ { "aInst", 7 }, { "voices", 1 }, { "glide", .03f }, { "aWid", 1.0f } } + AMP(.1f, .3f, 0, .3f) + FLT(4500, .1f, 30, .1f, 1, .8f, .3f) + REV(.3f, 0, .7f));
        A("cello",  L('A', .85f, -12, .65f, .15f, 1, 1, .35f) + KV{ { "aInst", 7 }, { "voices", 1 }, { "glide", .03f }, { "aWid", 1.0f } } + AMP(.12f, .3f, 0, .35f) + FLT(2800, .12f, 30, .12f, 1, .8f, .3f) + REV(.3f, 0, .7f));
        A("ensemble", L('A', .75f, 0, .85f, .25f, 1, 1, .8f) + L('B', .25f, 0, .7f, .3f, 1, 1, .8f) + KV{ { "aInst", 7 }, { "bTbl", .5f }, { "bDet", .6f }, { "aWid", 1.8f } } + AMP(.22f, .3f, 0, .9f) + FLT(5000, .1f, 25, .25f, 1, .8f, .5f) + REV(.45f, 0, .85f));
        A("stacc",  L('A', .8f, 0, .85f, .005f, .25f, 0, .1f) + L('B', .25f, 0, .7f, .005f, .25f, 0, .1f) + KV{ { "aInst", 7 }, { "bTbl", .5f }, { "bDet", .5f }, { "aWid", 1.7f } } + AMP(.004f, .3f, 0, .15f) + FLT(5500, .1f, 30, .002f, .2f, 0, .2f) + REV(.3f, 0, .7f));
        A("pizz",   L('A', .9f, 0, 1, .001f, .4f, 0, .1f) + L('C', .05f, 0, .6f, .001f, .02f, 0, .05f) + KV{ { "aInst", 10 }, { "aWid", 1.4f } } + AMP(.001f, .3f, 0, .15f) + FLT(5000, .1f, 25, .001f, .3f, 0, .2f) + REV(.3f, 0, .7f));
        A("brass",  L('A', .8f, 0, .8f, .06f, 1, 1, .15f) + L('B', .3f, 0, .7f, .06f, 1, 1, .15f) + KV{ { "aInst", 8 }, { "bTbl", .5f }, { "bDet", .2f }, { "aWid", 1.4f } } + AMP(.05f, .3f, 0, .2f) + FLT(2500, .2f, 55, .07f, .4f, .7f, .2f) + REV(.3f, 0, .7f));
        A("flute",  L('B', .7f, 0, 1, .08f, 1, 1, .2f) + L('C', .1f, 0, .8f, .06f, 1, 1, .2f) + KV{ { "bTbl", 0 }, { "bDet", .05f }, { "cCol", .7f }, { "voices", 1 }, { "glide", .02f } } + AMP(.06f, .3f, 0, .25f) + FLT(6000, .1f, 20, .06f, .5f, .8f, .3f) + REV(.3f, 0, .7f));
        A("oboe",   L('A', .8f, 0, .8f, .05f, 1, 1, .2f) + KV{ { "aInst", 9 }, { "voices", 1 }, { "glide", .02f } } + AMP(.04f, .3f, 0, .22f) + FLT(2500, .4f, 30, .05f, .4f, .8f, .3f) + REV(.28f, 0, .65f));
        A("clarinet", L('A', .8f, 0, .6f, .04f, 1, 1, .2f) + KV{ { "aInst", 9 }, { "voices", 1 }, { "glide", .02f } } + AMP(.04f, .3f, 0, .22f) + FLT(1800, .15f, 25, .05f, .4f, .8f, .3f) + REV(.28f, 0, .65f));
        A("strat",  L('A', .8f, 0, 1, .001f, 1.5f, .1f, .25f) + L('C', .05f, 0, .6f, .001f, .02f, 0, .05f) + KV{ { "aInst", 10 }, { "aWid", 1.2f } } + AMP(.001f, .3f, 0, .3f) + FLT(5500, .2f, 30, .001f, .5f, .1f, .3f) + DLY(.15f, .3f, 1) + REV(.2f, 2, .5f));
        A("jazzgtr",L('A', .85f, 0, .6f, .002f, 1.8f, .15f, .3f) + KV{ { "aInst", 10 }, { "aWid", 1.0f } } + AMP(.002f, .3f, 0, .3f) + FLT(2200, .15f, 25, .001f, .6f, .1f, .3f) + REV(.2f, 2, .5f));
        A("crunch", L('A', .8f, 0, 1, .001f, 1.5f, .3f, .2f) + KV{ { "aInst", 10 }, { "fDrv", 18 }, { "aWid", 1.2f } } + AMP(.001f, .3f, 0, .2f) + FLT(4200, .25f, 20, .001f, .5f, .3f, .3f) + REV(.15f, 2, .4f));
        A("distlead", L('A', .8f, 0, 1, .002f, 2, .6f, .3f) + L('B', .2f, 0, .8f, .002f, 1, 1, .3f) + KV{ { "aInst", 10 }, { "bTbl", .5f }, { "fDrv", 24 }, { "voices", 1 }, { "glide", .06f } } + AMP(.002f, .3f, 0, .3f) + FLT(3800, .3f, 20, .002f, .5f, .5f, .3f) + DLY(.2f, .35f, 1) + REV(.2f, 1, .5f));
        A("acoustic", L('A', .85f, 0, 1, .001f, 1.6f, .05f, .3f) + L('C', .04f, 0, .7f, .001f, .015f, 0, .05f) + KV{ { "aInst", 10 }, { "aWid", 1.4f } } + AMP(.001f, .3f, 0, .3f) + FLT(7000, .15f, 25, .001f, .6f, .1f, .3f) + REV(.22f, 2, .5f));
        A("nylon",  L('A', .85f, 0, .6f, .002f, 1.4f, 0, .3f) + KV{ { "aInst", 10 }, { "aWid", 1.2f } } + AMP(.002f, .3f, 0, .3f) + FLT(3000, .12f, 25, .001f, .5f, 0, .3f) + REV(.25f, 0, .6f));
        A("harp",   L('A', .85f, 0, .9f, .001f, 2.2f, 0, .5f) + KV{ { "aInst", 1 }, { "aWid", 1.6f } } + AMP(.001f, .3f, 0, .6f) + FLT(7000, .1f, 25, .001f, .8f, 0, .5f) + REV(.4f, 0, .8f));
        A("marimba",L('A', .9f, 0, 1, .001f, .9f, 0, .15f) + KV{ { "aInst", 11 }, { "aWid", 1.3f } } + AMP(.001f, .3f, 0, .2f) + FLT(9000, .1f, 20, .001f, .5f, 0, .3f) + REV(.25f, 2, .5f));
        A("kalimba",L('A', .9f, 0, 1, .001f, 1.1f, 0, .3f) + KV{ { "aInst", 12 }, { "aWid", 1.4f } } + AMP(.001f, .3f, 0, .4f) + FLT(9000, .1f, 20, .001f, .5f, 0, .3f) + REV(.3f, 1, .6f));
        A("kit",    KV{ { "voices", 16 }, { "aGain", 3 }, { "aWid", 1.2f } } + AMP(.001f, .5f, 0, .1f) + REV(.12f, 2, .35f));
        A("riser",  L('C', .7f, 0, 1, 6, 1, 1, 1.5f) + L('B', .3f, 0, 1, 6, 1, 1, 1.5f) + KV{ { "bTbl", .5f }, { "bDet", .9f }, { "cCol", .6f }, { "cGrn", .3f }, { "voices", 4 }, { "aWid", 1.9f } } + AMP(5, .5f, 0, 1.5f) + FLT(300, .4f, 100, 6, 1, 1, 1) + REV(.5f, 0, .9f));
        A("impact", L('A', .9f, -12, 1, .001f, 2.5f, 0, .5f) + L('C', .5f, 0, .6f, .001f, 1.5f, 0, .5f) + KV{ { "aInst", 13 }, { "cCol", .2f }, { "cGrn", .2f }, { "fDrv", 6 } } + AMP(.001f, .5f, 0, .8f) + FLT(2000, .2f, 40, .001f, 1.5f, 0, .5f) + REV(.7f, 0, 1.f));
        A("arppluck", L('B', .8f, 0, 1, .001f, .3f, 0, .2f) + KV{ { "bTbl", .5f }, { "bDet", .35f }, { "arpOn", 1 }, { "arpOct", 2 }, { "arpGate", .5f }, { "aWid", 1.5f } } + AMP(.001f, .3f, 0, .2f) + FLT(3500, .25f, 55, .001f, .25f, .05f, .2f) + DLY(.25f, .4f, 1) + REV(.2f, 1, .5f));
        A("gatearp", L('B', .7f, 0, 1, .003f, 1, 1, .1f) + L('A', .3f, 0, .8f, .003f, 1, 1, .1f) + KV{ { "bTbl", .5f }, { "bDet", .8f }, { "aInst", 7 }, { "arpOn", 1 }, { "arpOct", 3 }, { "arpGate", .35f }, { "arpSwing", .1f },
                                           { "st1", 0 }, { "st5", 0 }, { "st7", 0 }, { "st11", 0 }, { "st13", 0 }, { "aWid", 1.8f } } + AMP(.003f, .3f, 0, .12f) + FLT(5000, .2f, 30, .003f, .3f, .3f, .2f) + DLY(.2f, .4f, 1) + REV(.25f, 0, .7f));
        A("pulsegrid", L('B', .75f, 0, 1, .001f, .4f, 0, .3f) + KV{ { "bTbl", 1.f }, { "bFM", .35f }, { "bRat", 3.f }, { "bDet", .4f }, { "arpOn", 1 }, { "arpMode", 3 }, { "arpOct", 3 }, { "arpEvo", .6f }, { "arpGate", .45f },
                                          { "st3", 0 }, { "st6", 0 }, { "st10", 0 }, { "st12", 0 }, { "aWid", 1.8f } } + AMP(.001f, .4f, 0, .3f) + FLT(5000, .2f, 40, .001f, .3f, .05f, .2f) + DLY(.3f, .5f, 1) + REV(.3f, 0, .8f));
        return a;
    }();
    return m;
}

struct Entry { const char* cat; const char* sub; const char* name; bool sig; const char* arch; KV tweaks; };

std::vector<Entry> entries()
{
    auto E = [](const char* c, const char* s, const char* n, bool g, const char* a, KV t = {}) { return Entry { c, s, n, g, a, t }; };
    std::vector<Entry> e;
    // KEYS
    e.push_back(E("KEYS", "Grand/Acoustic Piano", "Concert Grand", 0, "grand"));
    e.push_back(E("KEYS", "Grand/Acoustic Piano", "Felt Piano", 0, "felt"));
    e.push_back(E("KEYS", "Grand/Acoustic Piano", "Quantum Keys", 1, "grand", { { "lBLvl", .25f }, { "lBTune", 7 }, { "bDet", .8f }, { "lCLvl", .2f }, { "cGrn", .5f }, { "smMix", .35f }, { "aWid", 1.7f } }));
    e.push_back(E("KEYS", "Electric Piano", "Rhodes Suitcase", 0, "ep"));
    e.push_back(E("KEYS", "Electric Piano", "Wurlitzer 200A", 0, "wurli"));
    e.push_back(E("KEYS", "Electric Piano", "Transmission EP", 1, "ep", { { "bcMix", .45f }, { "bcBits", 7 }, { "bcRate", .35f }, { "rmMix", .1f }, { "rmFreq", 220 } }));
    e.push_back(E("KEYS", "Electric Piano", "Wormhole Keys", 1, "ep", { { "bFM", .5f }, { "bRat", 1.5f }, { "lBLvl", .3f }, { "smMix", .3f }, { "m5", .2f } }));
    e.push_back(E("KEYS", "Clavinet", "Funk Clav", 0, "clav"));
    e.push_back(E("KEYS", "Clavinet", "Wah Clav", 0, "clav", { { "fLfo", .7f }, { "lfoRate", 2.2f }, { "fCut", 1200 }, { "fRes", .5f } }));
    e.push_back(E("KEYS", "Organs", "Hammond B3 Gospel", 0, "organ", { { "fDrv", 7 }, { "lBLvl", .25f } }));
    e.push_back(E("KEYS", "Organs", "Church Pipe Organ", 0, "pipe"));
    e.push_back(E("KEYS", "Organs", "Cryo Organ", 1, "organ", { { "aAtk", .3f }, { "aRel", 2.f }, { "lBLvl", .4f }, { "bDet", .9f }, { "bTbl", .75f }, { "smMix", .3f }, { "rWet", .5f }, { "aWid", 1.9f } }));
    // BASS
    e.push_back(E("BASS", "Sub Bass", "Clean Sine Sub", 0, "subsine"));
    e.push_back(E("BASS", "Sub Bass", "Triangle Sub", 0, "subtri"));
    e.push_back(E("BASS", "Sub Bass", "Gravity Well", 1, "subsine", { { "lBLvl", .35f }, { "bFM", .45f }, { "bRat", 1.f }, { "fLfo", .5f }, { "lfoRate", .5f }, { "fCut", 900 } }));
    e.push_back(E("BASS", "Sub Bass", "Dark Matter Sub", 1, "subsine", { { "lCLvl", .1f }, { "cCol", .15f }, { "cGrn", .3f }, { "fDrv", 6 }, { "rmMix", .15f }, { "rmFreq", 55 } }));
    e.push_back(E("BASS", "Analog Bass", "Moog Fat Saw", 0, "analogbass"));
    e.push_back(E("BASS", "Analog Bass", "TB-303 Acid Bass", 0, "acid"));
    e.push_back(E("BASS", "Analog Bass", "Rustbelt Bass", 1, "analogbass", { { "bTbl", .75f }, { "fDrv", 20 }, { "bcMix", .3f }, { "bcBits", 8 }, { "fCut", 600 } }));
    e.push_back(E("BASS", "Slap/Finger Bass", "Slap Funk Bass", 0, "slap"));
    e.push_back(E("BASS", "Slap/Finger Bass", "Fretless Jazz Bass", 0, "fretless"));
    e.push_back(E("BASS", "Synth Bass", "Reese Bass", 0, "reese"));
    e.push_back(E("BASS", "Synth Bass", "Pluck Bass", 0, "pluckbass"));
    e.push_back(E("BASS", "Synth Bass", "Signal Bass", 1, "reese", { { "bFM", .5f }, { "bRat", 3.5f }, { "rmMix", .2f }, { "rmFreq", 880 }, { "fCut", 700 } }));
    // LEAD
    e.push_back(E("LEAD", "Analog/Mono Lead", "Fat Analog Lead", 0, "analoglead"));
    e.push_back(E("LEAD", "Analog/Mono Lead", "Saw Supersaw Lead", 0, "supersaw"));
    e.push_back(E("LEAD", "Analog/Mono Lead", "Ion Drift", 1, "analoglead", { { "bDet", .7f }, { "fLfo", .3f }, { "lfoRate", .7f }, { "aWid", 1.8f } }));
    e.push_back(E("LEAD", "Analog/Mono Lead", "Quasar Lead", 1, "supersaw", { { "bFM", .25f }, { "lCLvl", .08f }, { "cGrn", .5f }, { "m5", .15f } }));
    e.push_back(E("LEAD", "Pluck Lead", "Digital Pluck", 0, "pluckLead"));
    e.push_back(E("LEAD", "Pluck Lead", "Analog Pluck", 0, "analogPluck"));
    e.push_back(E("LEAD", "Pluck Lead", "Starforge Pluck", 1, "pluckLead", { { "lCLvl", .25f }, { "lCDec", 1.2f }, { "lCSus", 0 }, { "cGrn", .8f }, { "smMix", .4f } }));
    e.push_back(E("LEAD", "Sync/FM Lead", "Hard Sync Lead", 0, "sync"));
    e.push_back(E("LEAD", "Sync/FM Lead", "FM Bell Lead", 0, "fmbell", { { "voices", 4 } }));
    e.push_back(E("LEAD", "Sync/FM Lead", "ZIL Signal", 1, "sync", { { "bFM", .75f }, { "bRat", 2.5f }, { "rmMix", .35f }, { "rmFreq", 330 }, { "bcMix", .15f }, { "bcBits", 10 } }));
    // PAD
    e.push_back(E("PAD", "Warm Analog Pad", "Lush Strings Pad", 0, "warmpad"));
    e.push_back(E("PAD", "Warm Analog Pad", "Chorus Analog Pad", 0, "chorpad"));
    e.push_back(E("PAD", "Warm Analog Pad", "Pulsar Bloom", 1, "warmpad", { { "lCLvl", .1f }, { "cGrn", .6f }, { "smMix", .4f }, { "fLfo", .35f }, { "lfoRate", .4f } }));
    e.push_back(E("PAD", "Warm Analog Pad", "Xeno Choir", 1, "warmpad", { { "aInst", 14 }, { "ftMix", .5f }, { "ftShift", .3f }, { "lBLvl", .2f } }));
    e.push_back(E("PAD", "Digital/Wavetable Pad", "Evolving Wavetable", 0, "wtpad"));
    e.push_back(E("PAD", "Digital/Wavetable Pad", "Nebula Drift", 1, "wtpad", { { "fLfo", .8f }, { "lfoRate", .09f }, { "smMix", .3f }, { "bFM", .15f } }));
    e.push_back(E("PAD", "Ambient/Cinematic Drone", "Dark Drone", 0, "drone"));
    e.push_back(E("PAD", "Ambient/Cinematic Drone", "Deep Space Transmission", 1, "drone", { { "lCLvl", .4f }, { "cGrn", .8f }, { "smMix", .5f }, { "rmMix", .15f }, { "rmFreq", 90 } }));
    e.push_back(E("PAD", "Ambient/Cinematic Drone", "Event Horizon", 1, "drone", { { "fCut", 500 }, { "fLfo", .8f }, { "lfoRate", .05f }, { "bFM", .3f } }));
    // STRINGS & ORCHESTRAL
    e.push_back(E("STRINGS", "Solo Strings", "Solo Violin", 0, "solostr"));
    e.push_back(E("STRINGS", "Solo Strings", "Solo Cello", 0, "cello"));
    e.push_back(E("STRINGS", "Ensemble Strings", "Sustain Ensemble", 0, "ensemble"));
    e.push_back(E("STRINGS", "Ensemble Strings", "Staccato Ensemble", 0, "stacc"));
    e.push_back(E("STRINGS", "Ensemble Strings", "Pizzicato Strings", 0, "pizz"));
    e.push_back(E("STRINGS", "Brass Section", "Trumpet Section", 0, "brass"));
    e.push_back(E("STRINGS", "Brass Section", "Horn Section", 0, "brass", { { "fCut", 1600 }, { "lATune", -5 }, { "lBLvl", .15f } }));
    e.push_back(E("STRINGS", "Brass Section", "Trombone Section", 0, "brass", { { "lATune", -12 }, { "fCut", 1400 }, { "lBTune", -12 } }));
    e.push_back(E("STRINGS", "Woodwinds", "Flute", 0, "flute"));
    e.push_back(E("STRINGS", "Woodwinds", "Oboe", 0, "oboe"));
    e.push_back(E("STRINGS", "Woodwinds", "Clarinet", 0, "clarinet"));
    // PLUCK (guitars & plucked)
    e.push_back(E("PLUCK", "Clean Electric", "Clean Strat", 0, "strat"));
    e.push_back(E("PLUCK", "Clean Electric", "Jazz Hollowbody", 0, "jazzgtr"));
    e.push_back(E("PLUCK", "Overdriven/Rock", "Crunch Rock Guitar", 0, "crunch"));
    e.push_back(E("PLUCK", "Overdriven/Rock", "Distorted Lead Guitar", 0, "distlead"));
    e.push_back(E("PLUCK", "Acoustic", "Steel-String Acoustic", 0, "acoustic"));
    e.push_back(E("PLUCK", "Acoustic", "Nylon Classical", 0, "nylon"));
    e.push_back(E("PLUCK", "Ethnic Plucks", "Concert Harp", 0, "harp"));
    e.push_back(E("PLUCK", "Ethnic Plucks", "Marimba", 0, "marimba"));
    e.push_back(E("PLUCK", "Ethnic Plucks", "Kalimba", 0, "kalimba"));
    e.push_back(E("PLUCK", "Ethnic Plucks", "Glass Kalimba", 1, "kalimba", { { "bcMix", .5f }, { "bcBits", 7 }, { "bcRate", .45f }, { "lBLvl", .2f }, { "bTbl", 1.f }, { "lBTune", 12 }, { "lBDec", 1.f }, { "lBSus", 0 } }));
    // DRUMS & PERCUSSION
    e.push_back(E("DRUMS", "Acoustic Kits", "Studio Rock Kit", 0, "kit", { { "kit", 1 }, { "fCut", 16000 }, { "fEnv", 0 }, { "fRes", .05f } }));
    e.push_back(E("DRUMS", "Acoustic Kits", "Jazz Brush Kit", 0, "kit", { { "kit", 1 }, { "fCut", 5200 }, { "fRes", .05f }, { "fEnv", 0 }, { "aVel", .9f }, { "aGain", 0 }, { "rWet", .2f } }));
    e.push_back(E("DRUMS", "Electronic Kits", "808 Trap Kit", 0, "kit", { { "kit", 2 }, { "fCut", 18000 }, { "fRes", .05f }, { "fEnv", 0 }, { "fDrv", 3 } }));
    e.push_back(E("DRUMS", "Electronic Kits", "Techno Kit", 0, "kit", { { "kit", 2 }, { "fCut", 12000 }, { "fRes", .1f }, { "fEnv", 0 }, { "fDrv", 8 }, { "bcMix", .15f }, { "bcBits", 10 } }));
    e.push_back(E("DRUMS", "Electronic Kits", "Transmitter Kit", 1, "kit", { { "kit", 3 }, { "fCut", 15000 }, { "fRes", .1f }, { "fEnv", 0 }, { "rmMix", .2f }, { "rmFreq", 520 }, { "smMix", .25f }, { "dOn", 1 }, { "dMix", .18f } }));
    e.push_back(E("DRUMS", "Percussion Loops", "World Percussion", 0, "kit", { { "kit", 4 }, { "fCut", 12000 }, { "fRes", .05f }, { "fEnv", 0 }, { "rWet", .25f } }));
    e.push_back(E("DRUMS", "Percussion Loops", "Hand Drums", 0, "kit", { { "kit", 4 }, { "fCut", 6000 }, { "fRes", .05f }, { "fEnv", 0 }, { "aVel", .9f } }));
    // FX
    e.push_back(E("FX", "Risers/Impacts", "Cinematic Riser", 0, "riser"));
    e.push_back(E("FX", "Risers/Impacts", "Sub Drop Impact", 0, "impact"));
    e.push_back(E("FX", "Signature ZIL Textures", "Signal Decay", 1, "fmbell", { { "bcMix", .4f }, { "bcBits", 6 }, { "bcRate", .5f }, { "lBDec", 4.f }, { "aRel", 4.f }, { "dOn", 1 }, { "dMix", .4f }, { "dFb", .65f } }));
    e.push_back(E("FX", "Signature ZIL Textures", "Cryo Chamber", 1, "drone", { { "aInst", 12 }, { "fCut", 2500 }, { "smMix", .6f }, { "smSize", .9f }, { "rWet", .6f } }));
    e.push_back(E("FX", "Signature ZIL Textures", "Hyperdrive Pulse", 1, "riser", { { "aAtk", .3f }, { "lBAtk", .3f }, { "lCAtk", .3f }, { "fAtk", .6f }, { "fLfo", .9f }, { "lfoRate", 6.f }, { "lfoShape", 3 }, { "rmMix", .3f }, { "rmFreq", 140 } }));
    e.push_back(E("FX", "Signature ZIL Textures", "Alien Choir", 1, "warmpad", { { "aInst", 15 }, { "ftMix", .7f }, { "ftShift", .7f }, { "rmMix", .1f }, { "rmFreq", 150 }, { "smMix", .3f } }));
    e.push_back(E("FX", "Signature ZIL Textures", "Ghost Telemetry", 1, "kalimba", { { "lCLvl", .15f }, { "cGrn", .9f }, { "lCDec", 2.f }, { "lCSus", 0 }, { "smMix", .5f }, { "dOn", 1 }, { "dMix", .35f }, { "dFb", .6f } }));
    // ARPS & SEQUENCES
    e.push_back(E("ARP", "Rhythmic Plucks", "Arp Pluck Sequence", 0, "arppluck"));
    e.push_back(E("ARP", "Rhythmic Plucks", "Trance Gate Arp", 0, "gatearp"));
    e.push_back(E("ARP", "Evolving Sequences", "ZIL Pulse Grid", 1, "pulsegrid"));
    e.push_back(E("ARP", "Evolving Sequences", "Signal 47", 1, "pulsegrid", { { "arpMode", 2 }, { "arpEvo", .3f }, { "lBLvl", .6f }, { "rmMix", .2f }, { "rmFreq", 470 }, { "bFM", .5f } }));
    return e;
}

uint32_t fnv(const std::string& s) { uint32_t h = 2166136261u; for (unsigned char c : s) { h ^= c; h *= 16777619u; } return h; }

std::array<float, P_COUNT> computeValues(const Entry& e)
{
    const auto& D = defs();
    std::array<float, P_COUNT> v {};
    for (int i = 0; i < P_COUNT; ++i) v[(size_t) i] = D[(size_t) i].def;
    auto apply = [&](const KV& kv) {
        for (auto& p : kv) { int i = indexOf(p.first); jassert(i >= 0); if (i >= 0) v[(size_t) i] = juce::jlimit(D[(size_t) i].mn, D[(size_t) i].mx, p.second); } };
    apply(archetypes().at(e.arch));
    if (e.sig)
    {
        const uint32_t h = fnv(e.name);
        switch (h % 4)
        {
            case 0: apply({ { "bcMix", .3f }, { "bcBits", 8 }, { "rmMix", .12f }, { "rmFreq", 330 } }); break;
            case 1: apply({ { "smMix", .4f }, { "smSize", .6f }, { "cGrn", .6f } }); break;
            case 2: apply({ { "ftMix", .4f }, { "ftShift", (float) (h % 100) / 100.f }, { "smMix", .2f } }); break;
            default: apply({ { "rmMix", .3f }, { "rmFreq", 110.f + (float) (h % 400) }, { "dOn", 1 }, { "dMix", .3f } }); break;
        }
        apply({ { "m5", .2f } });
    }
    apply(e.tweaks);
    return v;
}

// ---- starter pack: dark / cloudy underground-rap sounds (drop-in .zilpreset files; not part of the factory bank)
std::vector<Entry> packEntries()
{
    auto E = [](const char* s, const char* n, const char* a, KV t) { return Entry { "Cloud Rap Vol.1", s, n, false, a, t }; };
    std::vector<Entry> e;
    const char* B = "808 and Bass"; const char* K = "Bells and Plucks"; const char* P = "Pads and Air"; const char* Y = "Keys"; const char* Ld = "Leads"; const char* D = "Drums and FX";
    e.push_back(E(B, "Cloud 808 Long", "subsine", { { "lALvl", .95f }, { "lADec", 2.4f }, { "lASus", 0 }, { "lARel", .4f }, { "aRel", .4f }, { "fDrv", 8 }, { "fCut", 900 }, { "fEnv", 0 }, { "glide", .08f }, { "aGain", 2 }, { "rWet", 0 } }));
    e.push_back(E(B, "Distorted 808 Slide", "subsine", { { "lADec", 1.6f }, { "lASus", 0 }, { "fDrv", 20 }, { "glide", .15f }, { "lBLvl", .3f }, { "bTbl", .75f }, { "lBTune", -12 }, { "lBDec", 1.2f }, { "lBSus", 0 }, { "fCut", 1800 }, { "bcMix", .1f }, { "rWet", 0 } }));
    e.push_back(E(B, "Hollow Sub Boom", "subsine", { { "lATune", -12 }, { "lADec", 3.5f }, { "lASus", 0 }, { "aRel", .6f }, { "fCut", 400 }, { "fEnv", 0 }, { "rOn", 1 }, { "rWet", .15f }, { "rSize", .9f } }));
    e.push_back(E(B, "Grit Bass Stab", "pluckbass", { { "fDrv", 14 }, { "fCut", 1400 }, { "bcMix", .2f }, { "bcBits", 9 } }));
    e.push_back(E(B, "Rage Sub Short", "subsine", { { "lADec", .45f }, { "lASus", 0 }, { "fDrv", 12 }, { "glide", 0 }, { "aRel", .1f }, { "aGain", 2 } }));
    e.push_back(E(K, "Icy Bell Lead", "fmbell", { { "lBTune", 12 }, { "bRat", 4.f }, { "voices", 6 }, { "rWet", .5f }, { "dOn", 1 }, { "dMix", .3f }, { "dFb", .45f } }));
    e.push_back(E(K, "Glass Pluck Melody", "pluckLead", { { "bTbl", 1.f }, { "bFM", .2f }, { "lBDec", .35f }, { "rWet", .4f }, { "dMix", .25f } }));
    e.push_back(E(K, "Crystal Music Box", "kalimba", { { "lATune", 12 }, { "lADec", 1.8f }, { "rWet", .5f }, { "rType", 1 }, { "smMix", .2f } }));
    e.push_back(E(K, "Detuned Bell Dream", "fmbell", { { "bDet", .8f }, { "lBLvl", .6f }, { "smMix", .3f }, { "rWet", .55f } }));
    e.push_back(E(K, "Soft Kalimba Dream", "kalimba", { { "fCut", 4500 }, { "rWet", .45f }, { "dOn", 1 }, { "dMix", .3f } }));
    e.push_back(E(K, "Sparkle Pluck", "analogPluck", { { "bTbl", .75f }, { "bFM", .15f }, { "lBTune", 12 }, { "rWet", .35f }, { "dMix", .3f } }));
    e.push_back(E(P, "Cloud Pad", "warmpad", { { "fCut", 1800 }, { "fLfo", .2f }, { "lfoRate", .25f }, { "rWet", .65f }, { "rSize", .95f }, { "aWid", 1.9f } }));
    e.push_back(E(P, "Night Drive Pad", "chorpad", { { "fCut", 2200 }, { "dOn", 1 }, { "dMix", .25f }, { "rWet", .5f } }));
    e.push_back(E(P, "Foggy Strings", "ensemble", { { "aAtk", .5f }, { "lAAtk", .6f }, { "fCut", 1500 }, { "rWet", .6f }, { "bcMix", .1f } }));
    e.push_back(E(P, "Wide Air Pad", "wtpad", { { "lCLvl", .12f }, { "cGrn", .5f }, { "cCol", .8f }, { "rWet", .6f }, { "aWid", 2.f } }));
    e.push_back(E(P, "Void Choir", "warmpad", { { "aInst", 14 }, { "ftMix", .4f }, { "ftShift", .2f }, { "rWet", .7f }, { "fCut", 1800 } }));
    e.push_back(E(Y, "Dark Piano Loop", "grand", { { "fCut", 1800 }, { "lADec", 2.5f }, { "rWet", .35f }, { "lBLvl", .1f }, { "bcMix", .1f } }));
    e.push_back(E(Y, "Lo-Fi EP Haze", "ep", { { "bcMix", .35f }, { "bcBits", 9 }, { "bcRate", .3f }, { "fCut", 2500 }, { "fLfo", .15f }, { "lfoRate", .4f }, { "rWet", .4f } }));
    e.push_back(E(Y, "Detuned Piano Dream", "grand", { { "lBLvl", .25f }, { "lBTune", .14f }, { "smMix", .25f }, { "rWet", .45f } }));
    e.push_back(E(Ld, "Rage Lead Saw", "supersaw", { { "fDrv", 10 }, { "fCut", 5000 }, { "voices", 1 }, { "glide", .06f }, { "dOn", 1 }, { "dMix", .2f } }));
    e.push_back(E(Ld, "Hyper Square Lead", "pluckLead", { { "lBDec", 1.f }, { "lBSus", .8f }, { "bFM", .1f }, { "dMix", .25f } }));
    e.push_back(E(Ld, "Whisper Synth Lead", "analoglead", { { "lCLvl", .06f }, { "cCol", .8f }, { "fCut", 2800 }, { "rWet", .45f } }));
    e.push_back(E(D, "Trap Kit Hard", "kit", { { "kit", 2 }, { "fCut", 18000 }, { "fRes", .05f }, { "fEnv", 0 }, { "fDrv", 5 } }));
    e.push_back(E(D, "Lo-Fi Drill Kit", "kit", { { "kit", 1 }, { "bcMix", .25f }, { "bcBits", 10 }, { "fCut", 8000 }, { "fRes", .05f }, { "fEnv", 0 } }));
    e.push_back(E(D, "Dark Reverse Riser", "riser", { { "fCut", 400 }, { "lCCol", .3f }, { "rWet", .6f } }));
    return e;
}

std::vector<Preset> build()
{
    std::vector<Preset> out;
    for (const auto& e : entries())
    {
        const auto v = computeValues(e);

        struct Variant { const char* suffix; };
        static const char* suffixes[4] = { "", " Bright", " Dark", " Wide" };
        for (int vi = 0; vi < 4; ++vi)
        {
            Preset p; p.name = juce::String(e.name) + suffixes[vi]; p.category = e.cat; p.sub = e.sub; p.signature = e.sig; p.v = v;
            auto at = [&](const char* id) -> float& { return p.v[(size_t) indexOf(id)]; };
            if (vi == 1) { at("fCut") = juce::jmin(20000.f, at("fCut") * 2.2f); at("fRes") = juce::jmin(1.f, at("fRes") + 0.05f); }
            if (vi == 2) { at("fCut") = juce::jmax(60.f, at("fCut") * 0.45f); at("rWet") = juce::jmin(1.f, at("rWet") + 0.05f); }
            if (vi == 3) { at("aWid") = 1.9f; at("bDet") = juce::jmin(1.f, at("bDet") + 0.3f); at("rOn") = 1; at("rWet") = juce::jmin(0.8f, at("rWet") + 0.15f); }
            out.push_back(std::move(p));
        }
    }
    return out;
}
}

namespace
{
std::vector<Preset>& store() { static std::vector<Preset> v = [] { auto b = build(); b.reserve(b.size() + 4000); return b; }(); return v; }
size_t& factoryN() { static size_t n = 0; return n; }
juce::File& dirRef() { static juce::File f = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("ZIL").getChildFile("Presets"); return f; }
}

const std::vector<Preset>& presets() { auto& v = store(); if (factoryN() == 0) factoryN() = v.size(); return v; }
int factoryCount() { presets(); return (int) factoryN(); }
juce::File userPresetDir() { return dirRef(); }
void setUserPresetDir(const juce::File& f) { dirRef() = f; }

int reloadUserPresets()
{
    auto& v = store(); presets(); v.resize(factoryN());
    const auto dir = dirRef(); dir.createDirectory();
    auto files = dir.findChildFiles(juce::File::findFiles, true, "*.zilpreset");
    files.sort();
    const auto& D = defs(); int n = 0;
    for (const auto& f : files)
    {
        if (n >= 3500) break;
        auto xml = juce::parseXML(f);
        if (!xml || !xml->hasTagName("ZILPreset")) continue;
        Preset p; p.user = true; p.file = f;
        p.name = xml->getStringAttribute("name", f.getFileNameWithoutExtension());
        p.category = xml->getStringAttribute("category", "USER"); p.sub = xml->getStringAttribute("sub", "User");
        for (int i = 0; i < P_COUNT; ++i) p.v[(size_t) i] = D[(size_t) i].def;
        for (auto* c : xml->getChildIterator())
            if (c->hasTagName("P")) { int i = indexOf(c->getStringAttribute("id")); if (i >= 0) p.v[(size_t) i] = juce::jlimit(D[(size_t) i].mn, D[(size_t) i].mx, (float) c->getDoubleAttribute("v")); }
        v.push_back(std::move(p)); ++n;
    }
    return n;
}

static bool writePreset(const juce::File& f, const juce::String& name, const juce::String& cat, const juce::String& sub, const std::array<float, P_COUNT>& vals)
{
    juce::XmlElement root("ZILPreset"); root.setAttribute("name", name); root.setAttribute("category", cat); root.setAttribute("sub", sub); root.setAttribute("version", 1);
    for (int i = 0; i < P_COUNT; ++i) { auto* c = root.createNewChildElement("P"); c->setAttribute("id", defs()[(size_t) i].id); c->setAttribute("v", (double) vals[(size_t) i]); }
    return f.replaceWithText(root.toString());
}

bool savePreset(const juce::String& name, const std::array<float, P_COUNT>& vals, juce::File* written)
{
    auto dir = dirRef(); dir.createDirectory();
    auto base = juce::File::createLegalFileName(name.trim().isEmpty() ? juce::String("My Preset") : name.trim());
    auto f = dir.getChildFile(base + ".zilpreset"); int k = 2;
    while (f.existsAsFile()) f = dir.getChildFile(base + " " + juce::String(k++) + ".zilpreset");
    if (written) *written = f;
    return writePreset(f, name.trim().isEmpty() ? juce::String("My Preset") : name.trim(), "USER", "My Presets", vals);
}

int exportPack(const juce::File& dir)
{
    dir.createDirectory(); int n = 0;
    for (const auto& e : packEntries())
        if (writePreset(dir.getChildFile(juce::File::createLegalFileName(e.name) + ".zilpreset"), e.name, e.cat, e.sub, computeValues(e))) ++n;
    return n;
}

const juce::StringArray& categoryNames()
{
    static const juce::StringArray c { "KEYS", "BASS", "LEAD", "PAD", "STRINGS", "PLUCK", "DRUMS", "FX", "ARP" };
    return c;
}
}
