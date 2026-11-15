#pragma once
/**
 * @file        BootLogoAnim.h
 * @brief       Animated boot logo of OpenKNX with product credits
 * @version     0.0.1
 * @date        2026-11-16
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/

#ifdef DEVICE_DISPLAY_BOOT_ANIM // animated boot logo, built only for products that set this flag
#include <Adafruit_SSD1306.h>
#include <stdint.h>

/** @brief Build date "YYYY-MM-DD" from buildtime.h, nullptr when the product has none. */
const char *bootLogoBuildDate();

/**
 * @brief Ping-pong between the two KNX squares of the 128x64 OpenKNX logo, the logo grows from the bus line and
 *        stands, then rolls up while the product name, version and an optional author are typed in.
 *        Each frame is drawn from a page-format copy of the logo (1 KB heap, freed after the last frame).
 */
class BootLogoAnim
{
  public:
    static constexpr uint32_t LOGO_MS = 2700;    // ping, pong, logo grows
    static constexpr uint32_t HOLD_MS = 2000;    // full logo
    static constexpr uint32_t CREDITS_MS = 2200; // logo rolls up, texts typed
    static constexpr uint32_t END_MS = 1500;     // last frame stands
    static constexpr uint32_t TOTAL_MS = LOGO_MS + HOLD_MS + CREDITS_MS + END_MS;

    BootLogoAnim() = default;
    BootLogoAnim(const BootLogoAnim &) = delete;
    BootLogoAnim &operator=(const BootLogoAnim &) = delete;
    ~BootLogoAnim();

    /**
     * @brief Takes the 128x64 logo (row-major, MSB left, as drawBitmap) and the texts; product "Name (Dev)" puts
     *        "Dev" behind the version, the date follows it. version/date/author may be nullptr. UTF-8 Latin-1
     *        letters print in CP437. False for a missing or other-sized logo, a failed malloc or a second call.
     *        Without a version the "(Dev)" suffix is left out as well.
     */
    bool begin(const uint8_t *logo, uint16_t width, uint16_t height, const char *product, const char *version, const char *date,
               const char *author);

    /**
     * @brief The first call starts the clock; then one logo page is converted per call, then the frame for the
     *        time since the first call is drawn into the GFX buffer, at most one per 20 ms.
     * @return true when the GFX buffer changed (blanked by the first call, or a new frame); the caller hands it to the panel
     */
    bool tick(Adafruit_SSD1306 &d, uint32_t now);

    /** @brief After the last frame: the next tick() draws it once more (another widget had the panel). */
    void redraw() { _redrawLast = _done; }

  private:
    struct Line
    {
        char text[24];
        uint8_t size;
        int16_t x, y;
    };

    const uint8_t *_logo = nullptr;
    uint8_t *_pages = nullptr; // logo in SSD1306 page format
    uint8_t _converted = 0;    // pages converted so far (8 = ready)
    bool _started = false;     // _start holds the first tick
    bool _done = false;        // last frame drawn, _pages freed
    bool _redrawLast = false;  // redraw() asked for the last frame once more
    uint32_t _start = 0;
    uint32_t _lastFrame = UINT32_MAX;
    Line _lines[4];
    uint8_t _lineCount = 0;
    uint8_t _bigCount = 0; // leading name lines, typed char by char

    void layout(const char *product, const char *version, const char *date, const char *author);
    void logoRect(uint8_t *buf, int16_t x0, int16_t x1, int16_t y0, int16_t y1) const;
    int16_t ping(Adafruit_SSD1306 &d, uint32_t t, bool forward) const;
    void drawLogo(Adafruit_SSD1306 &d, uint32_t t) const;
    void drawCredits(Adafruit_SSD1306 &d, uint32_t t) const;
};

#endif // DEVICE_DISPLAY_BOOT_ANIM
