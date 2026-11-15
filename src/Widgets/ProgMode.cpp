#ifdef DEVICE_DISPLAY_MODULE
    #include "ProgMode.h"
    #include "../DisplayUi.h"
    #include "OpenKNX.h"
    #include <stdio.h>

WidgetProgMode::WidgetProgMode(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED),
      _lastBlinkTime(0), _showProgMode(true)
{
}

WidgetProgMode::~WidgetProgMode()
{
}

void WidgetProgMode::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetProgMode::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _redraw = true; // initial frame in the next loop
}

void WidgetProgMode::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetProgMode::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetProgMode::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _redraw = true; // redraw in the next loop
}

void WidgetProgMode::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    const unsigned long currentTime = millis();
    if (currentTime - _lastBlinkTime >= PROG_MODE_BLINK_DELAY)
    {
        _lastBlinkTime = currentTime;
        _showProgMode = !_showProgMode;
        _redraw = true;
    }
    if (_redraw || openknx.info.individualAddress() != _drawnPa) draw();
}

void WidgetProgMode::draw()
{
    _redraw = false;
    _drawnPa = openknx.info.individualAddress();

    Adafruit_SSD1306 &d = *_display->display;
    OpenKNX::Ui::begin(d);
    d.clearDisplay();
    OpenKNX::Ui::header(d, "Prog-Mode", 1, 0);

    // Blink phase on: white box 4 px around the text, text in BLACK. x is the printCentred() position.
    static const char kWord[] = "ProgMode";
    if (_showProgMode)
    {
        constexpr int16_t textW = (sizeof(kWord) - 1) * OpenKNX::Ui::CharW * 2;
        const int16_t x = static_cast<int16_t>((d.width() - textW + 2) / 2);
        d.fillRect(static_cast<int16_t>(x - 4), 13, textW + 6, 18, WHITE);
        d.setTextColor(BLACK);
    }
    OpenKNX::Ui::printCentred(d, 14, kWord, 2);
    d.setTextColor(WHITE);

    char pa[14]; // "PA 15.15.255"
    snprintf(pa, sizeof(pa), "PA %u.%u.%u", static_cast<unsigned>(_drawnPa >> 12), static_cast<unsigned>((_drawnPa >> 8) & 0x0F),
             static_cast<unsigned>(_drawnPa & 0xFF));
    OpenKNX::Ui::printCentred(d, 34, pa);
    OpenKNX::Ui::printCentred(d, 44, "Ready to program");
    OpenKNX::Ui::printCentred(d, 54, "Hold OK = exit");
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE