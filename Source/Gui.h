#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace zil::ui
{
using namespace juce;
constexpr float kPi = MathConstants<float>::pi;

// ------------------------------------------------------------------ theme
struct Theme
{
    static inline Colour cyan { 0xff35d6ff }, violet { 0xff9a73ff }, amber { 0xffffa726 }, amberDim { 0xff4d3a1c };
    static inline Colour text { 0xffc4cfe8 }, textDim { 0xff7584a8 }, screenText { 0xff6fffe0 }, screenDim { 0xff2f9e8f };
    static inline float tx = 0, ty = 0, act = 0;     // tilt (-1..1) and modulation activity (0..1)
    static Colour accent(float t) { return cyan.interpolatedWith(violet, jlimit(0.f, 1.f, t + act * 0.3f)); }

    static Font head(float h)
    {
       #ifdef ZIL_HAS_FONTS
        static Typeface::Ptr tf = Typeface::createSystemTypefaceFor(BinaryData::Orbitron_ttf, BinaryData::Orbitron_ttfSize);
        return Font(FontOptions(tf).withHeight(h)).withExtraKerningFactor(0.06f);
       #else
        return Font(FontOptions(h, Font::bold)).withExtraKerningFactor(0.10f);
       #endif
    }
    static Font mono(float h)
    {
       #ifdef ZIL_HAS_FONTS
        static Typeface::Ptr tf = Typeface::createSystemTypefaceFor(BinaryData::ShareTechMono_ttf, BinaryData::ShareTechMono_ttfSize);
        return Font(FontOptions(tf).withHeight(h));
       #else
        return Font(FontOptions(Font::getDefaultMonospacedFontName(), h, Font::plain));
       #endif
    }
    static Font label(float h) { return Font(FontOptions(h, Font::plain)).withExtraKerningFactor(0.14f); }
};

inline void fillGlow(Graphics& g, const Path& p, Colour c, float width, float alpha)
{
    for (int i = 3; i >= 1; --i)
    { g.setColour(c.withAlpha(alpha / (float) i * 0.5f)); g.strokePath(p, PathStrokeType(width + (float) i * 3.f, PathStrokeType::curved, PathStrokeType::rounded)); }
}

inline void drawPanel(Graphics& g, Rectangle<float> r, const String& title, bool led = true)
{
    Path p; p.addRoundedRectangle(r, 14.f);
    DropShadow(Colours::black.withAlpha(0.5f), 14, { 0, 6 }).drawForPath(g, p);
    g.setGradientFill(ColourGradient(Colour(0xff1b2339), r.getX(), r.getY(), Colour(0xff111827), r.getX(), r.getBottom(), false));
    g.fillPath(p);
    g.setColour(Theme::cyan.withAlpha(0.06f)); g.strokePath(p, PathStrokeType(5.f));
    g.setColour(Colour(0xff2f4170)); g.strokePath(p, PathStrokeType(1.6f));
    g.setColour(Colour(0x14ffffff)); g.drawLine(r.getX() + 14, r.getY() + 1.5f, r.getRight() - 14, r.getY() + 1.5f);
    if (title.isNotEmpty())
    {
        if (led)
        {
            const float lx = r.getX() + 30, ly = r.getY() + 29;
            g.setColour(Theme::amber.withAlpha(0.25f)); g.fillEllipse(lx - 9, ly - 9, 18, 18);
            g.setColour(Theme::amber); g.fillEllipse(lx - 5, ly - 5, 10, 10);
            g.setColour(Colours::white.withAlpha(0.6f)); g.fillEllipse(lx - 2.5f, ly - 3.5f, 3.f, 3.f);
        }
        g.setColour(Colour(0xffd9e4ff)); g.setFont(Theme::head(21.f));
        g.drawText(title, Rectangle<float>(r.getX() + (led ? 50.f : 24.f), r.getY() + 14, r.getWidth() - 120, 30), Justification::centredLeft);
        g.setGradientFill(ColourGradient(Theme::cyan.withAlpha(0.55f), r.getX() + 20, r.getY() + 57, Theme::violet.withAlpha(0.0f), r.getRight() - 20, r.getY() + 57, false));
        g.fillRect(r.getX() + 20, r.getY() + 56, r.getWidth() - 40, 2.f);
    }
}

inline Path starPath(float cx, float cy, float r)
{
    Path p; for (int i = 0; i < 10; ++i) { float a = -kPi / 2 + (float) i * kPi / 5, rr = (i & 1) ? r * 0.42f : r;
        auto pt = Point<float>(cx + std::cos(a) * rr, cy + std::sin(a) * rr); if (i == 0) p.startNewSubPath(pt); else p.lineTo(pt); }
    p.closeSubPath(); return p;
}
inline Path diamondPath(float cx, float cy, float r) { Path p; p.startNewSubPath(cx, cy - r); p.lineTo(cx + r, cy); p.lineTo(cx, cy + r); p.lineTo(cx - r, cy); p.closeSubPath(); return p; }

// ------------------------------------------------------------------ look and feel (3D knob)
class ZilLF : public LookAndFeel_V4
{
public:
    ZilLF()
    {
        setColour(PopupMenu::backgroundColourId, Colour(0xff10162a));
        setColour(PopupMenu::textColourId, Theme::text);
        setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xff1d3a6e));
        setColour(PopupMenu::highlightedTextColourId, Theme::cyan);
        setColour(TooltipWindow::backgroundColourId, Colour(0xee0a1020));
        setColour(TooltipWindow::textColourId, Theme::screenText);
        setColour(TooltipWindow::outlineColourId, Theme::cyan.withAlpha(0.5f));
    }
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s) override
    {
        const float d = (float) jmin(w, h); const auto c = Rectangle<float>((float) x, (float) y, (float) w, (float) h).getCentre();
        const float R = d * 0.5f; const bool drag = s.isMouseButtonDown(), hover = s.isMouseOverOrDragging();
        const float ringW = jmax(3.f, d * 0.045f), rArc = R - ringW * 0.5f - 1.f;
        auto pt = [&](float r, float a) { return Point<float>(c.x + r * std::sin(a), c.y - r * std::cos(a)); };

        Path track; track.addCentredArc(c.x, c.y, rArc, rArc, 0, a0, a1, true);
        g.setColour(Colour(0xff080d1c)); g.strokePath(track, PathStrokeType(ringW + 2, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(Colour(0xff1c2748)); g.strokePath(track, PathStrokeType(ringW * 0.6f, PathStrokeType::curved, PathStrokeType::rounded));
        const float ang = a0 + pos * (a1 - a0);
        if (pos > 0.002f)
        {
            Path v; v.addCentredArc(c.x, c.y, rArc, rArc, 0, a0, ang, true);
            const auto ca = Theme::accent(0.f), cb = Theme::accent(0.85f);
            fillGlow(g, v, ca.interpolatedWith(cb, pos), ringW, hover || drag ? 0.5f : 0.35f);
            g.setGradientFill(ColourGradient(ca, c.x - rArc, c.y + rArc * 0.5f, cb, c.x + rArc, c.y - rArc * 0.5f, false));
            g.strokePath(v, PathStrokeType(ringW, PathStrokeType::curved, PathStrokeType::rounded));
        }
        const float rk = R - ringW * 2.3f;
        Path body; body.addEllipse(c.x - rk, c.y - rk, rk * 2, rk * 2);
        DropShadow(Colours::black.withAlpha(drag ? 0.9f : 0.62f), (int) (rk * (drag ? 0.62f : 0.34f)),
                   { (int) (-Theme::tx * rk * 0.12f), (int) (rk * (drag ? 0.26f : 0.15f) - Theme::ty * rk * 0.08f) }).drawForPath(g, body);
        // rim
        g.setGradientFill(ColourGradient(Colour(0xfff4f6fb), c.x - rk * (0.7f + Theme::tx * 0.25f), c.y - rk * (0.8f + Theme::ty * 0.25f),
                                         Colour(0xff222838), c.x + rk * 0.7f, c.y + rk * 0.85f, false));
        g.fillEllipse(c.x - rk, c.y - rk, rk * 2, rk * 2);
        g.setColour(Colour(0xff0a0e1a)); g.drawEllipse(c.x - rk, c.y - rk, rk * 2, rk * 2, 1.f);
        // brushed aluminium face
        const float rf = rk * 0.84f;
        g.setGradientFill(ColourGradient(Colour(0xfff1f3f8), c.x - rf * (0.35f + Theme::tx * 0.3f), c.y - rf * (0.45f + Theme::ty * 0.3f),
                                         Colour(0xff858da1), c.x + rf * 0.75f, c.y + rf * 0.85f, true));
        g.fillEllipse(c.x - rf, c.y - rf, rf * 2, rf * 2);
        for (int k = 0; k < 48; ++k)
        {
            const float a = (float) k * kPi * 2.f / 48.f; g.setColour((k & 1) ? Colour(0x0f000000) : Colour(0x14ffffff));
            g.drawLine(Line<float>(pt(rf * 0.12f, a), pt(rf * 0.98f, a)), 1.f);
        }
        // recessed indicator
        const float iw = jmax(2.6f, d * 0.036f);
        g.setColour(Colour(0x66ffffff)); g.drawLine(Line<float>(pt(rf * 0.22f, ang).translated(0.8f, 1.2f), pt(rf * 0.9f, ang).translated(0.8f, 1.2f)), iw);
        g.setColour(Colour(0xff161b2b)); g.drawLine(Line<float>(pt(rf * 0.22f, ang), pt(rf * 0.9f, ang)), iw);
        g.setColour(Theme::accent(pos).withAlpha(0.9f)); g.fillEllipse(Rectangle<float>(iw * 0.9f, iw * 0.9f).withCentre(pt(rf * 0.86f, ang)));
        // specular highlight (shifts with tilt)
        const auto hc = Point<float>(c.x - rf * (0.38f + Theme::tx * 0.22f), c.y - rf * (0.5f + Theme::ty * 0.22f));
        g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.5f), hc.x, hc.y, Colours::white.withAlpha(0.f), hc.x + rf * 0.6f, hc.y + rf * 0.4f, true));
        g.fillEllipse(c.x - rf, c.y - rf, rf * 2, rf * 2);
        if (hover) { g.setColour(Theme::cyan.withAlpha(0.18f)); g.drawEllipse(c.x - rk - 1, c.y - rk - 1, rk * 2 + 2, rk * 2 + 2, 1.5f); }
    }
    void drawTooltip(Graphics& g, const String& text, int w, int h) override
    {
        g.setColour(Colour(0xf00a1020)); g.fillRoundedRectangle(0, 0, (float) w, (float) h, 5.f);
        g.setColour(Theme::cyan.withAlpha(0.6f)); g.drawRoundedRectangle(0.5f, 0.5f, (float) w - 1, (float) h - 1, 5.f, 1.f);
        g.setColour(Theme::screenText); g.setFont(Theme::mono(14.f)); g.drawText(text, 6, 0, w - 12, h, Justification::centredLeft);
    }
    Font getPopupMenuFont() override { return Theme::mono(15.f); }
};

// ------------------------------------------------------------------ knob
class ZilSlider : public Slider
{
public:
    ZilSlider(ZilProcessor& p, int idx) : proc(p), pidx(idx) {}
    void mouseDown(const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            PopupMenu m; const int cc = proc.ccFor(pidx); const bool learning = proc.learnIndex.load() == pidx;
            m.addItem(1, learning ? "Waiting for MIDI CC ..." : "MIDI Learn");
            if (cc >= 0) m.addItem(2, "Clear MIDI mapping (CC " + String(cc) + ")");
            m.addItem(3, "Reset to default");
            m.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [this, safe = SafePointer<Component>(this)](int r) {
                if (safe == nullptr) return;
                if (r == 1) proc.learnIndex.store(pidx);
                else if (r == 2) proc.clearCC(pidx);
                else if (r == 3) setValue(getDoubleClickReturnValue(), sendNotificationSync); });
            return;
        }
        Slider::mouseDown(e);
        repaint();
    }
    void mouseUp(const MouseEvent& e) override { Slider::mouseUp(e); repaint(); }
private:
    ZilProcessor& proc; int pidx;
};

class KnobCell : public Component, private Slider::Listener
{
public:
    KnobCell(ZilProcessor& p, const char* id, const String& label, float diameter, const String& tag = {})
        : idx(zil::indexOf(id)), def(zil::defs()[(size_t) zil::indexOf(id)]), labelText(label.toUpperCase()), tagText(tag), dia(diameter), slider(p, zil::indexOf(id))
    {
        jassert(idx >= 0);
        addAndMakeVisible(slider);
        slider.setSliderStyle(Slider::RotaryVerticalDrag); slider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
        slider.setMouseDragSensitivity(200); slider.setScrollWheelEnabled(true);
        slider.addListener(this);
        attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, id, slider);
        slider.setDoubleClickReturnValue(true, def.def);
        sliderValueChanged(&slider);
        horizontal = tag.isNotEmpty();
    }
    void centreAt(int cx, int cy)
    {
        if (horizontal) setBounds(cx - (int) dia / 2, cy - (int) dia / 2 - 2, (int) dia + 90, (int) dia + 4);
        else { int w = jmax((int) dia + 26, 104); setBounds(cx - w / 2, cy - (int) dia / 2 - 2, w, (int) dia + 48); }
    }
    void resized() override
    {
        if (horizontal) slider.setBounds(0, 2, (int) dia, (int) dia);
        else slider.setBounds((getWidth() - (int) dia) / 2, 2, (int) dia, (int) dia);
    }
    void paint(Graphics& g) override
    {
        if (horizontal)
        {
            const int x = (int) dia + 8;
            g.setColour(Theme::text); g.setFont(Theme::head(17.f)); g.drawText(tagText, x, 6, 80, 22, Justification::centredLeft);
            g.setColour(Theme::textDim); g.setFont(Theme::label(12.f)); g.drawText(labelText, x, 28, 80, 16, Justification::centredLeft);
            return;
        }
        const int y = (int) dia + 4;
        g.setColour(Theme::text); g.setFont(Theme::label(dia < 70 ? 14.f : 17.f)); g.drawText(labelText, 0, y, getWidth(), 22, Justification::centred);
        g.setColour(Theme::accent(0.3f).withAlpha(0.85f)); g.setFont(Theme::mono(dia < 70 ? 12.f : 13.5f)); g.drawText(valueStr, 0, y + 20, getWidth(), 18, Justification::centred);
    }
private:
    void sliderValueChanged(Slider*) override
    {
        valueStr = zil::formatValue(def.fmt, (float) slider.getValue());
        slider.setTooltip(def.name + ":  " + valueStr);
        repaint();
    }
    int idx; const zil::PDef& def; String labelText, tagText, valueStr; float dia; bool horizontal = false;
    ZilSlider slider; std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attach;
};

// ------------------------------------------------------------------ toggles
class LedSwitch : public ToggleButton
{
public:
    LedSwitch(ZilProcessor& p, const char* id, const String& text) : label(text.toUpperCase())
    { attach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, *this); setClickingTogglesState(true); setMouseCursor(MouseCursor::PointingHandCursor); }
    void paintButton(Graphics& g, bool hover, bool) override
    {
        const bool on = getToggleState(); const float h = (float) getHeight();
        auto tr = Rectangle<float>(1, (h - 26) * 0.5f, 54, 26);
        g.setGradientFill(ColourGradient(Colour(0xff05070f), 0, tr.getY(), Colour(0xff1b2238), 0, tr.getBottom(), false));
        g.fillRoundedRectangle(tr, 13.f);
        g.setColour(on ? Theme::cyan.withAlpha(0.35f) : Colour(0xff2b3558)); g.drawRoundedRectangle(tr, 13.f, 1.3f);
        const float tx = on ? tr.getRight() - 23.f : tr.getX() + 3.f;
        Rectangle<float> th(tx, tr.getY() + 3, 20, 20);
        DropShadow(Colours::black.withAlpha(0.6f), 5, { 0, 2 }).drawForPath(g, [&] { Path p; p.addEllipse(th); return p; }());
        g.setGradientFill(ColourGradient(Colour(0xfff1f3f8), th.getX(), th.getY(), Colour(0xff6c748a), th.getRight(), th.getBottom(), false));
        g.fillEllipse(th);
        const float lx = tr.getRight() + 24.f, ly = h * 0.5f;
        if (on) { g.setColour(Theme::amber.withAlpha(0.28f)); g.fillEllipse(lx - 10, ly - 10, 20, 20); }
        g.setColour(on ? Theme::amber : Theme::amberDim); g.fillEllipse(lx - 5.5f, ly - 5.5f, 11, 11);
        if (on) { g.setColour(Colours::white.withAlpha(0.6f)); g.fillEllipse(lx - 2.5f, ly - 4.f, 3.5f, 3.5f); }
        g.setColour(hover ? Colours::white : Theme::text); g.setFont(Theme::label(17.f));
        g.drawText(label, (int) lx + 14, 0, getWidth() - (int) lx - 14, getHeight(), Justification::centredLeft);
    }
private:
    String label; std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> attach;
};

inline void drawChip(Graphics& g, Rectangle<float> r, const String& t, bool on, bool hover, float fs = 15.f)
{
    if (on)
    {
        g.setGradientFill(ColourGradient(Colour(0xff1d4aa0), 0, r.getY(), Colour(0xff0f2a62), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 6.f);
        g.setColour(Theme::cyan.withAlpha(0.25f)); g.drawRoundedRectangle(r.expanded(2), 8.f, 3.f);
        g.setColour(Theme::cyan); g.drawRoundedRectangle(r, 6.f, 1.4f); g.setColour(Colour(0xffbdf1ff));
    }
    else
    {
        g.setGradientFill(ColourGradient(Colour(0xff070a14), 0, r.getY(), Colour(0xff161d33), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 6.f);
        g.setColour(hover ? Colour(0xff3d4c7c) : Colour(0xff252f4e)); g.drawRoundedRectangle(r, 6.f, 1.2f); g.setColour(hover ? Theme::text : Theme::textDim);
    }
    g.setFont(Theme::label(fs)); g.drawText(t, r, Justification::centred);
}

class ChipToggle : public ToggleButton
{
public:
    ChipToggle(ZilProcessor& p, const char* id, const String& text, float fs = 15.f) : label(text), fontSize(fs)
    { attach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, id, *this); setClickingTogglesState(true); setMouseCursor(MouseCursor::PointingHandCursor); }
    void paintButton(Graphics& g, bool hover, bool) override { drawChip(g, getLocalBounds().toFloat().reduced(1.5f), label, getToggleState(), hover, fontSize); }
private:
    String label; float fontSize; std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> attach;
};

class ChoiceChips : public Component
{
public:
    ChoiceChips(ZilProcessor& p, const char* id, StringArray labels, float fs = 15.f) : items(std::move(labels)), fontSize(fs)
    {
        auto* prm = p.apvts.getParameter(id);
        attach = std::make_unique<ParameterAttachment>(*prm, [this](float v) { index = (int) std::round(v); repaint(); }, &p.undo);
        attach->sendInitialUpdate(); setMouseCursor(MouseCursor::PointingHandCursor);
    }
    void paint(Graphics& g) override
    {
        const float w = (float) getWidth() / (float) items.size();
        for (int i = 0; i < items.size(); ++i)
            drawChip(g, Rectangle<float>(w * (float) i + 2, 1, w - 4, (float) getHeight() - 2), items[i], i == index, i == hoverIdx, fontSize);
    }
    void mouseMove(const MouseEvent& e) override { int h = hit(e); if (h != hoverIdx) { hoverIdx = h; repaint(); } }
    void mouseExit(const MouseEvent&) override { hoverIdx = -1; repaint(); }
    void mouseDown(const MouseEvent& e) override { attach->setValueAsCompleteGesture((float) hit(e)); }
private:
    int hit(const MouseEvent& e) const { return jlimit(0, items.size() - 1, (int) ((float) e.x / ((float) getWidth() / (float) items.size()))); }
    StringArray items; float fontSize; int index = 0, hoverIdx = -1; std::unique_ptr<ParameterAttachment> attach;
};

// choice with prev / next arrows (for long lists)
class ChoiceStepper : public Component
{
public:
    ChoiceStepper(ZilProcessor& p, const char* id) : prm(dynamic_cast<AudioParameterChoice*>(p.apvts.getParameter(id)))
    {
        attach = std::make_unique<ParameterAttachment>(*prm, [this](float v) { index = (int) std::round(v); repaint(); }, &p.undo);
        attach->sendInitialUpdate(); setMouseCursor(MouseCursor::PointingHandCursor);
    }
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(1.5f);
        g.setGradientFill(ColourGradient(Colour(0xff05080f), 0, r.getY(), Colour(0xff151c30), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 8.f);
        g.setColour(Colour(0xff2b3a64)); g.drawRoundedRectangle(r, 8.f, 1.3f);
        auto arrow = [&](float cx, bool left) { Path p; float s = 8; if (left) p.addTriangle(cx + s, r.getCentreY() - s, cx + s, r.getCentreY() + s, cx - s, r.getCentreY());
            else p.addTriangle(cx - s, r.getCentreY() - s, cx - s, r.getCentreY() + s, cx + s, r.getCentreY()); g.setColour(Theme::cyan); g.fillPath(p); };
        arrow(r.getX() + 24, true); arrow(r.getRight() - 24, false);
        g.setColour(Theme::screenText); g.setFont(Theme::head(19.f)); g.drawText(prm->choices[index], r.reduced(44, 0), Justification::centred);
    }
    void mouseDown(const MouseEvent& e) override
    {
        const int n = prm->choices.size(); int ni = index;
        if (e.x < getWidth() / 2) ni = (index + n - 1) % n; else ni = (index + 1) % n;
        attach->setValueAsCompleteGesture((float) ni);
    }
    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& w) override
    { const int n = prm->choices.size(); attach->setValueAsCompleteGesture((float) ((index + (w.deltaY > 0 ? -1 : 1) + n) % n)); }
private:
    AudioParameterChoice* prm; int index = 0; std::unique_ptr<ParameterAttachment> attach;
};

class IconButton : public Button
{
public:
    enum Kind { Heart, Undo, Redo, Keys };
    IconButton(Kind k, const String& tip) : Button(tip), kind(k) { setTooltip(tip); setMouseCursor(MouseCursor::PointingHandCursor); }
    bool lit = false;
    void paintButton(Graphics& g, bool hover, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced(1.5f);
        g.setGradientFill(ColourGradient(down ? Colour(0xff05070f) : Colour(0xff1c2440), 0, r.getY(), Colour(0xff0b1020), 0, r.getBottom(), false));
        g.fillRoundedRectangle(r, 8.f); g.setColour(hover ? Theme::cyan.withAlpha(0.6f) : Colour(0xff2b3558)); g.drawRoundedRectangle(r, 8.f, 1.2f);
        const auto c = r.getCentre(); Path p; const Colour col = (kind == Heart && lit) ? Colour(0xffff5c8a) : (hover ? Theme::cyan : Theme::textDim);
        g.setColour(col);
        if (kind == Heart)
        {
            p.startNewSubPath(c.x, c.y + 9); p.cubicTo(c.x - 16, c.y - 2, c.x - 9, c.y - 14, c.x, c.y - 5); p.cubicTo(c.x + 9, c.y - 14, c.x + 16, c.y - 2, c.x, c.y + 9);
            if (lit) g.fillPath(p); else g.strokePath(p, PathStrokeType(2.f));
        }
        else if (kind == Undo || kind == Redo)
        {
            const float s = kind == Undo ? -1.f : 1.f;
            p.addCentredArc(c.x, c.y + 2, 9.f, 9.f, 0, s > 0 ? 0.f : -kPi * 1.0f, s > 0 ? kPi * 1.0f : 0.f, true);
            g.strokePath(p, PathStrokeType(2.2f)); Path a; a.addTriangle(c.x + s * 9.f - 5.f, c.y - 6.f, c.x + s * 9.f + 5.f, c.y - 6.f, c.x + s * 9.f, c.y + 2.f);
            g.fillPath(a);
        }
        else
        {
            for (int i = 0; i < 5; ++i) { g.setColour(Colour(0xffe5eaf5).withAlpha(0.85f)); g.fillRect(c.x - 15 + (float) i * 6.5f, c.y - 9, 5.5f, 18.f); }
            g.setColour(Colour(0xff0b1020)); for (int i : { 0, 1, 3 }) g.fillRect(c.x - 15 + (float) i * 6.5f + 4.f, c.y - 9, 5.f, 11.f);
        }
    }
private:
    Kind kind;
};

// ------------------------------------------------------------------ radar emblem with MIDI status ring
class Emblem : public Component, private Timer
{
public:
    explicit Emblem(ZilProcessor& p) : proc(p) { startTimerHz(30); setInterceptsMouseClicks(false, false); lastMidi = p.engine.midiCount.load(); }
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(4); const auto c = b.getCentre(); const float R = jmin(b.getWidth(), b.getHeight()) * 0.5f;
        g.setColour(Theme::accent(0.2f).withAlpha(0.2f + pulse * 0.5f)); g.fillEllipse(c.x - R - 6, c.y - R - 6, R * 2 + 12, R * 2 + 12);
        g.setGradientFill(ColourGradient(Colour(0xff0a3a48), c.x, c.y - R * 0.3f, Colour(0xff020a10), c.x, c.y + R, true)); g.fillEllipse(c.x - R, c.y - R, R * 2, R * 2);
        g.setColour(Theme::accent(0.1f).withAlpha(0.28f)); for (float f : { 0.8f, 0.6f, 0.4f, 0.2f }) g.drawEllipse(c.x - R * f, c.y - R * f, R * f * 2, R * f * 2, 1.2f);
        g.drawLine(c.x - R, c.y, c.x + R, c.y, 1.f); g.drawLine(c.x, c.y - R, c.x, c.y + R, 1.f);
        // sweep
        Path w; w.startNewSubPath(c); w.addCentredArc(c.x, c.y, R * 0.92f, R * 0.92f, 0, sweep - 0.7f, sweep, false); w.closeSubPath();
        g.setGradientFill(ColourGradient(Theme::cyan.withAlpha(0.0f), c.x + R * std::sin(sweep - 0.7f), c.y - R * std::cos(sweep - 0.7f),
                                         Theme::cyan.withAlpha(0.45f), c.x + R * std::sin(sweep), c.y - R * std::cos(sweep), false));
        g.fillPath(w);
        g.setColour(Theme::cyan); g.drawLine(c.x, c.y, c.x + R * 0.92f * std::sin(sweep), c.y - R * 0.92f * std::cos(sweep), 1.8f);
        static const float blips[3][2] = { { 0.6f, 1.1f }, { 0.35f, 3.9f }, { 0.75f, 5.2f } };
        for (auto& bl : blips)
        {
            float d = std::fmod(sweep - bl[1] + 2 * kPi * 2, 2 * kPi); const float a = jmax(0.f, 1.f - d / 4.5f);
            g.setColour((bl[0] > 0.7f ? Theme::amber : Theme::cyan).withAlpha(0.15f + a * 0.85f));
            g.fillEllipse(c.x + R * bl[0] * std::sin(bl[1]) - 3.5f, c.y - R * bl[0] * std::cos(bl[1]) - 3.5f, 7, 7);
        }
        g.setColour(Theme::cyan); g.fillEllipse(c.x - 3, c.y - 3, 6, 6);
        // status ring (pulses with incoming MIDI)
        g.setColour(Theme::accent(0.5f).withAlpha(0.5f + pulse * 0.5f)); g.drawEllipse(c.x - R, c.y - R, R * 2, R * 2, 2.5f + pulse * 3.f);
        g.setColour(Colour(0xff8a96b4)); g.drawEllipse(c.x - R - 4, c.y - R - 4, R * 2 + 8, R * 2 + 8, 3.f);
        g.setColour(Colours::white.withAlpha(0.10f)); Path gl; gl.addCentredArc(c.x, c.y, R * 0.9f, R * 0.9f, 0, -kPi * 0.9f, -kPi * 0.2f, true); g.strokePath(gl, PathStrokeType(5.f));
    }
private:
    void timerCallback() override
    {
        sweep += 0.07f; if (sweep > 2 * kPi) sweep -= 2 * kPi;
        const int m = proc.engine.midiCount.load(); if (m != lastMidi) { lastMidi = m; pulse = 1.f; }
        pulse *= 0.88f; repaint();
    }
    ZilProcessor& proc; float sweep = 0, pulse = 0; int lastMidi = 0;
};

// ------------------------------------------------------------------ hum-bar VU
class OutMeter : public Component, private Timer
{
public:
    explicit OutMeter(ZilProcessor& p) : proc(p) { startTimerHz(30); setInterceptsMouseClicks(false, false); }
    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill(ColourGradient(Colour(0xff03060d), 0, 0, Colour(0xff0c1224), 0, r.getBottom(), false)); g.fillRoundedRectangle(r, 6.f);
        g.setColour(Colour(0xff26335a)); g.drawRoundedRectangle(r.reduced(0.5f), 6.f, 1.2f);
        const int nb = 9; const float bw = (r.getWidth() - 12) / (float) nb;
        for (int i = 0; i < nb; ++i)
        {
            const float lv = i < nb / 2 ? lvlL : (i > nb / 2 ? lvlR : jmax(lvlL, lvlR));
            const float hum = 0.045f * std::sin(phase * 2.1f + (float) i * 1.3f) + 0.03f * std::sin(phase * 5.f + (float) i);
            const float hgt = jlimit(0.03f, 1.f, lv * (0.78f + 0.22f * std::sin(phase + (float) i * 0.7f)) + hum * (lv > 0.01f ? 1.f : 0.3f));
            Rectangle<float> bar(r.getX() + 6 + (float) i * bw + 2, r.getBottom() - 6 - hgt * (r.getHeight() - 12), bw - 4, hgt * (r.getHeight() - 12));
            g.setGradientFill(ColourGradient(Colour(0xff8a6bff), 0, bar.getY(), Colour(0xff2fd4ff), 0, r.getBottom(), false));
            if (hgt > 0.85f) g.setGradientFill(ColourGradient(Theme::amber, 0, bar.getY(), Colour(0xff2fd4ff), 0, r.getBottom(), false));
            g.fillRoundedRectangle(bar, 2.f);
        }
        g.setColour(Colour(0x22ffffff)); for (int y = 8; y < (int) r.getHeight(); y += 4) g.drawHorizontalLine(y, 4.f, r.getWidth() - 4);
    }
private:
    void timerCallback() override
    {
        phase += 0.25f;
        const float l = proc.engine.peakL.load(), rr = proc.engine.peakR.load();
        lvlL += (jmin(1.f, l * 1.2f) - lvlL) * 0.45f; lvlR += (jmin(1.f, rr * 1.2f) - lvlR) * 0.45f; repaint();
    }
    ZilProcessor& proc; float lvlL = 0, lvlR = 0, phase = 0;
};

// ------------------------------------------------------------------ CRT preset browser
class Browser : public Component, private Timer
{
public:
    explicit Browser(ZilProcessor& p) : proc(p)
    {
        cats = StringArray { "ALL", "FAV", "SIG" }; cats.addArray(zil::categoryNames());
        setWantsKeyboardFocus(true); refresh(); startTimerHz(15);
        syncToCurrent();
    }
    void refresh()
    {
        list.clear(); const auto& all = zil::presets(); const String cat = cats[catIndex];
        for (int i = 0; i < (int) all.size(); ++i)
        {
            const auto& pr = all[(size_t) i];
            if (cat == "ALL" || (cat == "FAV" && proc.isFavorite(pr.name)) || (cat == "SIG" && pr.signature) || pr.category == cat) list.add(i);
        }
        presetScroll = jlimit(0, jmax(0, list.size() - rows), presetScroll); repaint();
    }
    void syncToCurrent()
    {
        const int pos = list.indexOf(proc.currentPreset);
        if (pos < 0) { return; }
        if (pos < presetScroll || pos >= presetScroll + rows) presetScroll = jlimit(0, jmax(0, list.size() - rows), pos - rows / 2);
    }
    void step(int d)
    {
        if (list.isEmpty()) return; int pos = list.indexOf(proc.currentPreset); pos = pos < 0 ? 0 : jlimit(0, list.size() - 1, pos + d);
        proc.loadPreset(list[pos]); syncToCurrent(); repaint();
    }
    std::function<void()> onPresetChanged;

    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill(ColourGradient(Colour(0xff0b3038), r.getCentreX(), r.getY() + 40, Colour(0xff020b10), r.getCentreX(), r.getBottom(), true));
        g.fillRoundedRectangle(r, 16.f);
        const auto sc = Theme::screenText, sd = Theme::screenDim;
        const auto& all = zil::presets(); const auto& cur = all[(size_t) jlimit(0, (int) all.size() - 1, proc.currentPreset)];
        g.setFont(Theme::mono(16.f)); g.setColour(sc);
        g.drawText("PRESET BROWSER", 24, 14, 300, 24, Justification::centredLeft);
        g.setColour(Theme::amber); g.fillPath(diamondPath(r.getRight() - 24 - g.getCurrentFont().getStringWidthFloat((cur.category + " / " + cur.name).toUpperCase()) - 14, 26, 5));
        g.setColour(sc); g.drawText((cur.category + " / " + cur.name).toUpperCase(), (int) r.getWidth() - 440, 14, 416, 24, Justification::centredRight);
        g.setColour(sd.withAlpha(0.6f)); g.drawHorizontalLine(45, 20, r.getWidth() - 20); g.drawVerticalLine(204, 54, 306);

        g.setFont(Theme::mono(17.f));
        for (int i = 0; i < rows; ++i)   // categories
        {
            int ci = i + catScroll; if (ci >= cats.size()) break;
            Rectangle<float> rr(24, (float) (52 + i * 28), 170, 27); const bool sel = ci == catIndex;
            if (sel) { g.setColour(sc.withAlpha(0.13f)); g.fillRoundedRectangle(rr, 4); g.setColour(sc.withAlpha(0.55f)); g.drawRoundedRectangle(rr, 4, 1.f); }
            if (sel) { Path t; t.addTriangle(rr.getX() + 8, rr.getCentreY() - 4, rr.getX() + 8, rr.getCentreY() + 4, rr.getX() + 14, rr.getCentreY()); g.setColour(sc); g.fillPath(t); }
            g.setColour(sel ? sc : sd); g.drawText(cats[ci], rr.withTrimmedLeft(28), Justification::centredLeft);
        }
        for (int i = 0; i < rows; ++i)   // presets
        {
            int li = i + presetScroll; if (li >= list.size()) break;
            const auto& pr = all[(size_t) list[li]]; Rectangle<float> rr(216, (float) (52 + i * 28), 408, 27); const bool sel = list[li] == proc.currentPreset;
            if (sel) { g.setColour(sc.withAlpha(0.13f)); g.fillRoundedRectangle(rr, 4); g.setColour(sc.withAlpha(0.55f)); g.drawRoundedRectangle(rr, 4, 1.f); }
            if (sel) { Path t; t.addTriangle(rr.getX() + 8, rr.getCentreY() - 4, rr.getX() + 8, rr.getCentreY() + 4, rr.getX() + 14, rr.getCentreY()); g.setColour(sc); g.fillPath(t); }
            if (pr.signature) { g.setColour(Theme::violet.withAlpha(0.9f)); g.fillPath(diamondPath(rr.getX() + 24, rr.getCentreY(), 3.5f)); }
            g.setColour(sel ? sc : sd.brighter(0.25f)); g.drawText(pr.name, rr.withTrimmedLeft(36).withTrimmedRight(40), Justification::centredLeft);
            const bool fav = proc.isFavorite(pr.name);
            g.setColour(fav ? Theme::amber : sd.withAlpha(0.35f)); Path st = starPath(rr.getRight() - 20, rr.getCentreY(), 8); if (fav) g.fillPath(st); else g.strokePath(st, PathStrokeType(1.2f));
        }
        // scrollbars
        auto bar = [&](float x, int total, int scroll) { if (total <= rows) return; const float h = 252.f; const float th = jmax(18.f, h * (float) rows / (float) total);
            g.setColour(sd.withAlpha(0.5f)); g.fillRoundedRectangle(x, 54 + (h - th) * (float) scroll / (float) (total - rows), 3.f, th, 1.5f); };
        bar(198, cats.size(), catScroll); bar(r.getWidth() - 12, list.size(), presetScroll);
        g.setFont(Theme::mono(13.f)); g.setColour(sd);
        g.drawText(String(list.size()) + " PRESETS", 24, (int) r.getHeight() - 28, 200, 20, Justification::centredLeft);
        g.drawText("VOX " + String(proc.engine.nVoices.load()).paddedLeft('0', 2), (int) r.getWidth() / 2 - 60, (int) r.getHeight() - 28, 120, 20, Justification::centred);
        const float lv = jmax(proc.engine.peakL.load(), proc.engine.peakR.load());
        g.drawText(String(40.f + (float) proc.currentPreset * 0.0731f, 1) + " MHz", (int) r.getWidth() - 160, (int) r.getHeight() - 28, 136, 20, Justification::centredRight);
        g.drawText("SIG", (int) r.getWidth() - 270, (int) r.getHeight() - 28, 40, 20, Justification::centredRight);
        for (int i = 0; i < 4; ++i) { g.setColour(lv * 4.f > (float) i + 0.3f ? sc : sd.withAlpha(0.35f)); g.fillRect(r.getWidth() - 224 + (float) i * 11, r.getHeight() - 23, 8.f, 10.f); }
        // CRT: scanlines + vignette + glass sheen
        g.setColour(Colour(0x16000000)); for (int y = 0; y < (int) r.getHeight(); y += 3) g.drawHorizontalLine(y, 0, r.getWidth());
        g.setGradientFill(ColourGradient(Colours::transparentBlack, r.getCentreX(), r.getCentreY(), Colour(0x88000000), r.getX(), r.getY(), true)); g.fillRoundedRectangle(r, 16.f);
        g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.07f), 0, 0, Colours::transparentWhite, 0, r.getHeight() * 0.45f, false)); g.fillRoundedRectangle(r.removeFromTop(r.getHeight() * 0.45f), 16.f);
        g.setColour(Colour(0xff031014)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 16.f, 3.f);
    }
    void mouseDown(const MouseEvent& e) override
    {
        grabKeyboardFocus();
        const int row = (e.y - 52) / 28; if (e.y < 52 || row >= rows) return;
        if (e.x >= 24 && e.x < 200)
        { const int ci = row + catScroll; if (ci < cats.size()) { catIndex = ci; presetScroll = 0; refresh(); syncToCurrent(); } }
        else if (e.x >= 216)
        {
            const int li = row + presetScroll; if (li >= list.size()) return;
            const auto& pr = zil::presets()[(size_t) list[li]];
            if (e.x > getWidth() - 56) { proc.toggleFavorite(pr.name); refresh(); if (onPresetChanged) onPresetChanged(); }
            else { proc.loadPreset(list[li]); if (onPresetChanged) onPresetChanged(); }
            repaint();
        }
    }
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const int d = w.deltaY > 0 ? -1 : 1;
        if (e.x < 204) catScroll = jlimit(0, jmax(0, cats.size() - rows), catScroll + d);
        else presetScroll = jlimit(0, jmax(0, list.size() - rows), presetScroll + d);
        repaint();
    }
    bool keyPressed(const KeyPress& k) override
    {
        if (k == KeyPress::downKey || k == KeyPress::rightKey) { step(1); if (onPresetChanged) onPresetChanged(); return true; }
        if (k == KeyPress::upKey || k == KeyPress::leftKey) { step(-1); if (onPresetChanged) onPresetChanged(); return true; }
        return false;
    }
private:
    void timerCallback() override { if (lastPreset != proc.currentPreset) { lastPreset = proc.currentPreset; syncToCurrent(); } repaint(); }
    ZilProcessor& proc; StringArray cats; Array<int> list; int catIndex = 0, catScroll = 0, presetScroll = 0, lastPreset = -1; static constexpr int rows = 9;
};
}
