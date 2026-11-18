#ifdef DEVICE_DISPLAY_MODULE
    /**
     * @file        GestureOverlay.cpp
     * @brief       Hold-/Auto-confirm countdown overlay widget
     * @details     One frame per visible change: action verb as header, target in size 2,
     *              countdown bar, remaining time "N.N s" and a hint line. Phase, bar fraction
     *              and action are read from the bound GestureEngine.
     *              Confirmed/cancelled callbacks fire exactly once per gesture.
     * @version     0.0.1
     * @date        2026-07-09
     * @copyright   Copyright (c) 2026, Erkan Çolak
     *              Licensed under GNU GPL v3.0
     **/
    #include "GestureOverlay.h"
    #include "../DisplayUi.h"
    #include "OpenKNX.h"
    #include <stdio.h>

WidgetGestureOverlay::WidgetGestureOverlay(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED)
{
    // Overlay must win over everything on screen while a gesture is running.
    _priority = WidgetPriority::WIDGET_PRIO_CRITICAL;
}

WidgetGestureOverlay::~WidgetGestureOverlay()
{
}

void WidgetGestureOverlay::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetGestureOverlay::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;

    // Reset the change-tracking snapshot + one-shot callback latches so a fresh overlay
    // paints on the first loop() and re-arms the confirm/cancel callbacks.
    _lastPhase = GesturePhase::Idle;
    _lastBarCols = 0xFF;
    _lastTenths = 0xFFFF;
    _lastAction = GestureAction::None;
    _confirmedFired = false;
    _cancelledFired = false;
}

void WidgetGestureOverlay::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetGestureOverlay::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetGestureOverlay::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _lastBarCols = 0xFF; // repaint in the next loop
}

void WidgetGestureOverlay::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;
    if (!_engine) return; // nothing to show without a bound engine

    const GesturePhase phase = _engine->getPhase();
    const GestureAction action = _engine->getCurrentAction();
    const float frac = _engine->getBarFraction();

    // Fire the one-shot outcome callbacks. Firing/Done -> confirmed, Aborted -> cancelled.
    if ((phase == GesturePhase::Firing || phase == GesturePhase::Done) && !_confirmedFired)
    {
        _confirmedFired = true;
        if (_onConfirmed) _onConfirmed();
    }
    else if (phase == GesturePhase::Aborted && !_cancelledFired)
    {
        _cancelledFired = true;
        if (_onCancelled) _onCancelled();
    }

    // The bar fills toward the action (full when it fires); empty after an abort. Repaint only when
    // the fill column as Ui::bar() draws it or the remaining time (rounded up to 0.1 s) changes.
    const bool ended = phase == GesturePhase::Aborted || phase == GesturePhase::Idle;
    const uint16_t fill = ended ? 0 : static_cast<uint16_t>(1000 - static_cast<uint16_t>(frac * 1000.0f + 0.5f));
    const uint8_t barCols = static_cast<uint8_t>(static_cast<uint32_t>(_display->display->width() - 2 * OpenKNX::Ui::BarPad) * fill / 1000);
    const uint16_t tenths = static_cast<uint16_t>((static_cast<uint32_t>(frac * _engine->getBarMs() + 0.5f) + 99) / 100);

    if (phase == _lastPhase && action == _lastAction && barCols == _lastBarCols && tenths == _lastTenths) return;
    _lastPhase = phase;
    _lastAction = action;
    _lastBarCols = barCols;
    _lastTenths = tenths;
    draw(fill);
}

const char *WidgetGestureOverlay::titleForAction(GestureAction action) const
{
    // Header: what holding does; Exit/Resume when the state is already on.
    switch (action)
    {
        case GestureAction::Pause:
            return _pauseActive ? "Resume" : "Pause";
        case GestureAction::Reboot:
            return "Restart";
        case GestureAction::ProgMode:
            return _progActive ? "Exit" : "Activate";
        case GestureAction::DisplayOff:
            return "Turn off";
        case GestureAction::Screenshot:
            return "Capture";
        case GestureAction::DisplayReset:
        case GestureAction::WifiReset:
            return "Reset";
        case GestureAction::NetworkRestart:
        case GestureAction::Bootloader:
            return "Restart";
        case GestureAction::FullReset:
            return "Erase";
        default:
            return "Action";
    }
}

const char *WidgetGestureOverlay::labelForAction(GestureAction action) const
{
    // Size-2 line: what the action works on (10 characters fit).
    switch (action)
    {
        case GestureAction::Pause:
            return "Rotation";
        case GestureAction::Reboot:
            return "Device";
        case GestureAction::ProgMode:
            return "ProgMode";
        case GestureAction::DisplayOff:
            return "Display";
        case GestureAction::Screenshot:
            return "Screen";
        case GestureAction::DisplayReset:
            return "Display";
        case GestureAction::NetworkRestart:
            return "Network";
        case GestureAction::WifiReset:
            return "WiFi";
        case GestureAction::FullReset:
            return "Data";
        case GestureAction::Bootloader:
            return "Bootloader";
        default:
            return "";
    }
}

void WidgetGestureOverlay::draw(uint16_t fill)
{
    Adafruit_SSD1306 &d = *_display->display;
    OpenKNX::Ui::begin(d);
    d.clearDisplay();
    OpenKNX::Ui::header(d, titleForAction(_lastAction), 1, 0);
    OpenKNX::Ui::printCentred(d, 14, labelForAction(_lastAction), 2);
    OpenKNX::Ui::bar(d, 0, 32, d.width(), fill);

    char left[10]; // "3.0 s" .. "29.0 s"
    snprintf(left, sizeof(left), "%u.%u s", static_cast<unsigned>(_lastTenths / 10), static_cast<unsigned>(_lastTenths % 10));
    OpenKNX::Ui::printCentred(d, 44, left);
    // Hold: releasing early aborts; Auto (menu-triggered): LEFT aborts.
    OpenKNX::Ui::printCentred(d, 54, _mode == GestureOverlayMode::Hold ? "Release = cancel" : "< Cancel");
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE
