#include "PluginEditor.h"

using namespace juce;
using namespace zil::ui;

namespace
{
constexpr int kW = 2000, kH = 1125;

struct Page : Component
{
    explicit Page(ZilProcessor& pr) : p(pr) { setInterceptsMouseClicks(false, true); setBounds(0, 0, kW, kH); }
    template <class T, class... A> T* add(A&&... a) { auto* c = new T(std::forward<A>(a)...); kids.emplace_back(c); addAndMakeVisible(c); return c; }
    KnobCell* knob(const char* id, const char* label, int cx, int cy, float d) { auto* k = add<KnobCell>(p, id, label, d); k->centreAt(cx, cy); return k; }
    template <class T> T* place(T* c, int x, int y, int w, int h) { c->setBounds(x, y, w, h); return c; }
    ZilProcessor& p; std::vector<std::unique_ptr<Component>> kids;
};

// small live graphs ------------------------------------------------------------------
struct EnvGraph : Component, Timer
{
    EnvGraph(ZilProcessor& pr, const char* a, const char* d, const char* s, const char* r) : p(pr), ids { a, d, s, r } { startTimerHz(12); setInterceptsMouseClicks(false, false); }
    void timerCallback() override { repaint(); }
    void paint(Graphics& g) override
    {
        auto v = [&](int i) { return p.apvts.getRawParameterValue(ids[i])->load(); };
        auto r = getLocalBounds().toFloat();
        g.setColour(Colour(0xff060a14)); g.fillRoundedRectangle(r, 8); g.setColour(Colour(0xff25325a)); g.drawRoundedRectangle(r.reduced(.5f), 8, 1.2f);
        const float a = std::sqrt(v(0)), d = std::sqrt(v(1)), s = v(2), rl = std::sqrt(v(3)); const float tot = a + d + 0.5f + rl;
        const float w = r.getWidth() - 40, h = r.getHeight() - 40, x0 = 20, y1 = 20, y0 = 20 + h; const float u = w / tot;
        Path path; path.startNewSubPath(x0, y0); path.lineTo(x0 + a * u, y1); path.lineTo(x0 + (a + d) * u, y1 + (1 - s) * h);
        path.lineTo(x0 + (a + d + 0.5f) * u, y1 + (1 - s) * h); path.lineTo(x0 + w, y0);
        Path fill(path); fill.lineTo(x0, y0); fill.closeSubPath();
        g.setGradientFill(ColourGradient(Theme::cyan.withAlpha(0.28f), 0, y1, Theme::violet.withAlpha(0.02f), 0, y0, false)); g.fillPath(fill);
        fillGlow(g, path, Theme::cyan, 2.f, 0.4f); g.setColour(Theme::cyan); g.strokePath(path, PathStrokeType(2.2f, PathStrokeType::curved));
    }
    ZilProcessor& p; const char* ids[4];
};
struct LfoScope : Component, Timer
{
    explicit LfoScope(ZilProcessor& pr) : p(pr) { startTimerHz(20); setInterceptsMouseClicks(false, false); }
    void timerCallback() override { ph += 0.02f * (0.4f + p.apvts.getRawParameterValue("lfoRate")->load() * 0.15f); if (ph > 1) ph -= 1; repaint(); }
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour(Colour(0xff060a14)); g.fillRoundedRectangle(r, 8); g.setColour(Colour(0xff25325a)); g.drawRoundedRectangle(r.reduced(.5f), 8, 1.2f);
        const int shape = (int) p.apvts.getRawParameterValue("lfoShape")->load(); const float depth = p.apvts.getRawParameterValue("fLfo")->load();
        auto f = [&](float t) { switch (shape) { case 0: return std::sin(t * 2 * kPi); case 1: return 4 * std::abs(t - .5f) - 1; case 2: return 2 * t - 1; case 3: return t < .5f ? 1.f : -1.f;
                                default: return std::sin(t * 37.f) * std::cos(t * 11.f); } };
        Path path; for (int i = 0; i <= 200; ++i) { float t = (float) i / 200.f, y = r.getCentreY() - f(t) * (r.getHeight() * 0.36f) * (0.3f + 0.7f * depth); float x = r.getX() + 16 + t * (r.getWidth() - 32);
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y); }
        fillGlow(g, path, Theme::violet, 2.f, 0.4f); g.setColour(Theme::accent(0.6f)); g.strokePath(path, PathStrokeType(2.2f));
        const float x = r.getX() + 16 + ph * (r.getWidth() - 32), y = r.getCentreY() - f(ph) * (r.getHeight() * 0.36f) * (0.3f + 0.7f * depth);
        g.setColour(Theme::amber); g.fillEllipse(x - 5, y - 5, 10, 10);
    }
    ZilProcessor& p; float ph = 0;
};

void label(Graphics& g, const String& t, int x, int y, int w, float size = 14.f, Colour c = Theme::textDim, Justification j = Justification::centredLeft)
{ g.setColour(c); g.setFont(Theme::label(size)); g.drawText(t, x, y, w, 22, j); }
void mono(Graphics& g, const String& t, int x, int y, int w, float size = 14.f, Colour c = Theme::textDim, Justification j = Justification::centredLeft)
{ g.setColour(c); g.setFont(Theme::mono(size)); g.drawText(t, x, y, w, 22, j); }

// ------------------------------------------------------------------ pages
struct OscPage : Page
{
    Browser* browser;
    explicit OscPage(ZilProcessor& pr) : Page(pr)
    {
        const int cols[4] = { 178, 301, 423, 546 };
        const char* f[8][2] = { { "fCut", "Cutoff" }, { "fRes", "Reso" }, { "fDrv", "Drive" }, { "fKey", "Key Trk" }, { "fEnv", "Env Amt" }, { "fLfo", "LFO Amt" }, { "lfoRate", "Rate" }, { "fMix", "Mix" } };
        const char* a[8][2] = { { "aAtk", "Attack" }, { "aDec", "Decay" }, { "aSus", "Sustain" }, { "aRel", "Release" }, { "aVel", "Velo" }, { "aPan", "Pan" }, { "aWid", "Width" }, { "aGain", "Gain" } };
        for (int i = 0; i < 8; ++i) { knob(f[i][0], f[i][1], cols[i % 4], i < 4 ? 327 : 493, 92); knob(a[i][0], a[i][1], cols[i % 4] + 1275, i < 4 ? 327 : 493, 92); }
        place(add<ChipToggle>(p, "fA", "A"), 529, 213, 32, 28); place(add<ChipToggle>(p, "fB", "B"), 567, 213, 32, 28);
        place(add<ChipToggle>(p, "lAOn", "A"), 1804, 213, 32, 28); place(add<ChipToggle>(p, "lBOn", "B"), 1842, 213, 32, 28);
        place(add<Emblem>(p), 946, 204, 108, 108);
        browser = place(add<Browser>(p), 676, 323, 648, 344);
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 527, 500 }, "FILTER MODIFIER"); drawPanel(g, { 1373, 195, 527, 500 }, "AMP MODIFIER");
        g.setColour(Theme::textDim); g.setFont(Theme::mono(12.f));
        g.drawText("ROUTE", 460, 215, 60, 22, Justification::centredRight); g.drawText("LAYER", 1730, 215, 70, 22, Justification::centredRight);
        // glass bezel of the central display
        Rectangle<float> r(650, 195, 700, 500); Path bz; bz.addRoundedRectangle(r, 30.f);
        DropShadow(Colours::black.withAlpha(0.6f), 24, { 0, 10 }).drawForPath(g, bz);
        g.setGradientFill(ColourGradient(Colour(0xffe3e7ef), 0, r.getY(), Colour(0xff525a6e), 0, r.getBottom(), false)); g.fillPath(bz);
        g.setColour(Colour(0xff1a2034)); g.drawRoundedRectangle(r, 30.f, 2.f);
        g.setColour(Colours::white.withAlpha(0.45f)); g.drawRoundedRectangle(r.reduced(3), 28.f, 1.4f);
        Rectangle<float> rec(662, 312, 676, 368); g.setColour(Colour(0xff0b101c)); g.fillRoundedRectangle(rec, 22);
        g.setColour(Theme::cyan.withAlpha(0.18f)); g.drawRoundedRectangle(rec, 22, 6); g.setColour(Colour(0xff35405e)); g.drawRoundedRectangle(rec, 22, 1.4f);
    }
};

struct LayerPage : Page
{
    explicit LayerPage(ZilProcessor& pr) : Page(pr)
    {
        struct Col { const char* pre; int x0; };
        const Col cols[3] = { { "lA", 0 }, { "lB", 1 }, { "lC", 2 } };
        const int cxA[4] = { 178, 301, 423, 546 }, cxB[4] = { 775, 925, 1075, 1225 }, cxC[4] = { 1451, 1574, 1696, 1819 };
        const int* cx[3] = { cxA, cxB, cxC };
        for (int L = 0; L < 3; ++L)
        {
            const char* pre = cols[L].pre;
            static const char* sfx[8] = { "Lvl", "Tune", "Cut", "Res", "Atk", "Dec", "Sus", "Rel" }; static const char* nm[8] = { "Level", "Tune", "Cutoff", "Res", "Attack", "Decay", "Sustain", "Release" };
            for (int i = 0; i < 8; ++i)
            { names.add(String(pre) + sfx[i]); knob(names.getReference(names.size() - 1).toRawUTF8(), nm[i], cx[L][i % 4], i < 4 ? 300 : 440, 78); }
        }
        place(add<ChoiceStepper>(p, "aInst"), 150, 572, 400, 50);
        knob("bTbl", "Table", cxB[0], 580, 78); knob("bFM", "FM", cxB[1], 580, 78); knob("bRat", "Ratio", cxB[2], 580, 78); knob("bDet", "Detune", cxB[3], 580, 78);
        knob("cCol", "Colour", cxC[0], 580, 78); knob("cGrn", "Grain", cxC[1], 580, 78); knob("cDns", "Density", cxC[2], 580, 78);
        place(add<ChipToggle>(p, "lAOn", "ON"), 540, 213, 60, 28); place(add<ChipToggle>(p, "lBOn", "ON"), 1262, 213, 60, 28); place(add<ChipToggle>(p, "lCOn", "ON"), 1815, 213, 60, 28);
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 527, 500 }, "LAYER A  ROM"); drawPanel(g, { 650, 195, 700, 500 }, "LAYER B  WAVETABLE / FM"); drawPanel(g, { 1373, 195, 527, 500 }, "LAYER C  NOISE / GRAIN");
        label(g, "ROM INSTRUMENT MODEL", 150, 545, 400, 13.f, Theme::textDim, Justification::centred);
    }
    StringArray names;
};

struct ModPage : Page
{
    explicit ModPage(ZilProcessor& pr) : Page(pr)
    {
        knob("fAtk", "Attack", 178, 305, 84); knob("fDec", "Decay", 301, 305, 84); knob("fSus", "Sustain", 423, 305, 84); knob("fRel", "Release", 546, 305, 84);
        place(add<EnvGraph>(p, "fAtk", "fDec", "fSus", "fRel"), 130, 440, 467, 225);
        place(add<ChoiceChips>(p, "lfoShape", StringArray { "SIN", "TRI", "SAW", "SQR", "S&H" }, 16.f), 690, 250, 620, 44);
        knob("lfoRate", "Rate", 760, 400, 90); knob("fLfo", "Depth", 890, 400, 90);
        place(add<LfoScope>(p), 990, 330, 330, 150);
        place(add<ChipToggle>(p, "lfoSync", "SYNC", 15.f), 690, 540, 80, 44);
        place(add<ChoiceChips>(p, "lfoDiv", StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" }, 15.f), 785, 540, 525, 44);
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 527, 500 }, "FILTER ENVELOPE"); drawPanel(g, { 650, 195, 700, 500 }, "LFO 1"); drawPanel(g, { 1373, 195, 527, 500 }, "MACRO MAP");
        static const char* m[8][2] = { { "M1 CUT", "filter cutoff  +/-3 oct" }, { "M2 MORPH", "wavetable position" }, { "M3 SPACE", "reverb + delay send" }, { "M4 GRIT", "drive + bit-crush" },
                                       { "M5 WARP", "FM depth + ring mod" }, { "M6 AIR", "layer C noise level" }, { "M7 DRIFT", "detune + analog drift" }, { "M8 PUNCH", "filter env snap" } };
        for (int i = 0; i < 8; ++i)
        { mono(g, m[i][0], 1410, 280 + i * 46, 150, 17.f, Theme::screenText); mono(g, m[i][1], 1560, 280 + i * 46, 320, 15.f, Theme::textDim); }
        mono(g, "Mod wheel -> vibrato (CC1)    Sustain pedal (CC64)", 1400, 650, 480, 12.f, Theme::textDim);
        label(g, "TEMPO SYNC", 690, 518, 200, 12.f);
    }
};

struct FxPage : Page
{
    explicit FxPage(ZilProcessor& pr) : Page(pr)
    {
        knob("ftShift", "Vowel", 235, 400, 110); knob("ftMix", "Mix", 385, 400, 110);
        knob("rmFreq", "Freq", 695, 400, 110); knob("rmMix", "Mix", 845, 400, 110);
        knob("bcBits", "Bits", 1085, 400, 96); knob("bcRate", "Rate", 1230, 400, 96); knob("bcMix", "Mix", 1375, 400, 96);
        knob("smSize", "Size", 1615, 400, 110); knob("smMix", "Mix", 1765, 400, 110);
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 420, 500 }, "FORMANT SHIFT"); drawPanel(g, { 560, 195, 420, 500 }, "RING MOD"); drawPanel(g, { 1020, 195, 420, 500 }, "BIT CRUSH"); drawPanel(g, { 1480, 195, 420, 500 }, "GRAIN SMEAR");
        const char* d[4] = { "A-E-I-O-U vowel filter morph", "sine carrier x signal", "sample-rate + bit-depth reduction", "granular cloud from the live signal" };
        for (int i = 0; i < 4; ++i) mono(g, d[i], 100 + i * 460, 520, 420, 13.f, Theme::textDim, Justification::centred);
        mono(g, "Signal-from-space processors, applied after the voices and before the master filter.", 100, 620, 1800, 15.f, Theme::screenDim, Justification::centred);
    }
};

struct ArpPage : Page
{
    explicit ArpPage(ZilProcessor& pr) : Page(pr)
    {
        place(add<LedSwitch>(p, "arpOn", "ON"), 130, 268, 150, 36); place(add<LedSwitch>(p, "arpHold", "HOLD"), 330, 268, 170, 36);
        place(add<ChoiceChips>(p, "arpMode", StringArray { "UP", "DOWN", "UP/DN", "RANDOM", "PLAYED" }, 16.f), 130, 335, 640, 46);
        place(add<ChoiceChips>(p, "arpRate", StringArray { "1/4", "1/8", "1/16", "1/32" }, 16.f), 130, 440, 420, 46);
        knob("arpOct", "Octaves", 200, 590, 82); knob("arpGate", "Gate", 360, 590, 82); knob("arpSwing", "Swing", 520, 590, 82); knob("arpEvo", "Evolve", 680, 590, 82);
        for (int i = 0; i < 16; ++i) place(add<ChipToggle>(p, ("st" + String(i)).toRawUTF8(), String(i + 1), 18.f), 862 + i * 62 + (i / 4) * 6, 330, 56, 150);
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 700, 500 }, "ARPEGGIATOR"); drawPanel(g, { 830, 195, 1070, 500 }, "STEP GRID");
        label(g, "MODE", 130, 310, 200, 12.f); label(g, "RATE", 130, 415, 200, 12.f);
        mono(g, "Steps flip on/off per bar when EVOLVE > 0  (ZIL Pulse Grid)", 860, 520, 1000, 15.f, Theme::screenDim);
        mono(g, "Synced to host tempo / transport. Hold latches chords.", 860, 552, 1000, 15.f, Theme::textDim);
    }
};

struct SetPage : Page
{
    std::function<void(int)> onScale;
    explicit SetPage(ZilProcessor& pr, std::function<void(int)> scale) : Page(pr), onScale(std::move(scale))
    {
        knob("voices", "Voices", 220, 340, 92); knob("glide", "Glide", 380, 340, 92); knob("bend", "Bend", 540, 340, 92); knob("tune", "Tune", 700, 340, 92);
        place(add<ChoiceChips>(p, "kit", StringArray { "OFF", "ACOU", "ELEC", "METAL", "PERC" }, 16.f), 140, 520, 640, 46);
        place(add<LedSwitch>(p, "tilt", "TILT"), 1160, 270, 160, 36);
        static const int widths[5] = { 900, 1200, 1500, 1800, 2400 }; static const char* names[5] = { "75%", "100%", "125%", "150%", "200%" };
        for (int i = 0; i < 5; ++i)
        {
            auto* b = add<TextButton>(names[i]); b->setBounds(1160 + i * 120, 360, 112, 46); b->onClick = [this, i] { if (onScale) onScale(widths[i]); };
            b->setColour(TextButton::buttonColourId, Colour(0xff0d1324)); b->setColour(TextButton::textColourOffId, Theme::text);
        }
    }
    void paint(Graphics& g) override
    {
        drawPanel(g, { 100, 195, 1000, 500 }, "ENGINE"); drawPanel(g, { 1130, 195, 770, 500 }, "INTERFACE  /  ABOUT");
        label(g, "KEYMAP  (DRUM / PERCUSSION MODE)", 140, 495, 600, 12.f);
        label(g, "UI SCALE  (or drag the window corner)", 1160, 335, 600, 12.f);
        g.setColour(Theme::text); g.setFont(Theme::head(30.f)); g.drawText("ZIL", 1160, 470, 200, 40, Justification::centredLeft);
        mono(g, "Xeno-Synth Transmitter  v1.0", 1160, 515, 600, 16.f, Theme::screenText);
        mono(g, "by Trill Tron", 1160, 541, 600, 16.f, Theme::textDim);
        mono(g, String(zil::presets().size()) + " factory presets  ·  3 layers  ·  16 voices", 1160, 580, 700, 15.f, Theme::textDim);
        mono(g, "Right-click any knob: MIDI learn.  Double-click: reset.", 1160, 608, 700, 15.f, Theme::textDim);
    }
};
}

// ------------------------------------------------------------------ editor implementation
struct ZilEditor::Impl
{
    struct Content : Component { std::function<void(Graphics&)> painter; void paint(Graphics& g) override { if (painter) painter(g); } };
    struct Layer : Component { std::function<void(Graphics&)> painter; void paint(Graphics& g) override { if (painter) painter(g); } };
    struct TabButton : Button
    {
        TabButton(const String& t) : Button(t) { setClickingTogglesState(false); setRadioGroupId(77); setMouseCursor(MouseCursor::PointingHandCursor); }
        void paintButton(Graphics& g, bool hover, bool) override
        {
            auto r = getLocalBounds().toFloat().reduced(2); const bool on = getToggleState();
            if (on)
            {
                g.setGradientFill(ColourGradient(Colour(0xff2a56b8), 0, r.getY(), Colour(0xff152e6e), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 10);
                g.setColour(Theme::cyan.withAlpha(0.28f)); g.drawRoundedRectangle(r.expanded(2), 12, 4); g.setColour(Theme::cyan); g.drawRoundedRectangle(r, 10, 1.6f);
            }
            else
            {
                g.setGradientFill(ColourGradient(Colour(0xff1b2339), 0, r.getY(), Colour(0xff0e1424), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 10);
                g.setColour(hover ? Colour(0xff4a5d93) : Colour(0xff2a3558)); g.drawRoundedRectangle(r, 10, 1.3f);
            }
            g.setColour(on ? Colour(0xffe4f6ff) : (hover ? Theme::text : Theme::textDim)); g.setFont(Theme::head(18.f)); g.drawText(getName(), r, Justification::centred);
        }
    };

    Impl(ZilEditor& e, ZilProcessor& p) : ed(e), proc(p)
    {
        content.painter = [this](Graphics& g) { paintChassis(g); };
        ed.addAndMakeVisible(content);
        auto* osc = new OscPage(p); browser = osc->browser;
        pages.emplace_back(osc); pages.emplace_back(new LayerPage(p)); pages.emplace_back(new ModPage(p)); pages.emplace_back(new FxPage(p)); pages.emplace_back(new ArpPage(p));
        pages.emplace_back(new SetPage(p, [this](int w) { ed.setSize(w, w * 9 / 16); }));
        for (auto& pg : pages) content.addChildComponent(*pg);

        global.painter = [this](Graphics& g) { paintGlobal(g); };
        global.setInterceptsMouseClicks(false, true); global.setBounds(0, 0, kW, kH); content.addAndMakeVisible(global);

        const char* tn[6] = { "OSC", "LAY", "MOD", "FX", "ARP", "SET" };
        for (int i = 0; i < 6; ++i)
        {
            auto* t = new TabButton(tn[i]); tabs.emplace_back(t); global.addAndMakeVisible(*t); t->setBounds(700 + i * 99, 97, 92, 58);
            t->onClick = [this, i] { ed.showPage(i); };
        }
        heart = std::make_unique<IconButton>(IconButton::Heart, "Favourite this preset"); undoB = std::make_unique<IconButton>(IconButton::Undo, "Undo"); redoB = std::make_unique<IconButton>(IconButton::Redo, "Redo");
        for (auto* b : { heart.get(), undoB.get(), redoB.get() }) global.addAndMakeVisible(*b);
        heart->setBounds(575, 105, 36, 36); undoB->setBounds(617, 105, 36, 36); redoB->setBounds(659, 105, 36, 36);
        heart->onClick = [this] { proc.toggleFavorite(currentName()); browser->refresh(); updateHeart(); };
        undoB->onClick = [this] { proc.undo.undo(); }; redoB->onClick = [this] { proc.undo.redo(); };
        browser->onPresetChanged = [this] { updateHeart(); };

        auto addK = [&](const char* id, const char* lbl, int cx, int cy, float d, const char* tag = "") { auto* k = new KnobCell(proc, id, lbl, d, tag); kids.emplace_back(k); global.addAndMakeVisible(*k); k->centreAt(cx, cy); };
        addK("master", "Master", 1840, 114, 84);
        // bottom row
        auto addC = [&](Component* c, int x, int y, int w, int h) { kids.emplace_back(c); global.addAndMakeVisible(*c); c->setBounds(x, y, w, h); };
        addC(new LedSwitch(proc, "mfOn", "ON"), 120, 819, 150, 40); addC(new ChoiceChips(proc, "mfType", StringArray { "LP", "BP", "HP" }, 15.f), 120, 866, 144, 32);
        addK("mfFreq", "Freq", 344, 840, 80); addK("mfQ", "Q", 470, 840, 80);
        addC(new LedSwitch(proc, "dOn", "ON"), 574, 819, 150, 40); addC(new ChipToggle(proc, "dSync", "SYNC", 13.f), 574, 866, 58, 32);
        addC(new ChoiceChips(proc, "dDiv", StringArray { "1/4", "1/8", "1/16" }, 13.f), 636, 866, 126, 32);
        addK("dTime", "Time", 810, 838, 66); addK("dFb", "Fdbk", 880, 838, 66); addK("dMix", "Mix", 950, 838, 66);
        addC(new LedSwitch(proc, "rOn", "ON"), 1030, 819, 150, 40); addC(new ChoiceChips(proc, "rType", StringArray { "HALL", "PLATE", "ROOM" }, 14.f), 1030, 866, 225, 32);
        addK("rSize", "Size", 1294, 840, 80); addK("rWet", "Wet", 1382, 840, 80);
        addC(new LedSwitch(proc, "oLim", "LIMIT"), 1484, 819, 170, 40); addC(new ChoiceChips(proc, "oMode", StringArray { "ST", "MONO" }, 14.f), 1484, 866, 130, 32);
        addK("oVol", "Vol", 1697, 840, 80); addC(new OutMeter(proc), 1782, 797, 98, 128);
        // footer macros
        const char* mn[8] = { "Cut", "Morph", "Space", "Grit", "Warp", "Air", "Drift", "Punch" };
        for (int i = 0; i < 8; ++i) addK(("m" + String(i + 1)).toRawUTF8(), mn[i], 156 + i * 172, 1011, 56, ("M" + String(i + 1)).toRawUTF8());
        addC(new LedSwitch(proc, "arpHold", "HOLD"), 1560, 993, 170, 36);
        auto* keys = new IconButton(IconButton::Keys, "On-screen keyboard"); keys->onClick = [this, keys] { showKeyboard(keys); };
        addC(keys, 1790, 989, 60, 44);

        updateHeart();
    }
    String currentName() const { return zil::presets()[(size_t) jlimit(0, (int) zil::presets().size() - 1, proc.currentPreset)].name; }
    void updateHeart() { heart->lit = proc.isFavorite(currentName()); heart->repaint(); }
    void showKeyboard(Component* anchor)
    {
        auto kb = std::make_unique<MidiKeyboardComponent>(proc.keyboard, MidiKeyboardComponent::horizontalKeyboard);
        kb->setSize(760, 120); kb->setKeyWidth(26.f); kb->setAvailableRange(24, 96);
        CallOutBox::launchAsynchronously(std::move(kb), anchor->getScreenBounds(), nullptr);
    }

    void paintChassis(Graphics& g)
    {
        Rectangle<float> housing(50, 40, 1900, 1045);
        g.setColour(Colour(0xff04060d)); g.fillRect(0, 0, kW, kH);
        Path hp; hp.addRoundedRectangle(housing, 58.f);
        DropShadow(Colours::black.withAlpha(0.8f), 30, { 0, 14 }).drawForPath(g, hp);
        g.setGradientFill(ColourGradient(Colour(0xff4a5878), 1000, 40, Colour(0xff0d1324), 1000, 1085, false)); g.fillPath(hp);
        g.setColour(Colours::white.withAlpha(0.25f)); g.drawRoundedRectangle(housing.reduced(1.5f), 57.f, 1.5f);
        Rectangle<float> body(66, 56, 1868, 1013);
        g.setGradientFill(ColourGradient(Colour(0xff141d38), 1000, 56, Colour(0xff080c1a), 1000, 1069, false)); g.fillRoundedRectangle(body, 46.f);
        g.setColour(Colour(0xff050811)); g.drawRoundedRectangle(body, 46.f, 3.f);
        // hex vent grille
        for (float y : { 57.f, 1065.f })
            for (int row = 0; row < 2; ++row)
                for (float x = 560 + (float) row * 7.f; x < 1440; x += 14.f)
                {
                    Path hx; for (int k = 0; k < 6; ++k) { float a = kPi / 6 + (float) k * kPi / 3; auto pt = Point<float>(x + std::cos(a) * 5.4f, y + (float) row * 6.f + std::sin(a) * 5.4f); if (k == 0) hx.startNewSubPath(pt); else hx.lineTo(pt); }
                    hx.closeSubPath(); g.setColour(Colour(0xff02040a)); g.fillPath(hx); g.setColour(Colour(0x22a0b4ff)); g.strokePath(hx, PathStrokeType(0.8f));
                }
        // backlit seams
        for (int y : { 186, 706, 965 })
        { g.setGradientFill(ColourGradient(Theme::cyan.withAlpha(0.0f), 100, 0, Theme::cyan.withAlpha(0.0f), 1900, 0, false));
          ColourGradient cg(Theme::cyan.withAlpha(0.0f), 100, 0, Theme::cyan.withAlpha(0.0f), 1900, 0, false); cg.addColour(0.5, Theme::accent(0.4f).withAlpha(0.55f)); g.setGradientFill(cg);
          g.fillRect(100.f, (float) y - 1.f, 1800.f, 2.f); g.setColour(Theme::cyan.withAlpha(0.05f)); g.fillRect(100.f, (float) y - 5.f, 1800.f, 10.f); }
        // bolts
        for (auto c : { Point<float>(89, 77), Point<float>(1910, 77), Point<float>(89, 1047), Point<float>(1910, 1047) })
        {
            g.setGradientFill(ColourGradient(Colour(0xfff0f3f8), c.x - 8, c.y - 8, Colour(0xff4b5368), c.x + 8, c.y + 8, false)); g.fillEllipse(c.x - 12, c.y - 12, 24, 24);
            g.setColour(Colour(0xff0a0d18)); g.drawEllipse(c.x - 12, c.y - 12, 24, 24, 1.2f);
            g.setColour(Colour(0xff2a3042)); g.drawLine(c.x - 7, c.y, c.x + 7, c.y, 3.f); g.drawLine(c.x, c.y - 7, c.x, c.y + 7, 3.f);
        }
        // tilt-driven gloss
        const float gx = 1000 + Theme::tx * 700;
        ColourGradient gl(Colours::transparentWhite, gx - 500, 40, Colours::transparentWhite, gx + 500, 1085, false); gl.addColour(0.5, Colours::white.withAlpha(0.045f));
        g.setGradientFill(gl); g.fillRoundedRectangle(housing, 58.f);
    }

    void paintGlobal(Graphics& g)
    {
        // top bar
        Rectangle<float> tb(100, 75, 1800, 103); Path tp; tp.addRoundedRectangle(tb, 22.f);
        g.setGradientFill(ColourGradient(Colour(0xff1a2340), 0, tb.getY(), Colour(0xff0c1224), 0, tb.getBottom(), false)); g.fillPath(tp);
        g.setColour(Theme::cyan.withAlpha(0.07f)); g.strokePath(tp, PathStrokeType(5.f)); g.setColour(Colour(0xff2f4170)); g.strokePath(tp, PathStrokeType(1.6f));
        // logo
        GlyphArrangement ga; ga.addFittedText(Theme::head(98.f).boldened(), "ZIL", 128, 86, 160, 84, Justification::centred, 1);
        Path lp; ga.createPath(lp);
        g.setColour(Theme::cyan.withAlpha(0.18f)); g.strokePath(lp, PathStrokeType(10.f)); g.setColour(Theme::violet.withAlpha(0.25f)); g.strokePath(lp, PathStrokeType(5.f));
        g.setGradientFill(ColourGradient(Colour(0xfffafcff), 0, 92, Colour(0xff6b7691), 0, 168, false)); g.fillPath(lp);
        g.setColour(Colour(0xff10162a)); g.strokePath(lp, PathStrokeType(1.2f));
        g.setColour(Colour(0xffa89cff)); g.setFont(Theme::head(18.f)); g.drawText("BY TRILL TRON", 301, 108, 300, 24, Justification::centredLeft);
        g.setColour(Theme::textDim); g.setFont(Theme::mono(12.f)); g.drawText("XENO-SYNTH TRANSMITTER  v1.0", 301, 134, 300, 18, Justification::centredLeft);
        // avatar
        g.setGradientFill(ColourGradient(Theme::violet, 1580, 106, Theme::cyan, 1620, 146, false)); g.fillEllipse(1580, 106, 42, 42);
        g.setColour(Colour(0xff0a0f1e)); g.setFont(Theme::head(15.f)); g.drawText("TT", 1580, 106, 42, 42, Justification::centred);
        g.setColour(Colour(0xff8a96b4)); g.drawEllipse(1580, 106, 42, 42, 1.5f);
        // cpu + voices
        const int v = proc.engine.nVoices.load(); const int th[4] = { 1, 3, 6, 10 };
        for (int i = 0; i < 4; ++i) { const bool on = v >= th[i]; if (on) { g.setColour(Theme::amber.withAlpha(0.3f)); g.fillEllipse(1685.f + i * 18 - 3, 99.f, 16, 16); }
            g.setColour(on ? Theme::amber : Theme::amberDim); g.fillEllipse(1688.f + (float) i * 18, 102, 10, 10); }
        g.setColour(Theme::text); g.setFont(Theme::mono(15.f));
        g.drawText("CPU " + String((int) (proc.cpuLoad.load() * 100.f)) + "% · " + String((int) (proc.getSampleRate() / 1000.0)) + "k", 1625, 122, 140, 22, Justification::centredRight);
        // bottom panels
        drawPanel(g, { 100, 717, 437, 240 }, "MASTER FILTER"); drawPanel(g, { 553, 717, 439, 240 }, "DELAY"); drawPanel(g, { 1008, 717, 439, 240 }, "REVERB"); drawPanel(g, { 1463, 717, 437, 240 }, "OUTPUT");
        // footer
        Rectangle<float> fr(100, 974, 1800, 75); Path fp; fp.addRoundedRectangle(fr, 20.f);
        g.setGradientFill(ColourGradient(Colour(0xff101830), 0, fr.getY(), Colour(0xff090e1e), 0, fr.getBottom(), false)); g.fillPath(fp);
        g.setColour(Colour(0xff2a3a66)); g.strokePath(fp, PathStrokeType(1.4f));
    }

    ZilEditor& ed; ZilProcessor& proc;
    Content content; Layer global; Browser* browser = nullptr;
    std::vector<std::unique_ptr<Component>> pages, kids; std::vector<std::unique_ptr<TabButton>> tabs;
    std::unique_ptr<IconButton> heart, undoB, redoB;
};

ZilEditor::ZilEditor(ZilProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lf);
    impl = std::make_unique<Impl>(*this, p);
    showPage(0);
    setResizable(true, true);
    setResizeLimits(800, 450, 3200, 1800);
    getConstrainer()->setFixedAspectRatio(16.0 / 9.0);
    const int w = jlimit(800, 3200, p.uiWidth);
    setSize(w, w * 9 / 16);
    addMouseListener(this, true);
    startTimerHz(30);
}

ZilEditor::~ZilEditor() { stopTimer(); setLookAndFeel(nullptr); }

void ZilEditor::paint(Graphics& g) { g.fillAll(Colour(0xff04060d)); }
void ZilEditor::resized() { proc.uiWidth = getWidth(); applyTransform(); }

void ZilEditor::applyTransform()
{
    const float s = (float) getWidth() / (float) kW;
    auto t = AffineTransform::translation(-kW * 0.5f, -kH * 0.5f).scaled(0.985f, 0.985f)
                 .sheared(Theme::tx * 0.012f, Theme::ty * 0.012f).translated(Theme::tx * 6.f, Theme::ty * 4.f).scaled(s).translated((float) getWidth() * 0.5f, (float) getHeight() * 0.5f);
    impl->content.setBounds(0, 0, kW, kH);
    impl->content.setTransform(t);
}

void ZilEditor::mouseMove(const MouseEvent& e)
{
    const auto p = e.getEventRelativeTo(this).position;
    targetTx = jlimit(-1.f, 1.f, (p.x / (float) getWidth() - 0.5f) * 2.f); targetTy = jlimit(-1.f, 1.f, (p.y / (float) getHeight() - 0.5f) * 2.f);
}
void ZilEditor::mouseExit(const MouseEvent&) { targetTx = targetTy = 0; }

void ZilEditor::showPage(int index)
{
    for (size_t i = 0; i < impl->pages.size(); ++i) impl->pages[i]->setVisible((int) i == index);
    for (size_t i = 0; i < impl->tabs.size(); ++i) impl->tabs[i]->setToggleState((int) i == index, dontSendNotification);
    impl->content.repaint();
}

void ZilEditor::timerCallback()
{
    const bool tiltOn = proc.apvts.getRawParameterValue("tilt")->load() > 0.5f;
    const float tx = tiltOn ? targetTx : 0.f, ty = tiltOn ? targetTy : 0.f;
    const float ox = Theme::tx, oy = Theme::ty;
    Theme::tx += (tx - Theme::tx) * 0.18f; Theme::ty += (ty - Theme::ty) * 0.18f;
    Theme::act = proc.engine.activity.load();
    if (std::abs(Theme::tx - ox) > 0.0015f || std::abs(Theme::ty - oy) > 0.0015f) { applyTransform(); impl->content.repaint(); }
    impl->global.repaint(100, 75, 1800, 105);
}
