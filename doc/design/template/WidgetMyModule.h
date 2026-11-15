#ifdef DEVICE_DISPLAY_MODULE
#pragma once
/**
 * @file        WidgetMyModule.h
 * @brief       Template for a display widget: card, values, history (rename MyModule, replace the values)
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
#include "Widget.h"
#include <cstdint>
#include <string>

/**
 * @brief Three read-only pages: status card, values, history.
 * The WidgetsManager rotates and flips the pages; the widget only draws what the module already holds.
 */
class WidgetMyModule : public Widget
{
  public:
    const std::string logPrefix() { return "mym"; }
    WidgetMyModule(uint32_t displayTime, WidgetFlags action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;

    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }
    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }

    // Pages: page 0 is the card; without auto paging the rotation shows only page 0.
    size_t getPageCount() const override { return PAGE_COUNT; }
    size_t getCurrentPage() const override { return _page; }
    void setPage(size_t page) override;
    void nextPage() override { setPage((_page + 1) % PAGE_COUNT); }
    void prevPage() override { setPage((_page + PAGE_COUNT - 1) % PAGE_COUNT); }

  private:
    enum Page : uint8_t
    {
        PAGE_CARD = 0,
        PAGE_VALUES,
        PAGE_HISTORY,
        PAGE_COUNT
    };

    static constexpr uint32_t REDRAW_INTERVAL_MS = 500;

    WidgetState _state = WidgetState::STOPPED;
    uint32_t _displayTime;
    WidgetFlags _action;
    i2cDisplay *_display = nullptr;
    std::string _name = "MyModule"; // rotation settings are stored by this name: never rename it later
    size_t _page = 0;
    uint32_t _lastDraw = 0;
    bool _forceRedraw = true;

    void drawPage();
    void drawCard(Adafruit_SSD1306 &d);
    void drawValues(Adafruit_SSD1306 &d);
    void drawHistory(Adafruit_SSD1306 &d);
};
#endif // DEVICE_DISPLAY_MODULE
