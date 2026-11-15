#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        DisplayUi.cpp
 * @brief       Shared drawing helpers for every display widget (see DisplayUi.h)
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
    #include "DisplayUi.h"
    #include "Icons/UiIcons.h"
    #include <stdio.h>
    #include <string.h>

namespace OpenKNX::Ui
{
    namespace
    {
        constexpr uint8_t TextBuf = 24;  // stack buffer for a cut text
        constexpr uint8_t MinTitle = 4;  // header title characters kept when dots are drawn

        int16_t textLen(const char *s) { return s ? static_cast<int16_t>(strlen(s)) : 0; }

        void textState(Adafruit_SSD1306 &d)
        {
            d.setTextSize(1);
            d.setTextColor(WHITE);
        }

        /**
         * @brief Returns s, or s cut to maxChars in buf (T5): "..." at the end when maxChars >= 4.
         * Returns nullptr when nothing fits.
         */
        const char *fit(char (&buf)[TextBuf], const char *s, int16_t maxChars)
        {
            if (!s || !*s || maxChars <= 0) return nullptr;
            if (textLen(s) <= maxChars) return s;
            if (maxChars > TextBuf - 1) maxChars = TextBuf - 1;
            const bool dots = maxChars >= 4;
            const int16_t keep = dots ? static_cast<int16_t>(maxChars - 3) : maxChars;
            memcpy(buf, s, static_cast<size_t>(keep));
            if (dots) memcpy(buf + keep, "...", 3);
            buf[maxChars] = '\0';
            return buf;
        }

        void printAt(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *s)
        {
            if (!s) return;
            d.setCursor(x, y);
            d.print(s);
        }

        /** @brief Divider at y10, every second pixel; the phase flips each minute against burn-in. */
        void dottedLine(Adafruit_SSD1306 &d)
        {
            const int16_t w = d.width();
            for (int16_t x = static_cast<int16_t>((millis() / 60000UL) & 1UL); x < w; x += 2)
                d.drawPixel(x, HeaderLine, WHITE);
        }

        /** @brief Page count that still leaves MinTitle characters for the header title. */
        uint8_t dotCount(Adafruit_SSD1306 &d, uint8_t pages)
        {
            const int16_t maxPages = static_cast<int16_t>((d.width() - CornerW - 2 - MinTitle * CharW) / 6);
            if (maxPages < 2) return 0;
            return pages > maxPages ? static_cast<uint8_t>(maxPages) : pages;
        }
    } // namespace

    void printCut(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *s, int16_t maxChars)
    {
        char buf[TextBuf];
        printAt(d, x, y, fit(buf, s, maxChars));
    }

    void printCentred(Adafruit_SSD1306 &d, int16_t y, const char *s, uint8_t size)
    {
        const int16_t w = d.width();
        char buf[TextBuf];
        const char *t = fit(buf, s, static_cast<int16_t>((w + size) / (CharW * size)));
        if (!t) return;
        d.setTextSize(size);
        printAt(d, static_cast<int16_t>((w - textLen(t) * CharW * size + size) / 2), y, t);
        d.setTextSize(1);
    }

    void begin(Adafruit_SSD1306 &d)
    {
        textState(d);
        d.setTextWrap(false);
        d.cp437(false);
    }

    void pageDots(Adafruit_SSD1306 &d, uint8_t pages, uint8_t page)
    {
        pages = dotCount(d, pages);
        if (pages < 2) return;
        if (page >= pages) page = static_cast<uint8_t>(pages - 1);
        const int16_t start = static_cast<int16_t>(d.width() - CornerW - pages * 6);
        for (uint8_t i = 0; i < pages; i++)
        {
            const int16_t x = static_cast<int16_t>(start + i * 6 + 2);
            if (i == page)
                d.fillCircle(x, 4, 2, WHITE);
            else
                d.drawCircle(x, 4, 1, WHITE);
        }
    }

    void header(Adafruit_SSD1306 &d, const char *title, uint8_t pages, uint8_t page)
    {
        textState(d);
        // Title ends before the dots, or before the manager corner (L1, L2).
        const uint8_t dots = dotCount(d, pages);
        const int16_t limit = static_cast<int16_t>(d.width() - CornerW - (dots > 1 ? dots * 6 + 2 : 0));
        char buf[TextBuf];
        printAt(d, 0, 0, fit(buf, title, static_cast<int16_t>(limit / CharW)));
        pageDots(d, pages, page);
        dottedLine(d);
    }

    void headerPos(Adafruit_SSD1306 &d, const char *title, uint16_t pos, uint16_t count)
    {
        textState(d);
        char right[12]; // "65535/65535"
        snprintf(right, sizeof(right), "%u/%u", static_cast<unsigned>(pos), static_cast<unsigned>(count));
        const int16_t rightX = static_cast<int16_t>(d.width() - ValueEnd - textLen(right) * CharW);
        char buf[TextBuf];
        printAt(d, 0, 0, fit(buf, title, static_cast<int16_t>(rightX / CharW - 1)));
        printAt(d, rightX, 0, right);
        dottedLine(d);
    }

    void rowAt(Adafruit_SSD1306 &d, int16_t y, const char *label, const char *value, uint8_t labelCols)
    {
        textState(d);
        const int16_t lineChars = static_cast<int16_t>((d.width() - ValueEnd) / CharW);
        char buf[TextBuf];
        if (!value)
        {
            printAt(d, 0, y, fit(buf, label, lineChars));
            return;
        }
        // Value at labelCols, moved left only as far as needed to end at x125; at x0 it drops the label.
        int16_t col = static_cast<int16_t>(lineChars - textLen(value));
        if (col > labelCols) col = labelCols;
        if (col < 0) col = 0;
        printAt(d, 0, y, fit(buf, label, static_cast<int16_t>(col - 1)));
        printAt(d, static_cast<int16_t>(col * CharW), y, value);
    }

    void row(Adafruit_SSD1306 &d, uint8_t row, const char *label, const char *value, uint8_t labelCols)
    {
        rowAt(d, static_cast<int16_t>(Row0 + Pitch * row), label, value, labelCols);
    }

    void rowIconAt(Adafruit_SSD1306 &d, int16_t y, const uint8_t *icon9, const char *text, const char *value)
    {
        textState(d);
        if (icon9) d.drawBitmap(0, static_cast<int16_t>(y - 1), icon9, 9, 9, WHITE);
        const int16_t end = static_cast<int16_t>(d.width() - ValueEnd);
        char buf[TextBuf];
        if (!value)
        {
            printAt(d, IconTextX, y, fit(buf, text, static_cast<int16_t>((end - IconTextX) / CharW)));
            return;
        }
        const int16_t vx = static_cast<int16_t>(end - textLen(value) * CharW);
        if (vx < IconTextX)
        {
            printAt(d, IconTextX, y, value); // value wider than the space: no text, never over the icon
            return;
        }
        printAt(d, IconTextX, y, fit(buf, text, static_cast<int16_t>((vx - IconTextX) / CharW - 1)));
        printAt(d, vx, y, value);
    }

    void rowIcon(Adafruit_SSD1306 &d, uint8_t row, const uint8_t *icon9, const char *text, const char *value)
    {
        rowIconAt(d, static_cast<int16_t>(Row0 + Pitch * row), icon9, text, value);
    }

    void badge(Adafruit_SSD1306 &d, int16_t cx, int16_t cy, const uint8_t *symbol13)
    {
        d.fillCircle(cx, cy, BadgeR, WHITE);
        if (symbol13) d.drawBitmap(static_cast<int16_t>(cx - 6), static_cast<int16_t>(cy - 6), symbol13, 13, 13, BLACK);
    }

    void statusCard(Adafruit_SSD1306 &d, CardSymbol symbol, const uint8_t *badge13, const char *word, const char *detail)
    {
        if (symbol) symbol(d);
        if (badge13)
        {
            d.fillCircle(83, 24, BadgeGap, BLACK);
            badge(d, 83, 24, badge13);
        }
        textState(d);
        printCentred(d, 33, word, 2);
        printCentred(d, 54, detail, 1);
    }

    void bigValue(Adafruit_SSD1306 &d, int16_t y, const char *value, const char *unit)
    {
        textState(d);
        const int16_t vlen = textLen(value);
        const int16_t unitW = unit ? static_cast<int16_t>(2 + textLen(unit) * CharW) : 0;
        const uint8_t size = vlen * CharW * 2 + unitW <= d.width() ? 2 : 1;
        d.setTextSize(size);
        printAt(d, 0, size == 2 ? y : static_cast<int16_t>(y + 6), value);
        d.setTextSize(1);
        printAt(d, static_cast<int16_t>(vlen * CharW * size + 2), static_cast<int16_t>(y + 6), unit);
    }

    void bar(Adafruit_SSD1306 &d, int16_t x, int16_t y, int16_t w, uint16_t permille)
    {
        d.drawRect(x, y, w, BarH, WHITE);
        if (w <= 2 * BarPad || permille > 1000) return;
        const int16_t fill = static_cast<int16_t>(static_cast<int32_t>(w - 2 * BarPad) * permille / 1000);
        if (fill > 0) d.fillRect(static_cast<int16_t>(x + BarPad), static_cast<int16_t>(y + BarPad), fill, BarH - 2 * BarPad, WHITE);
    }

    int16_t keyHint(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *key, const char *label)
    {
        d.setTextSize(1);
        const int16_t kw = static_cast<int16_t>(textLen(key) * CharW + 3);
        d.fillRoundRect(x, y, kw, 9, 2, WHITE);
        d.setTextColor(BLACK);
        printAt(d, static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 1), key);
        d.setTextColor(WHITE);
        if (!label) return kw;
        printAt(d, static_cast<int16_t>(x + kw + 3), static_cast<int16_t>(y + 1), label);
        return static_cast<int16_t>(kw + 3 + textLen(label) * CharW);
    }

    void graph(Adafruit_SSD1306 &d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t step, uint8_t count, GraphReader reader, void *ctx)
    {
        if (w <= 0) return;
        d.drawFastHLine(x, static_cast<int16_t>(y + h), w, WHITE);
        if (!reader || count == 0 || h < 1) return;
        if (step == 0) step = 1;
        const int32_t right = x + w - 1, bottom = y + h - 1;
        const int32_t fits = (w - 1) / step; // older samples that still land at or right of x
        const uint8_t first = count - 1 > fits ? static_cast<uint8_t>(count - 1 - fits) : 0;
        bool linked = false; // previous sample drawn, so this one closes a segment
        int16_t prevX = 0, prevY = 0;
        for (uint16_t k = first; k < count; k++)
        {
            uint16_t v = reader(ctx, static_cast<uint8_t>(k));
            if (v == GraphGap)
            {
                linked = false;
                continue;
            }
            if (v > 1000) v = 1000;
            const int16_t px = static_cast<int16_t>(right - static_cast<int32_t>(count - 1 - k) * step);
            const int16_t py = static_cast<int16_t>(bottom - static_cast<int32_t>(v) * (h - 1) / 1000);
            if (linked)
            {
                // Segment columns at the new level, checkerboard below it (handbook.js p-bus).
                for (int16_t cx = prevX; cx <= px; cx++)
                    for (int16_t cy = static_cast<int16_t>(py + ((cx + py) & 1)); cy <= bottom; cy += 2)
                        d.drawPixel(cx, cy, WHITE);
                d.drawLine(prevX, prevY, px, py, WHITE);
            }
            else
                d.drawPixel(px, py, WHITE);
            linked = true;
            prevX = px;
            prevY = py;
        }
    }

    const char *toggleText(bool on, bool selected)
    {
        static const char kOn[] = "On", kOff[] = "Off", kOnSel[] = "< On >", kOffSel[] = "< Off >";
        if (selected) return on ? kOnSel : kOffSel;
        return on ? kOn : kOff;
    }
} // namespace OpenKNX::Ui
#endif // DEVICE_DISPLAY_MODULE
