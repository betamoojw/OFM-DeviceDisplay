#ifdef DEVICE_DISPLAY_MODULE
    /**
     * @file        OTAUpdate.cpp
     * @brief       Full-screen OTA update progress overlay widget (WIDGET_PRIO_SYSTEM takeover)
     * @version     0.0.1
     * @date        2026-07-12
     * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
     *              Licensed under GNU GPL v3.0
     */
    #include "OTAUpdate.h"
    #include "../DisplayUi.h"
    #include "OpenKNX.h"
    #include <stdio.h>

WidgetOTA::WidgetOTA(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED)
{
}

WidgetOTA::~WidgetOTA()
{
}

void WidgetOTA::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetOTA::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _redraw = true; // initial frame next loop
}

void WidgetOTA::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetOTA::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetOTA::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _redraw = true; // redraw next loop
}

// Fed by DeviceDisplay each poll; only redraw when the value actually moves.
void WidgetOTA::setProgress(uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent == _percent) return;
    _percent = percent;
    if (_state == WidgetState::RUNNING) _redraw = true;
}

void WidgetOTA::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    // Toggle the "alive" square so a stalled percent still looks like work in progress.
    const unsigned long currentTime = millis();
    if (currentTime - _lastBlinkTime >= OTA_BLINK_DELAY)
    {
        _lastBlinkTime = currentTime;
        _blinkOn = !_blinkOn;
        _redraw = true;
    }
    if (_redraw) draw();
}

void WidgetOTA::draw()
{
    _redraw = false;

    Adafruit_SSD1306 &d = *_display->display;
    OpenKNX::Ui::begin(d);
    d.clearDisplay();
    OpenKNX::Ui::header(d, "OTA Update", 1, 0);
    // Activity square in the top-right corner: the manager overlay draws only on a DefaultWidget.
    if (_blinkOn) d.fillRect(120, 0, 6, 6, WHITE);

    char buf[6]; // "100 %"
    snprintf(buf, sizeof(buf), "%u %%", static_cast<unsigned>(_percent));
    OpenKNX::Ui::printCentred(d, 14, buf, 2);
    OpenKNX::Ui::bar(d, 0, 32, d.width(), static_cast<uint16_t>(_percent * 10));
    OpenKNX::Ui::printCentred(d, 54, "Keep power on");
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE
