#ifdef DEVICE_DISPLAY_MODULE
#pragma once
/**
 * @file        DisplayUi.h
 * @brief       Shared drawing helpers for every display widget: header, rows, card, bar, keys, graph
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Free functions, no state, no heap. Rules and values: doc/design/rules.md, doc/design/tokens.md.
 * Exit state of every text helper: size 1, colour WHITE transparent, wrap unchanged.
 * Pure drawing helpers (pageDots, badge, bar, graph) leave the text state untouched; printCut uses the current
 * size and colour, printCentred the current colour.
 **/
#include <Adafruit_SSD1306.h>
#include <stdint.h>

namespace OpenKNX::Ui
{
    constexpr int16_t HeaderLine = 10, Row0 = 14, Pitch = 10, PitchDense = 8, CornerW = 12;
    constexpr int16_t CharW = 6, ValueEnd = 2, IconTextX = 12; // font cell, right margin, text after a row icon
    constexpr uint8_t BarH = 10, BarPad = 2, BadgeR = 8, BadgeGap = 10;
    constexpr uint16_t NoValue = 0xFFFF;  // bar(): frame only
    constexpr uint16_t GraphGap = 0xFFFF; // graph reader: no sample at this index

    /** @brief Graph sample reader: 0..1000 permille or GraphGap for index (0 = oldest). */
    typedef uint16_t (*GraphReader)(void *ctx, uint8_t index);
    /** @brief Draws the symbol of a status card into x44..83, y1..27 (x40..79 next to six page dots). */
    typedef void (*CardSymbol)(Adafruit_SSD1306 &d);

    /** @brief Sets text size 1, colour WHITE, no wrap, cp437 off (T7). Does not clear the buffer. */
    void begin(Adafruit_SSD1306 &d);
    /**
     * @brief Title at (0,0), page dots when pages > 1, dotted line at y10; leaves the manager corner free.
     * Pages are capped so the title keeps 4 characters; a longer title ends on "...".
     */
    void header(Adafruit_SSD1306 &d, const char *title, uint8_t pages = 1, uint8_t page = 0);
    /** @brief Prints s at (x,y) in the current size and colour, cut to maxChars with "..." (T5; hard cut below 4). */
    void printCut(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *s, int16_t maxChars);
    /** @brief Prints s centred in the given size, cut to the screen width; leaves size 1. */
    void printCentred(Adafruit_SSD1306 &d, int16_t y, const char *s, uint8_t size = 1);
    /** @brief Title left, "pos/count" right-aligned (menus, no manager corner), dotted line at y10. */
    void headerPos(Adafruit_SSD1306 &d, const char *title, uint16_t pos, uint16_t count);
    /** @brief Page dots (L3): active fillCircle r2, others drawCircle r1; page >= pages marks the last one. */
    void pageDots(Adafruit_SSD1306 &d, uint8_t pages, uint8_t page);
    /**
     * @brief Label at x0, value at column labelCols on row y14 + 10 * row (T4).
     * The label is cut to the space left of the value; the value is never cut.
     */
    void row(Adafruit_SSD1306 &d, uint8_t row, const char *label, const char *value, uint8_t labelCols = 5);
    /** @brief row() at an explicit y, for 8 px data lists. */
    void rowAt(Adafruit_SSD1306 &d, int16_t y, const char *label, const char *value, uint8_t labelCols = 5);
    /**
     * @brief 9x9 icon at (0, rowY - 1), text at x12, value right-aligned ending at x125.
     * The text ends one character before the value; a value wider than the space starts at x12 without text.
     */
    void rowIcon(Adafruit_SSD1306 &d, uint8_t row, const uint8_t *icon9, const char *text, const char *value = nullptr);
    /** @brief rowIcon() at an explicit y, for 8 px data lists. */
    void rowIconAt(Adafruit_SSD1306 &d, int16_t y, const uint8_t *icon9, const char *text, const char *value = nullptr);
    /** @brief Notify card: symbol, badge at (83,24) with a clear ring r10, word size 2 at y33, detail at y54. */
    void statusCard(Adafruit_SSD1306 &d, CardSymbol symbol, const uint8_t *badge13, const char *word, const char *detail);
    /**
     * @brief Value in size 2 at (0,y), unit in size 1 after it at y + 6.
     * If both do not fit, the value is drawn in size 1 on the unit line; neither is cut.
     */
    void bigValue(Adafruit_SSD1306 &d, int16_t y, const char *value, const char *unit);
    /** @brief Filled circle r8 with a 13x13 symbol in BLACK (I3). */
    void badge(Adafruit_SSD1306 &d, int16_t cx, int16_t cy, const uint8_t *symbol13);
    /** @brief Frame 10 px high, fill 2 px inside (G5); permille > 1000 (NoValue) draws the frame only. */
    void bar(Adafruit_SSD1306 &d, int16_t x, int16_t y, int16_t w, uint16_t permille);
    /** @brief Inverse key cap with the key, label after a 3 px gap; returns the total width in px. */
    int16_t keyHint(Adafruit_SSD1306 &d, int16_t x, int16_t y, const char *key, const char *label);
    /**
     * @brief Line graph over h + 1 rows: values on y..y+h-1, baseline x..x+w-1 at y + h.
     * Sample k sits at x + w - 1 - (count - 1 - k) * step (newest right); samples left of x are skipped.
     * GraphGap breaks the line; a column is filled (checkerboard) only under a line segment.
     */
    void graph(Adafruit_SSD1306 &d, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t step, uint8_t count, GraphReader reader, void *ctx);
    /** @brief "On" / "Off", selected "< On >" / "< Off >" (M2). */
    const char *toggleText(bool on, bool selected);
} // namespace OpenKNX::Ui
#endif // DEVICE_DISPLAY_MODULE
