/**
 * @file        BootLogoAnim.cpp
 * @brief       Animated boot logo of OpenKNX with product credits
 * @version     0.0.1
 * @date        2026-11-16
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
#include "BootLogoAnim.h"
#ifdef DEVICE_DISPLAY_BOOT_ANIM

#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace
{
    constexpr int16_t W = 128, H = 64;

    struct Box
    {
        int16_t x, y, w, h;
    };
    // Logo geometry: the two hatched KNX squares, their x-shaped connectors and the bus line (rows 29..31).
    constexpr Box SQ_L{13, 36, 20, 19}, SQ_R{95, 6, 20, 19};
    constexpr Box CN_L{21, 32, 3, 3}, CN_R{103, 26, 3, 3};
    constexpr int16_t RAIL_LEFT = 22, RAIL_RIGHT = 104;

    /** @brief Bus line pattern (rows 29..31 = page 3 bits 5..7: even x rows 29+31, odd x row 30), also where the "p" crosses it. */
    void railPattern(uint8_t *buf, int16_t x0, int16_t x1)
    {
        for (int16_t x = x0 < 0 ? 0 : x0; x <= x1 && x < W; x++) buf[3 * W + x] |= (x & 1) ? 0x40 : 0xA0;
    }

    // Ping path: up from the left connector to the bus line, along it, up into the right connector.
    constexpr int16_t PATH[4][2] = {{22, 35}, {22, 30}, {104, 30}, {104, 25}};
    constexpr float PATH_LEN = 5 + 82 + 5;

    inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    inline float easeOut(float v)
    {
        const float r = 1 - clamp01(v);
        return 1 - r * r * r;
    }

    void pathAt(float dist, int16_t &x, int16_t &y)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            const int16_t x0 = PATH[i][0], y0 = PATH[i][1], x1 = PATH[i + 1][0], y1 = PATH[i + 1][1];
            const float len = abs(x1 - x0) + abs(y1 - y0);
            if (dist <= len || i == 2)
            {
                const float k = clamp01(dist / len);
                x = static_cast<int16_t>(lroundf(x0 + (x1 - x0) * k));
                y = static_cast<int16_t>(lroundf(y0 + (y1 - y0) * k));
                return;
            }
            dist -= len;
        }
    }

    /** @brief CP437 code for U+00C0..U+00FF; letters CP437 lacks fall back to the base letter. */
    const uint8_t LATIN1_CP437[64] = {
        'A', 'A', 'A', 'A', 0x8E, 0x8F, 0x92, 0x80, 'E', 0x90, 'E', 'E', 'I', 'I', 'I', 'I',   // C0..CF
        'D', 0xA5, 'O', 'O', 'O', 'O', 0x99, 'x', 'O', 'U', 'U', 'U', 0x9A, 'Y', 'T', 0xE1,    // D0..DF
        0x85, 0xA0, 0x83, 'a', 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B, // E0..EF
        'd', 0xA4, 0x95, 0xA2, 0x93, 'o', 0x94, 0xF6, 'o', 0x97, 0xA3, 0x96, 0x81, 'y', 't', 0x98};    // F0..FF

    /** @brief UTF-8 to CP437 for ASCII and Latin-1 letters; anything else becomes '?'. */
    void toCp437(const char *in, char *out, size_t size)
    {
        size_t n = 0;
        const uint8_t *p = reinterpret_cast<const uint8_t *>(in);
        while (*p && n + 1 < size)
        {
            uint8_t c = *p++;
            if (c >= 0x80)
            {
                if ((c == 0xC3) && (*p & 0xC0) == 0x80)
                    c = LATIN1_CP437[(*p++ & 0x3F)]; // U+00C0..U+00FF
                else
                {
                    while ((*p & 0xC0) == 0x80) p++; // skip the continuation bytes
                    c = '?';
                }
            }
            out[n++] = static_cast<char>(c);
        }
        out[n] = '\0';
    }

    inline int16_t centered(size_t len, uint8_t size) { return (W - (static_cast<int16_t>(len) * 6 * size - size)) >> 1; }
    inline int16_t lower(int16_t a, int16_t b) { return a < b ? a : b; }
    inline int16_t upper(int16_t a, int16_t b) { return a > b ? a : b; }
} // namespace

BootLogoAnim::~BootLogoAnim() { free(_pages); }

bool BootLogoAnim::begin(const uint8_t *logo, uint16_t width, uint16_t height, const char *product, const char *version, const char *date,
                          const char *author)
{
    if (_logo || !logo || width != W || height != H) return false; // _logo: begin() ran before
    _pages = static_cast<uint8_t *>(malloc(W * H / 8));
    if (!_pages) return false;
    _logo = logo;
    layout(product, version, date, author);
    return true;
}

void BootLogoAnim::layout(const char *product, const char *version, const char *date, const char *author)
{
    // "IP-Interface (Dev)" -> name "IP-Interface", version "<version> Dev"
    char name[48], suffix[12] = ""; // room for the suffix behind a name that is cut to one line later
    toCp437(product ? product : "", name, sizeof(name));
    char *open = strstr(name, " (");
    const size_t nameLen = strlen(name);
    if (open && nameLen && name[nameLen - 1] == ')')
    {
        name[nameLen - 1] = '\0';
        strncpy(suffix, open + 2, sizeof(suffix) - 1);
        *open = '\0';
    }

    // Name in size 2. Longer than 10 chars: two size-2 lines, cut after the first '-' (without a '-' at the first
    // space) when both parts have at most 10 chars; else one size-1 line of at most 21 chars.
    const size_t len = strlen(name);
    _lineCount = 0;
    uint8_t big = 0;
    if (len <= 10)
    {
        strcpy(_lines[0].text, name);
        _lines[0].size = 2;
        big = 1;
    }
    else
    {
        const char *cut = strchr(name, '-');
        size_t first = cut ? static_cast<size_t>(cut - name) + 1 : 0;
        if (!cut)
        {
            const char *sp = strchr(name, ' ');
            first = sp ? static_cast<size_t>(sp - name) : 0;
        }
        const char *rest = name + first + (name[first] == ' ' ? 1 : 0);
        if (first && first <= 10 && strlen(rest) <= 10)
        {
            memcpy(_lines[0].text, name, first);
            _lines[0].text[first] = '\0';
            strcpy(_lines[1].text, rest);
            _lines[0].size = _lines[1].size = 2;
            big = 2;
        }
        else
        {
            const size_t keep = len < 21 ? len : 21; // 21 chars fill the 128 px line
            memcpy(_lines[0].text, name, keep);
            _lines[0].text[keep] = '\0';
            _lines[0].size = 1;
            big = 1;
        }
    }
    _lineCount = _bigCount = big;

    // "2.5.0 Dev 2026-11-16": version, the suffix of the name (only with a version), build date; the date only while
    // the line fits (21 chars)
    char info[24] = "";
    for (const char *part : {version, static_cast<const char *>(version && *version ? suffix : nullptr), date})
    {
        const size_t used = strlen(info);
        if (!part || !*part || (part == date && used + 1 + strlen(part) > 21)) continue;
        snprintf(info + used, sizeof(info) - used, used ? " %s" : "%s", part);
    }
    if (*info)
    {
        Line &l = _lines[_lineCount++];
        strcpy(l.text, info);
        l.size = 1;
    }
    if (author && *author)
    {
        Line &l = _lines[_lineCount++];
        char cp[19]; // "by " + 18 chars fill the 128 px line
        toCp437(author, cp, sizeof(cp));
        snprintf(l.text, sizeof(l.text), "by %s", cp);
        l.size = 1;
    }

    // Vertical centring: name lines 2 px apart, 6 px gap, small lines 3 px apart.
    const uint8_t small = _lineCount - big;
    int16_t h = 2 * (big - 1) + (small ? 6 + 3 * (small - 1) : 0);
    for (uint8_t i = 0; i < _lineCount; i++) h += 8 * _lines[i].size;
    int16_t y = (H - h) >> 1;
    size_t longest = 0;
    for (uint8_t i = 0; i < big; i++)
        if (strlen(_lines[i].text) > longest) longest = strlen(_lines[i].text);
    for (uint8_t i = 0; i < _lineCount; i++)
    {
        Line &l = _lines[i];
        l.x = i < big ? centered(longest, l.size) : centered(strlen(l.text), 1); // split name lines share one left edge
        l.y = y;
        y += 8 * l.size + (i + 1 < big ? 2 : (i + 1 == big ? 6 : 3));
    }
}

void BootLogoAnim::logoRect(uint8_t *buf, int16_t x0, int16_t x1, int16_t y0, int16_t y1) const
{
    x0 = upper(x0, 0), x1 = lower(x1, W - 1), y0 = upper(y0, 0), y1 = lower(y1, H - 1);
    if (x0 > x1 || y0 > y1) return;
    for (int16_t p = y0 >> 3; p <= (y1 >> 3); p++)
    {
        const int16_t lo = upper(y0 - p * 8, 0), hi = lower(y1 - p * 8, 7);
        const uint8_t mask = static_cast<uint8_t>((0xFF << lo) & (0xFF >> (7 - hi)));
        const uint8_t *src = _pages + p * W;
        uint8_t *dst = buf + p * W;
        for (int16_t x = x0; x <= x1; x++) dst[x] |= src[x] & mask;
    }
}

int16_t BootLogoAnim::ping(Adafruit_SSD1306 &d, uint32_t t, bool forward) const
{
    // 0..250 ms rings around the sender, 250..950 ms travel, 950..1150 ms the receiver flashes
    const Box &from = forward ? SQ_L : SQ_R, &to = forward ? SQ_R : SQ_L;
    if (t < 250)
    {
        const int16_t g = 2 + 2 * static_cast<int16_t>(t / 125);
        d.drawRect(from.x - g, from.y - g, from.w + 2 * g, from.h + 2 * g, SSD1306_WHITE);
        return -1;
    }
    const float dist = PATH_LEN * clamp01((t - 250) / 700.0f);
    int16_t x, y;
    pathAt(forward ? dist : PATH_LEN - dist, x, y);
    d.fillRect(x - 1, y - 1, 3, 3, SSD1306_WHITE);
    if (t >= 950 && t < 1150 && ((t - 950) / 100) % 2 == 0) d.fillRect(to.x, to.y, to.w, to.h, SSD1306_INVERSE);
    return x;
}

void BootLogoAnim::drawLogo(Adafruit_SSD1306 &d, uint32_t t) const
{
    uint8_t *buf = d.getBuffer();
    for (const Box &b : {SQ_L, SQ_R, CN_L, CN_R}) logoRect(buf, b.x, b.x + b.w - 1, b.y, b.y + b.h - 1);
    if (t < 1150)
    {
        const int16_t x = ping(d, t, true);
        if (x >= 0) railPattern(buf, RAIL_LEFT, x); // the ping lays the bus line
        return;
    }
    if (t < 2300)
    {
        railPattern(buf, RAIL_LEFT, RAIL_RIGHT);
        ping(d, t - 1150 + 250, false); // acknowledge back, without the rings
        return;
    }
    logoRect(buf, RAIL_LEFT, RAIL_RIGHT, 29, 31); // the line as drawn in the logo: the "p" crosses it in full
    const float r = 34 * easeOut((t - 2300) / 400.0f); // the logo grows from the bus line
    logoRect(buf, 0, W - 1, static_cast<int16_t>(ceilf(30 - r)), static_cast<int16_t>(floorf(30 + r)));
}

void BootLogoAnim::drawCredits(Adafruit_SSD1306 &d, uint32_t t) const
{
    // The logo rolls up, then the name is typed (70 ms per char), version and author follow.
    uint8_t *buf = d.getBuffer();
    const int16_t up = static_cast<int16_t>(lroundf(H * easeOut(t / 500.0f)));
    if (up < H)
        for (int16_t x = 0; x < W; x++)
        {
            uint64_t col = 0;
            for (uint8_t p = 0; p < 8; p++) col |= static_cast<uint64_t>(_pages[p * W + x]) << (8 * p);
            col >>= up;
            for (uint8_t p = 0; p < 8; p++) buf[p * W + x] = static_cast<uint8_t>(col >> (8 * p));
        }
    if (t < 450) return;

    int32_t n = t >= 550 ? static_cast<int32_t>((t - 550) / 70) : -1; // chars typed so far
    d.setTextWrap(false);
    d.cp437(true);
    d.setTextColor(SSD1306_WHITE);
    for (uint8_t i = 0; i < _lineCount; i++)
    {
        const Line &l = _lines[i];
        const int32_t len = static_cast<int32_t>(strlen(l.text));
        d.setTextSize(l.size);
        d.setCursor(l.x, l.y);
        if (i < _bigCount)
        {
            for (int32_t k = 0; k < len && k < n; k++) d.write(static_cast<uint8_t>(l.text[k]));
            n -= len;
        }
        else if (n > 2 || t + 300 >= CREDITS_MS) // after the name, at the latest 300 ms before the end
            d.print(l.text);
    }
    d.setTextSize(1);
    d.cp437(false);
}

bool BootLogoAnim::tick(Adafruit_SSD1306 &d, uint32_t now)
{
    if (_done)
    {
        if (!_redrawLast) return false;
        _redrawLast = false; // back after another widget: the last frame again (texts only, the logo copy is gone)
        memset(d.getBuffer(), 0, W * H / 8);
        drawCredits(d, CREDITS_MS);
        return true;
    }
    if (!_pages) return false;
    if (!_started)
    {
        _start = now; // the timeline starts at the first call
        _started = true;
    }
    if (_converted < 8)
    {
        // One page per call: the logo rows p*8..p*8+7 into SSD1306 column bytes.
        const uint8_t p = _converted++;
        if (p == 0) memset(d.getBuffer(), 0, W * H / 8); // black while the copy is built
        for (int16_t x = 0; x < W; x++)
        {
            uint8_t v = 0;
            for (uint8_t b = 0; b < 8; b++)
                if (_logo[(p * 8 + b) * (W / 8) + (x >> 3)] & (0x80 >> (x & 7))) v |= 1 << b;
            _pages[p * W + x] = v;
        }
        return p == 0; // the first call blanked the screen
    }

    uint32_t t = now - _start;
    const uint32_t frame = t / 20; // at most 50 frames per second
    if (frame == _lastFrame) return false;
    _lastFrame = frame;

    constexpr uint32_t CREDITS_AT = LOGO_MS + HOLD_MS;
    const bool last = t >= CREDITS_AT + CREDITS_MS;
    if (last) t = CREDITS_AT + CREDITS_MS;
    memset(d.getBuffer(), 0, W * H / 8);
    if (t < LOGO_MS)
        drawLogo(d, t);
    else if (t < CREDITS_AT)
        memcpy(d.getBuffer(), _pages, W * H / 8);
    else
        drawCredits(d, t - CREDITS_AT);
    if (last)
    {
        _done = true;
        free(_pages);
        _pages = nullptr;
    }
    return true;
}

#endif // DEVICE_DISPLAY_BOOT_ANIM
