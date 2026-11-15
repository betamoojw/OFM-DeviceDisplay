#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        WidgetMyModule.cpp
 * @brief       Template for a display widget: card, values, history (rename MyModule, replace the values)
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
#include "WidgetMyModule.h"
#include "DisplayUi.h"
#include "Icons/UiIcons.h"
#include "OpenKNX.h"
#include <stdio.h>

namespace Ui = OpenKNX::Ui;

// ---- Values: replace with the getters of your module. The widget keeps no copies and no buffers. ----
static bool moduleOk() { return true; }
static int16_t temperatureTenths() { return 215; }  // 21.5 C
static uint8_t humidityPercent() { return 48; }
static uint8_t historyCount() { return 60; }         // samples the module keeps, one per minute
/** @brief Graph reader: sample i (0 = oldest) as 0..1000 permille, Ui::GraphGap for "no value". */
static uint16_t historyAt(void *, uint8_t i) { return (uint16_t)(400 + (i * 37) % 300); }
// -----------------------------------------------------------------------------------------------------

/** @brief Tenths as "-0.5": the sign also below one degree, where t / 10 is 0. */
static const char *tenthsText(char *b, size_t n, int16_t t)
{
    const unsigned a = t < 0 ? (unsigned)-t : (unsigned)t;
    snprintf(b, n, "%s%u.%u", t < 0 ? "-" : "", a / 10, a % 10);
    return b;
}

/** @brief Card symbol, drawn into x44..83 y1..27 (a 40x27 field above the word). */
static void drawSymbol(Adafruit_SSD1306 &d)
{
    d.drawRoundRect(52, 2, 24, 24, 3, WHITE);
    d.setCursor(56, 10);
    d.print("MOD");
}

WidgetMyModule::WidgetMyModule(uint32_t displayTime, WidgetFlags action)
    : _displayTime(displayTime), _action(action)
{
}

void WidgetMyModule::setup()
{
    if (_display == nullptr)
        logErrorP("Display is NULL.");
}

void WidgetMyModule::start()
{
    _state = WidgetState::RUNNING;
    _forceRedraw = true;
}

void WidgetMyModule::stop()
{
    _state = WidgetState::STOPPED;
}

void WidgetMyModule::pause()
{
    if (_state == WidgetState::RUNNING)
        _state = WidgetState::PAUSED;
}

void WidgetMyModule::resume()
{
    if (_state == WidgetState::PAUSED)
    {
        _state = WidgetState::RUNNING;
        _forceRedraw = true;
    }
}

void WidgetMyModule::loop()
{
    if (_state != WidgetState::RUNNING || !_display)
        return;
    // Non-blocking: redraw at most every REDRAW_INTERVAL_MS, at once after a page change.
    const uint32_t now = millis();
    if (_forceRedraw || now - _lastDraw >= REDRAW_INTERVAL_MS)
    {
        _lastDraw = now;
        _forceRedraw = false;
        drawPage();
    }
}

void WidgetMyModule::setPage(size_t page)
{
    if (page >= PAGE_COUNT)
        page = PAGE_COUNT - 1;
    if (page != _page)
    {
        _page = page;
        _forceRedraw = true;
    }
}

void WidgetMyModule::drawPage()
{
    Adafruit_SSD1306 &d = *_display->display;
    Ui::begin(d); // size 1, white, no wrap, cp437 off: never inherit the state of another widget
    d.clearDisplay();
    switch (_page)
    {
        case PAGE_VALUES: drawValues(d); break;
        case PAGE_HISTORY: drawHistory(d); break;
        default: drawCard(d); break;
    }
    _display->displayBuff();
}

/** @brief Page 1: symbol, badge, one word in size 2, one detail line. */
void WidgetMyModule::drawCard(Adafruit_SSD1306 &d)
{
    char detail[24], t[8];
    snprintf(detail, sizeof(detail), "%s C  %u %%", tenthsText(t, sizeof(t), temperatureTenths()), (unsigned)humidityPercent());
    if (moduleOk())
        Ui::statusCard(d, drawSymbol, Ui::kBadgeOk, "Ready", detail);
    else
        Ui::statusCard(d, drawSymbol, Ui::kBadgeError, "Error", "Check sensor");
    Ui::pageDots(d, PAGE_COUNT, PAGE_CARD);
}

/** @brief Page 2: label and value rows in the 10 px rhythm (y14, 24, 34, ...). */
void WidgetMyModule::drawValues(Adafruit_SSD1306 &d)
{
    char b[24], t[8];
    Ui::header(d, "Values", PAGE_COUNT, PAGE_VALUES);
    snprintf(b, sizeof(b), "%s C", tenthsText(t, sizeof(t), temperatureTenths()));
    Ui::row(d, 0, "Temp", b);
    snprintf(b, sizeof(b), "%u %%", (unsigned)humidityPercent());
    Ui::row(d, 1, "Hum", b);
    Ui::rowIcon(d, 3, moduleOk() ? Ui::kRowOk : Ui::kRowErr, "Sensor", moduleOk() ? "ok" : "error");
}

/** @brief Page 3: big value and the history the module keeps (60 samples, 2 px apart), its span above it on the right. */
void WidgetMyModule::drawHistory(Adafruit_SSD1306 &d)
{
    char b[8];
    Ui::header(d, "History", PAGE_COUNT, PAGE_HISTORY);
    Ui::bigValue(d, 14, tenthsText(b, sizeof(b), temperatureTenths()), "C");
    d.setCursor(128 - Ui::ValueEnd - 6 * Ui::CharW, 40);
    d.print("60 min");
    Ui::graph(d, 8, 50, 119, 13, 2, historyCount(), historyAt, nullptr);
}
#endif // DEVICE_DISPLAY_MODULE
