#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        Toast.cpp
 * @brief       Short on-screen messages (see Toast.h)
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
    #include "Toast.h"
    #include "DisplayUi.h"
    #include "Icons/UiIcons.h"
    #include "Settings/DisplaySettings.h"
    #include <stdio.h>
    #include <string.h>
    #ifdef OPENKNX_DISPLAY_TOAST_LOG
        #include "OpenKNX.h"
    #endif

namespace
{
    using Line = char[DisplayToast::LineLen + 1];
    constexpr uint8_t BannerChars = 18; // (128 - 15) px / 6
    constexpr uint8_t CardChars = 14;   // 84 px / 6
    constexpr uint8_t CountChars = 14;  // first bottom row while "xN" is shown
    static_assert(DisplayToast::NarrowChars == BannerChars - 4 && DisplayToast::NarrowChars == CardChars &&
                      DisplayToast::NarrowChars <= DisplayToast::BottomChars,
                  "NarrowChars: the banner row beside \"x99\" and the card's second row");

    /** @brief Copies at most n characters of src (nullptr = empty) into a line field, up to a '\n'. */
    void copyLine(Line &dst, const char *src, size_t n = DisplayToast::LineLen)
    {
        size_t len = 0;
        if (src)
            while (len < n && len < DisplayToast::LineLen && src[len] != '\0' && src[len] != '\n')
                ++len;
        if (len) memcpy(dst, src, len);
        dst[len] = '\0';
    }

    const uint8_t *levelIcon(ToastLevel level)
    {
        switch (level)
        {
            case ToastLevel::Ok: return OpenKNX::Ui::kRowOk;
            case ToastLevel::Warn: return OpenKNX::Ui::kRowWarn;
            case ToastLevel::Error: return OpenKNX::Ui::kRowErr;
            default: return OpenKNX::Ui::kRowInfo;
        }
    }

    /** @brief Space to break s at so the first part fits width, nearest to the middle; 0 = none. */
    size_t wrapPoint(const char *s, size_t width)
    {
        const size_t len = strlen(s);
        size_t best = 0, bestDist = len;
        for (size_t i = 1; i < len && i <= width; ++i)
        {
            if (s[i] != ' ') continue;
            const size_t dist = 2 * i + 1 > len ? 2 * i + 1 - len : len - 2 * i - 1;
            if (dist < bestDist) best = i, bestDist = dist;
        }
        return best;
    }

    /** @brief The two text rows: line1 and line2, or line1 broken at a space when it is too wide and alone. */
    void textRows(const char *line1, const char *line2, size_t width, Line &a, Line &b)
    {
        const size_t sp = (line2[0] == '\0' && strlen(line1) > width) ? wrapPoint(line1, width) : 0;
        if (sp == 0)
        {
            copyLine(a, line1);
            copyLine(b, line2);
            return;
        }
        copyLine(a, line1, sp);
        copyLine(b, line1 + sp + 1);
    }

    /** @brief Banner text in a; a too wide text gives two rows, a then b, shown one after the other (returns true). */
    bool bannerRows(const char *line1, const char *line2, uint8_t count, Line &a, Line &b)
    {
        const size_t width = BannerChars - (count > 9 ? 4 : count > 1 ? 3 : 0); // minus " xN"
        const size_t len1 = strlen(line1), len2 = strlen(line2);
        if (len2 == 0 ? len1 <= width : len1 + 2 + len2 <= width)
        {
            if (len2)
                snprintf(a, sizeof(a), "%s  %s", line1, line2);
            else
                copyLine(a, line1);
            b[0] = '\0';
            return false;
        }
        textRows(line1, line2, width, a, b);
        return b[0] != '\0';
    }

    /** @brief Milliseconds the toast is in its final place: rises over SlideMs, falls over the last SlideMs. */
    uint32_t slideIn(uint32_t t, uint32_t life)
    {
        if (t >= life) return 0;
        const uint32_t m = t < life - t ? t : life - t;
        return m < DisplayToast::SlideMs ? m : DisplayToast::SlideMs;
    }

    /** @brief Top border of the bottom toast: 64 (hidden) .. 43 (in place). */
    int16_t bottomY(uint32_t t, uint32_t life)
    {
        return static_cast<int16_t>(43 + 21 * (DisplayToast::SlideMs - slideIn(t, life)) / DisplayToast::SlideMs);
    }

    /** @brief Top of the banner: -11 (hidden) .. 0 (in place). */
    int16_t bannerY(uint32_t t, uint32_t life)
    {
        return static_cast<int16_t>(static_cast<int16_t>(11 * slideIn(t, life) / DisplayToast::SlideMs) - 11);
    }

    enum class Act : uint8_t
    {
        Show,
        Wait, // shown when the scene allows it
        Drop, // not shown in this scene
        Wake  // wake the panel, then show
    };

    /** @brief What a message of this level does in this scene. */
    Act act(DisplayToast::Scene scene, ToastLevel level)
    {
        using S = DisplayToast::Scene;
        const bool error = level == ToastLevel::Error, warn = level >= ToastLevel::Warn;
        switch (scene)
        {
            case S::Ota: return error ? Act::Wait : Act::Drop;
            case S::Prog:
            case S::Off: return warn ? Act::Wait : Act::Drop;
            case S::Busy: return Act::Wait;
            case S::Dark: return warn ? Act::Wake : Act::Drop;
            case S::Saver: // Info and Ok over the screensaver or the dimmed panel, without waking it
            case S::Dimmed: return warn ? Act::Wake : Act::Show;
            default: return Act::Show;
        }
    }

    /** @brief "xN" for a merged entry (count 2..99), empty for a single one. */
    void countText(uint8_t count, char (&buf)[4])
    {
        char *p = buf;
        if (count > 1)
        {
            *p++ = 'x';
            if (count > 9) *p++ = static_cast<char>('0' + count / 10 % 10);
            *p++ = static_cast<char>('0' + count % 10);
        }
        *p = '\0';
    }

    void printAt(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *s, int16_t maxChars)
    {
        OpenKNX::Ui::printCut(d, x, y, s, maxChars);
    }
} // namespace

void DisplayToast::post(ToastLevel level, const uint8_t *icon9, const char *line1, const char *line2, void (*open)(), uint32_t now)
{
    Entry e;
    copyLine(e.line1, line1);
    copyLine(e.line2, line2);
    if (e.line1[0] == '\0' && e.line2[0] == '\0') return;
    e.icon = icon9;
    e.open = open;
    e.stamp = now;
    e.level = level;
    e.count = 1;
    e.seen = false;

#ifdef OPENKNX_DISPLAY_TOAST_LOG
    LogEntry &l = _log[_logNext];
    l.ms = now;
    l.level = level;
    memcpy(l.line1, e.line1, sizeof(l.line1));
    memcpy(l.line2, e.line2, sizeof(l.line2));
    _logNext = static_cast<uint8_t>((_logNext + 1) % LogSize);
    if (_logCount < LogSize) ++_logCount;
#endif

    // Same line1 = same source: the newer text replaces the queued one, within MergeMs it counts up.
    for (uint8_t i = 0; i < _count; ++i)
    {
        Entry &o = _q[i];
        if (strcmp(o.line1, e.line1) != 0) continue;
        if (now - o.stamp < MergeMs) e.count = o.count < 99 ? static_cast<uint8_t>(o.count + 1) : 99;
        if (o.level > e.level) // never lower the severity of a queued message
        {
            e.level = o.level;
            e.icon = o.icon;
            e.open = o.open;
        }
        // A merge never makes an error fresh: seen stays wherever the entry sits, unless the merge raised it to Error.
        const bool raised = o.level != ToastLevel::Error && e.level == ToastLevel::Error;
        e.seen = o.seen && !raised;
        o = e;
        ++_rev;
        if (i == 0 && _shownAt != 0)
        {
            if (raised)
            {
                // Raised to Error: a new state, it appears as a fresh card (and wakes a dimmed or dark panel once).
                _shownAt = 0;
                _form = Form::None;
                return;
            }
            // On screen: full time again without a new slide, unless others wait (a repeating source must not hold the queue).
            if (_count == 1) _shownAt = now - SlideMs != 0 ? now - SlideMs : 1;
            return;
        }
        // An error moves in front of the waiting non-errors; a fresh one also in front of the seen errors.
        const uint8_t first = _shownAt != 0 ? 1 : 0;
        while (i > first && e.level == ToastLevel::Error && (_q[i - 1].level != ToastLevel::Error || (_q[i - 1].seen && !e.seen)))
        {
            const Entry t = _q[i - 1];
            _q[i - 1] = _q[i];
            _q[i] = t;
            --i;
        }
        return;
    }
    // Retracted a moment ago (a flapping source): counted on, seen when it was; a higher level is a new state.
    if (_gone.line1[0] != '\0' && strcmp(_gone.line1, e.line1) == 0)
    {
        if (now - _gone.at < ForgetMs)
        {
            e.count = _gone.count < 99 ? static_cast<uint8_t>(_gone.count + 1) : 99;
            e.seen = _gone.seen && e.level <= _gone.level;
        }
        _gone.line1[0] = '\0';
    }
    insert(e);
}

void DisplayToast::postText(ToastLevel level, const uint8_t *icon9, const char *text, uint32_t now)
{
    if (!text) return;
    const char *nl = strchr(text, '\n');
    Line l1, l2;
    copyLine(l1, text, nl ? static_cast<size_t>(nl - text) : LineLen);
    const char *rest = nl ? nl + 1 : "";
    const char *end = strchr(rest, '\n'); // a third line is not shown
    const size_t len = end ? static_cast<size_t>(end - rest) : strlen(rest);
    if (len > BottomChars)
    {
        // Keep the end (a file name, a value): "..." + the last BottomChars - 3 characters.
        memcpy(l2, "...", 3);
        memcpy(l2 + 3, rest + len - (BottomChars - 3), BottomChars - 3);
        l2[BottomChars] = '\0';
    }
    else
        copyLine(l2, rest, len);
    post(level, icon9, l1, l2, nullptr, now);
}

bool DisplayToast::retract(const char *line1, uint32_t now)
{
    Line l1;
    copyLine(l1, line1);
    if (l1[0] == '\0') return false;
    // A merge keeps one entry per line1, so the first match is the only one.
    for (uint8_t i = 0; i < _count; ++i)
    {
        if (strcmp(_q[i].line1, l1) != 0) continue;
        _gone.at = now;
        memcpy(_gone.line1, l1, sizeof(_gone.line1));
        _gone.level = _q[i].level;
        _gone.count = _q[i].count;
        _gone.seen = _q[i].seen;
        if (i == 0)
        {
            pop();
            _form = Form::None; // nothing drawn until update() picks the next head
            _keys = 0;          // a key armed on this card must not close the next one
        }
        else
        {
            memmove(&_q[i], &_q[i + 1], (_count - i - 1) * sizeof(Entry));
            --_count;
            ++_rev;
        }
        return true;
    }
    return false;
}

void DisplayToast::insert(const Entry &e)
{
    // An error goes behind the waiting fresh errors, in front of all else waiting (a head not shown counts); the rest in arrival order.
    ++_rev;
    uint8_t pos = _count;
    if (e.level == ToastLevel::Error)
    {
        pos = (_count != 0 && _shownAt != 0) ? 1 : 0;
        while (pos < _count && _q[pos].level == ToastLevel::Error && !_q[pos].seen)
            ++pos;
    }
    if (_count == Slots)
    {
        if (pos >= Slots) return; // full: a non-error is dropped, an error when every waiting place holds a fresh error
        --_count;                 // the last waiting message makes room for the error
    }
    memmove(&_q[pos + 1], &_q[pos], (_count - pos) * sizeof(Entry));
    _q[pos] = e;
    ++_count;
}

void DisplayToast::pop()
{
    if (_count == 0) return;
    --_count;
    memmove(&_q[0], &_q[1], _count * sizeof(Entry));
    _shownAt = 0;
    ++_rev;
}

uint32_t DisplayToast::lifetime(const Entry &e)
{
    // A banner with two rows shows each for half of it.
    return e.level >= ToastLevel::Warn ? LongMs : ShortMs;
}

void DisplayToast::demoteHead()
{
    // Shown again in full afterwards; as a waiting message it goes behind the waiting errors.
    _shownAt = 0;
    uint8_t errors = 0;
    while (1 + errors < _count && _q[1 + errors].level == ToastLevel::Error)
        ++errors;
    if (errors == 0) return;
    const Entry head = _q[0];
    memmove(&_q[0], &_q[1], errors * sizeof(Entry));
    _q[errors] = head;
    ++_rev;
}

bool DisplayToast::update(Scene scene, uint32_t now)
{
    if (_gone.line1[0] != '\0' && now - _gone.at >= ForgetMs) _gone.line1[0] = '\0'; // also keeps it from the millis() wrap
    // A waiting message that this scene does not show is gone without being shown.
    for (uint8_t i = 1; i < _count;)
        if (act(scene, _q[i].level) == Act::Drop)
        {
            memmove(&_q[i], &_q[i + 1], (_count - i - 1) * sizeof(Entry));
            --_count;
        }
        else
            ++i;

    while (_count != 0)
    {
        const Entry &e = _q[0];
        const bool error = e.level == ToastLevel::Error;
        Act a = act(scene, e.level);

        _form = Form::None;
        if (a == Act::Wait)
        {
            if (!error && _shownAt != 0) demoteHead();
            return false;
        }
        if (a == Act::Wake)
        {
            if (_shownAt == 0 && !e.seen) return true; // wake once, on the first appearance; a seen one shown again does not
            // a seen card stays hidden behind the screensaver and shows once it has ended
            a = (scene == Scene::Dimmed || (scene == Scene::Saver && !error)) ? Act::Show : Act::Drop;
        }

        Form f = Form::None;
        if (a == Act::Show) f = error ? Form::Card : (scene == Scene::Menu ? Form::Banner : Form::Bottom);

        // Time runs from the first appearance, also while hidden; the error card stays until a key closes it.
        if (_shownAt != 0 && !error && now - _shownAt >= lifetime(e))
        {
            pop();
            continue;
        }

        if (a == Act::Drop)
        {
            if (_shownAt == 0 && !(error && e.seen)) // a seen card waits hidden until a key closes it
            {
                pop();
                continue;
            }
            // Shown before: hidden, runs out (a card stays); a waiting error, or for a card a fresh one, does not wait for that.
            if (_count < 2 || _q[1].level != ToastLevel::Error || (error && _q[1].seen)) return false;
            demoteHead();
            continue;
        }
        if (_shownAt == 0) _shownAt = now != 0 ? now : 1;
        _q[0].seen = true;
        _form = f;
        return false;
    }
    _form = Form::None;
    return false;
}

void DisplayToast::draw(Adafruit_SSD1306 &d, uint32_t now) const
{
    if (_form == Form::None || _count == 0) return;
    const Entry &e = _q[0];
    OpenKNX::Ui::begin(d);
    if (_form == Form::Card)
    {
        drawCard(d, e);
        return;
    }
    const uint32_t life = lifetime(e);
    const uint32_t t = now - _shownAt;
    if (_form == Form::Bottom)
        drawBottom(d, e, t, life);
    else
        drawBanner(d, e, t, life);
}

uint32_t DisplayToast::frameKey(uint32_t now) const
{
    if (_form == Form::None || _count == 0) return 0;
    uint32_t key = static_cast<uint32_t>(_form) << 24 | static_cast<uint32_t>(_rev) << 16;
    if (_form == Form::Card) return key;
    // The y the draw step uses, so key and pixels change together, and the banner row (first or second half).
    const uint32_t life = lifetime(_q[0]), t = now - _shownAt;
    if (_form == Form::Bottom) return key | static_cast<uint8_t>(bottomY(t, life));
    return key | static_cast<uint8_t>(bannerY(t, life)) | (t >= life / 2 ? 0x100u : 0u);
}

void DisplayToast::drawBottom(Adafruit_SSD1306 &d, const Entry &e, uint32_t t, uint32_t life) const
{
    const int16_t y = bottomY(t, life);
    d.fillRect(0, y - 1, 128, 64 - y + 1, BLACK);
    d.drawRoundRect(1, y, 126, 20, 3, WHITE);
    d.drawBitmap(5, y + 5, e.icon ? e.icon : levelIcon(e.level), 9, 9, WHITE);

    char n[4];
    countText(e.count, n);
    const size_t w1 = n[0] ? CountChars : BottomChars;
    if (n[0]) printAt(d, static_cast<int16_t>(122 - strlen(n) * 6), y + 2, n, 3);
    Line a, b;
    textRows(e.line1, e.line2, w1, a, b);
    printAt(d, 18, y + 2, a, static_cast<int16_t>(w1));
    printAt(d, 18, y + 11, b, BottomChars);
}

void DisplayToast::drawBanner(Adafruit_SSD1306 &d, const Entry &e, uint32_t t, uint32_t life) const
{
    const int16_t y = bannerY(t, life);
    d.fillRect(0, y, 128, 11, WHITE);
    d.drawBitmap(1, y + 1, e.icon ? e.icon : levelIcon(e.level), 9, 9, BLACK);
    d.setTextColor(BLACK);

    char n[4];
    countText(e.count, n);
    if (n[0]) printAt(d, static_cast<int16_t>(127 - strlen(n) * 6), y + 2, n, 3);
    Line a, b;
    const bool two = bannerRows(e.line1, e.line2, e.count, a, b);
    printAt(d, 13, y + 2, two && t >= life / 2 ? b : a, static_cast<int16_t>(BannerChars - (n[0] ? strlen(n) + 1 : 0)));
    d.setTextColor(WHITE);
}

void DisplayToast::drawCard(Adafruit_SSD1306 &d, const Entry &e) const
{
    d.fillRect(5, 9, 118, 49, BLACK);
    d.drawRoundRect(6, 10, 116, 47, 3, WHITE);
    d.fillCircle(21, 25, 10, BLACK);
    OpenKNX::Ui::badge(d, 21, 25, OpenKNX::Ui::kBadgeError);

    char n[4];
    countText(e.count, n);
    const size_t w1 = CardChars - (n[0] ? strlen(n) + 1 : 0);
    if (n[0]) printAt(d, static_cast<int16_t>(119 - strlen(n) * 6), 15, n, 3);
    Line a, b;
    textRows(e.line1, e.line2, w1, a, b);
    printAt(d, 35, 15, a, static_cast<int16_t>(w1));
    printAt(d, 35, 25, b, CardChars);

    // Without a target OK closes as well, the hint names only the close key.
    int16_t x = 11;
    if (e.open) x = static_cast<int16_t>(x + OpenKNX::Ui::keyHint(d, x, 44, "OK", "Open") + 8);
    OpenKNX::Ui::keyHint(d, x, 44, "<", "Close");
}

bool DisplayToast::closeCard(bool ok)
{
    if (_form != Form::Card || _count == 0) return false;
    void (*open)() = _q[0].open;
    pop();
    _form = Form::None;
    if (ok && open) open();
    return true;
}

void DisplayToast::watchBurnIn(const DisplaySettings &s, uint32_t now)
{
    // Same off-rules as the runtime: dimming needs a time and a level, the screensaver a time and a type.
    const uint8_t guards = static_cast<uint8_t>((s.dimMin != 0 && s.dimLevelIdx != 0 ? 1 : 0) |
                                                (s.screenSaverMin != 0 && s.screenSaverType != SaverOff ? 2 : 0) |
                                                (s.sleepMin != 0 ? 4 : 0));
    if (_guards != 0xFF && (_guards & ~guards) != 0)
        post(ToastLevel::Warn, nullptr, "Screen may burn in", nullptr, nullptr, now);
    _guards = guards;
}

    #ifdef OPENKNX_DISPLAY_TOAST_LOG
void DisplayToast::printLog() const
{
    static const char Levels[] = "iowe";
    for (uint8_t i = 0; i < _logCount; ++i)
    {
        const LogEntry &l = _log[(_logNext + LogSize - _logCount + i) % LogSize];
        logInfo("Toast", "%8lu %c %s | %s", static_cast<unsigned long>(l.ms), Levels[static_cast<uint8_t>(l.level) & 3], l.line1, l.line2);
    }
}
    #endif
#endif // DEVICE_DISPLAY_MODULE
