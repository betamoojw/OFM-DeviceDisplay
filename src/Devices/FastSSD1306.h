#pragma once
/**
 * @file        FastSSD1306.h
 * @brief       Adafruit_SSD1306 with a column-wise text path for the classic font
 * @version     0.0.1
 * @date        2026-11-16
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/

#include <Adafruit_SSD1306.h>

/**
 * @brief Adafruit_SSD1306 whose write() puts each glyph column of the classic 5x7 font into the frame
 *        buffer as whole bytes instead of one GFX call per pixel (writePixel at size 1, writeFillRect above).
 *        Writes the same pixels and cursor as Adafruit_GFX::write(); a custom font, a GFX rotation, a
 *        text height above 3 or a colour other than BLACK/WHITE/INVERSE takes the Adafruit path.
 */
class FastSSD1306 : public Adafruit_SSD1306
{
  public:
    using Adafruit_SSD1306::Adafruit_SSD1306;
    using Adafruit_SSD1306::write;
    size_t write(uint8_t c) override;

  private:
    void drawGlyph(int16_t x, int16_t y, uint8_t c); // classic font, rotation 0, height 1..3
};
