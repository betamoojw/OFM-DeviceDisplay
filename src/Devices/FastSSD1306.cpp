/**
 * @file        FastSSD1306.cpp
 * @brief       Adafruit_SSD1306 with a column-wise text path for the classic font
 * @version     0.0.1
 * @date        2026-11-16
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
#include "FastSSD1306.h"

#include "glcdfont.c" // classic font table of Adafruit_GFX; file-static there, so this TU holds its own copy

namespace
{
    /** @brief One colour on the given bits of a buffer byte, as Adafruit_SSD1306::drawPixel does per pixel. */
    inline void paint(uint8_t &b, uint8_t bits, uint16_t color)
    {
        if (color == SSD1306_WHITE)
            b |= bits;
        else if (color == SSD1306_BLACK)
            b &= ~bits;
        else
            b ^= bits; // SSD1306_INVERSE
    }

    /** @brief Glyph column with each font row repeated sy times; bit n is row n below the glyph top. */
    inline uint32_t stretch(uint8_t line, uint8_t sy)
    {
        if (sy == 1) return line;
        const uint32_t unit = (1u << sy) - 1;
        uint32_t out = 0;
        for (uint8_t j = 0; line; j++, line >>= 1)
            if (line & 1) out |= unit << (j * sy);
        return out;
    }
} // namespace

size_t FastSSD1306::write(uint8_t c)
{
    if (gfxFont || rotation || textsize_y > 3 || textcolor > SSD1306_INVERSE || textbgcolor > SSD1306_INVERSE)
        return Adafruit_SSD1306::write(c);

    // Cursor handling of Adafruit_GFX::write() for the classic font
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y += textsize_y * 8;
    }
    else if (c != '\r')
    {
        if (wrap && ((cursor_x + textsize_x * 6) > _width))
        {
            cursor_x = 0;
            cursor_y += textsize_y * 8;
        }
        drawGlyph(cursor_x, cursor_y, c);
        cursor_x += textsize_x * 6;
    }
    return 1;
}

void FastSSD1306::drawGlyph(int16_t x, int16_t y, uint8_t c)
{
    const uint8_t sx = textsize_x, sy = textsize_y;
    if ((x >= _width) || (y >= _height) || ((x + 6 * sx - 1) < 0) || ((y + 8 * sy - 1) < 0)) return;
    if (!_cp437 && (c >= 176)) c++; // classic charset offset of Adafruit_GFX::drawChar

    const bool opaque = textbgcolor != textcolor; // opaque also fills the background and the 6th column
    const uint8_t rows = 8 * sy;                    // at most 24: shifted by up to 7 it fits 32 bits
    const uint32_t cover = (1u << rows) - 1;
    const int16_t pages = (HEIGHT + 7) / 8;
    const int16_t pageFirst = y < 0 ? 0 : y / 8;
    int16_t pageLast = (y + rows - 1) / 8; // not negative after the clip test
    if (pageLast >= pages) pageLast = pages - 1;
    const uint8_t lastPageRows = (HEIGHT & 7) ? (1u << (HEIGHT & 7)) - 1 : 0xFF;

    for (uint8_t i = 0; i < (opaque ? 6 : 5); i++)
    {
        const uint32_t fg = i < 5 ? stretch(pgm_read_byte(&font[c * 5 + i]), sy) : 0;
        const uint32_t bg = opaque ? cover & ~fg : 0;
        for (uint8_t k = 0; k < sx; k++)
        {
            const int16_t col = x + i * sx + k;
            if (col < 0) continue;
            if (col >= WIDTH) return;
            uint8_t *p = buffer + col + pageFirst * WIDTH;
            for (int16_t pg = pageFirst; pg <= pageLast; pg++, p += WIDTH)
            {
                const int16_t shift = pg * 8 - y; // glyph row that lands on bit 0 of this page
                uint8_t f = shift >= 0 ? fg >> shift : fg << -shift;
                uint8_t b = shift >= 0 ? bg >> shift : bg << -shift;
                if (pg == pages - 1)
                {
                    f &= lastPageRows;
                    b &= lastPageRows;
                }
                if (f) paint(*p, f, textcolor);
                if (b) paint(*p, b, textbgcolor);
            }
        }
    }
}
