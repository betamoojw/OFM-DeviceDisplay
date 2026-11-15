#ifdef DEVICE_DISPLAY_MODULE
#pragma once
/**
 * @file        Toast.h
 * @brief       Short on-screen messages: queue, visibility rules and the three forms (bottom toast, banner, card)
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Fixed RAM, no heap: texts are copied into fixed fields. Drawn as an overlay into the frame the panel gets,
 * so no widget knows about it. Rules and pixel values: doc/design/index.html, tab Toast.
 **/
#include <Adafruit_SSD1306.h>
#include <stdint.h>

struct DisplaySettings;

/** @brief Severity of a toast; decides form, time, waking and queue order. */
enum class ToastLevel : uint8_t
{
    Info = 0,
    Ok = 1,
    Warn = 2,
    Error = 3
};

class DisplayToast
{
  public:
    /** @brief What the display shows right now, from the most to the least blocking. */
    enum class Scene : uint8_t
    {
        Ota,      // firmware update
        Prog,     // KNX programming mode
        Off,      // panel switched off by the user (gesture): no toast wakes it
        Dark,     // sleep, off by timeout
        Busy,     // menu editor, About, gesture or other status widget, boot logo
        Saver,    // screensaver
        Dimmed,   // dimmed panel
        Menu,     // menu open
        Rotation  // widget rotation
    };
    enum class Form : uint8_t
    {
        None,
        Bottom,
        Banner,
        Card
    };

    static constexpr uint8_t LineLen = 21;          // characters per line field
    static constexpr uint8_t Slots = 4;             // one current + three waiting
    static constexpr uint32_t ShortMs = 2500;       // Info, Ok
    static constexpr uint32_t LongMs = 5000;        // Warn
    static constexpr uint32_t SlideMs = 300;        // slide in and out
    static constexpr uint32_t MergeMs = 10000;      // same line1 within this time counts up
    static constexpr uint32_t ForgetMs = 60000;     // a retracted line1 posted again within this time comes back seen
    static constexpr uint8_t BottomChars = 17;      // text width of the bottom toast (104 px)
    static constexpr uint8_t NarrowChars = 14;      // narrowest row a second line gets: banner row beside "xNN", card
    static constexpr uint8_t SaverOff = 10;         // ScreenSaverType::Off, checked in DeviceDisplay.cpp

    /**
     * @brief Queues a message. Lines are copied (21 characters); nullptr icon uses the level icon.
     * @param open optional target for OK on an error card; nullptr closes only.
     */
    void post(ToastLevel level, const uint8_t *icon9, const char *line1, const char *line2, void (*open)(), uint32_t now);
    /** @brief Queues text split at the first '\n'; a second line wider than the toast keeps its end after "...". */
    void postText(ToastLevel level, const uint8_t *icon9, const char *text, uint32_t now);
    /**
     * @brief Removes the message with this line1, on screen or waiting (its cause is gone). Returns false when none.
     *        It is remembered for ForgetMs: posted again in that time it counts on and keeps its seen state (no new
     *        wake for a seen one), unless at a higher level.
     */
    bool retract(const char *line1, uint32_t now);

    /**
     * @brief Advances the head of the queue for the current scene and sets the form to draw.
     * @return true when the caller has to wake the panel and call update() again (Warn/Error on a dimmed or dark panel or over the screensaver)
     */
    bool update(Scene scene, uint32_t now);
    /** @brief Draws the current form into d (the frame for the panel). */
    void draw(Adafruit_SSD1306 &d, uint32_t now) const;
    /** @brief Changes whenever the drawn toast changes (form, drawn y, banner row, content); 0 = nothing drawn. */
    uint32_t frameKey(uint32_t now) const;

    Form form() const { return _form; }
    /** @brief Nothing queued and nothing remembered: update() would change nothing. */
    bool idle() const { return _count == 0 && _gone.line1[0] == '\0'; }
    /** @brief Notes a key PRESS (bit = key mask): OK or LEFT pressed while the card is shown arms its release. */
    void pressKey(uint8_t bit, bool okOrLeft) { _keys = static_cast<uint8_t>(_form == Form::Card && okOrLeft ? _keys | bit : _keys & ~bit); }
    /** @brief Takes the armed state of a released key. */
    bool releaseKey(uint8_t bit)
    {
        const bool armed = (_keys & bit) != 0;
        _keys = static_cast<uint8_t>(_keys & ~bit);
        return armed;
    }
    void clearKeys() { _keys = 0; }
    /** @brief Key bits pressed on the shown card and not released yet. */
    uint8_t armedKeys() const { return _keys; }
    /** @brief Closes the error card on screen; runs its open target for OK. Returns false when no card is shown. */
    bool closeCard(bool ok);
    /** @brief Posts one Warn when dimming, screensaver or sleep went off since the last call; the first call takes the baseline. */
    void watchBurnIn(const DisplaySettings &s, uint32_t now);

#ifdef OPENKNX_DISPLAY_TOAST_LOG
    /** @brief Prints the last posted messages to the console log, oldest first. */
    void printLog() const;
#endif

  private:
    struct Entry
    {
        char line1[LineLen + 1];
        char line2[LineLen + 1];
        const uint8_t *icon;
        void (*open)();
        uint32_t stamp; // millis() of the last post (merge window)
        ToastLevel level;
        uint8_t count; // posts merged into this entry, shown from 2 on
        bool seen;     // was on screen (kept by a merge and a retract): a fresh error goes first, only a fresh one wakes
    };

    Entry _q[Slots];
    struct Gone // the last retracted entry, line1[0] == 0 = none
    {
        uint32_t at; // millis() of the retract
        char line1[LineLen + 1];
        ToastLevel level;
        uint8_t count;
        bool seen;
    };
    Gone _gone{};
    uint8_t _count = 0;
    uint32_t _shownAt = 0; // millis() when the head appeared, 0 = not shown yet
    Form _form = Form::None;
    uint8_t _guards = 0xFF; // burn-in guards seen last, 0xFF = no baseline yet
    uint8_t _keys = 0;      // key bits pressed while the card was shown
    uint8_t _rev = 0;       // counts changes of the queue content

    void insert(const Entry &e);
    void pop();
    void demoteHead();
    static uint32_t lifetime(const Entry &e);
    void drawBottom(Adafruit_SSD1306 &d, const Entry &e, uint32_t t, uint32_t life) const;
    void drawBanner(Adafruit_SSD1306 &d, const Entry &e, uint32_t t, uint32_t life) const;
    void drawCard(Adafruit_SSD1306 &d, const Entry &e) const;

#ifdef OPENKNX_DISPLAY_TOAST_LOG
    static constexpr uint8_t LogSize = 8;
    struct LogEntry
    {
        uint32_t ms;
        ToastLevel level;
        char line1[LineLen + 1];
        char line2[LineLen + 1];
    };
    LogEntry _log[LogSize];
    uint8_t _logNext = 0, _logCount = 0;
#endif
};
#endif // DEVICE_DISPLAY_MODULE
