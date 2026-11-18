#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    /**
     * @file        GestureOverlay.h
     * @brief       Hold-/Auto-confirm countdown overlay widget
     * @details     WidgetGestureOverlay renders the gesture confirm overlay: the action verb as
     *              header, the target in size 2, a countdown bar (fraction), the remaining time
     *              "N.N s" and a hint line. It reads phase/fraction/action from a GestureEngine*
     *              (setEngine()) and draws one frame per visible change. It is a CRITICAL
     *              StatusWidget that wants button input so it can overlay the menu/home while a
     *              gesture is running.
     *
     *              Two modes (setMode): Hold (button held, hint "Release = cancel") and Auto
     *              (menu-triggered, hint "< Cancel").
     *              setOnConfirmed()/setOnCancelled() are invoked once when the gesture reaches
     *              Firing/Done (confirmed) or Aborted (cancelled) respectively.
     *
     *              This widget is wired into DeviceDisplay in Phase 2; until then it
     *              is compiled but not registered.
     * @version     0.0.1
     * @date        2026-07-09
     * @copyright   Copyright (c) 2026, Erkan Çolak
     *              Licensed under GNU GPL v3.0
     **/
    #include "../Gesture/GestureEngine.h"
    #include "../Widget.h"
    #include <functional>

/**
 * @brief Overlay display mode.
 * @note  Hold - hold-to-confirm (button held; releasing before the end aborts).
 *        Auto - menu-triggered auto countdown (LEFT aborts); hint "< Cancel".
 */
enum class GestureOverlayMode : uint8_t
{
    Hold = 0,
    Auto
};

class WidgetGestureOverlay : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetGestureOverlay"; }

    // Defaults mirror ProgMode: a CRITICAL status widget that wants button input so it can
    // overlay whatever is on screen while a gesture is running.
    WidgetGestureOverlay(uint32_t displayTime = 0,
                         WidgetFlags action = static_cast<WidgetFlags>(StatusWidget | WantsButtonInput));
    ~WidgetGestureOverlay();

    void setup() override;                                                // Widget setup
    void start() override;                                                // Start widget
    void stop() override;                                                 // Stop widget
    void pause() override;                                                // Pause widget
    void resume() override;                                               // Resume widget
    void loop() override;                                                 // Widget loop
    inline const WidgetState getState() const override { return _state; } // Get widget state
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }               // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget

    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

    /** @brief Bind the gesture engine the overlay reads phase/fraction/action from. */
    inline void setEngine(GestureEngine *engine) { _engine = engine; }

    /** @brief Select the overlay mode (Hold or Auto). Changes the subtext hint. */
    inline void setMode(GestureOverlayMode mode) { _mode = mode; }

    /** @brief Current overlay mode. */
    inline GestureOverlayMode getMode() const { return _mode; }

    /** @brief Callback invoked once when the gesture is confirmed (Firing/Done). */
    inline void setOnConfirmed(std::function<void()> cb) { _onConfirmed = cb; }

    /** @brief Callback invoked once when the gesture is cancelled (Aborted). */
    inline void setOnCancelled(std::function<void()> cb) { _onCancelled = cb; }

    /**
     * @brief Tell the overlay whether KNX prog mode is CURRENTLY active, so the ProgMode
     *        header reads "Exit" (holding will turn it off) instead of "Activate".
     *        Set by DeviceDisplay when the overlay is shown.
     */
    inline void setProgActive(bool active) { _progActive = active; }

    /**
     * @brief Tell the overlay whether the rotation is CURRENTLY paused, so the Pause header
     *        reads "Resume" (holding will resume) instead of "Pause".
     */
    inline void setPauseActive(bool active) { _pauseActive = active; }

  private:
    i2cDisplay *_display;                 // Display module pointer
    WidgetFlags _action;                  // Action flags
    uint32_t _displayTime;                // Time to display the widget
    WidgetState _state;                   // Widget state
    std::string _name = "GestureOverlay"; // Widget name

    GestureEngine *_engine = nullptr;                    // Source of phase/fraction/action
    GestureOverlayMode _mode = GestureOverlayMode::Hold; // Hold vs. Auto hint line

    std::function<void()> _onConfirmed; // Fired once on confirm
    std::function<void()> _onCancelled; // Fired once on cancel

    // Last-drawn snapshot so we only repaint on change; _lastBarCols = 0xFF forces a frame.
    GesturePhase _lastPhase = GesturePhase::Idle;
    uint8_t _lastBarCols = 0xFF; // last drawn bar fill width in pixels
    uint16_t _lastTenths = 0xFFFF; // last drawn remaining time in 0.1 s (up to 290)
    GestureAction _lastAction = GestureAction::None;

    bool _confirmedFired = false; // ensure the confirm callback runs at most once
    bool _cancelledFired = false; // ensure the cancel callback runs at most once
    bool _progActive = false;     // KNX prog mode state at show time (Activate/Exit)
    bool _pauseActive = false;    // rotation pause state at show time (Pause/Resume)

    void draw(uint16_t fill); // Draws the snapshot in one pass; fill = bar fill in permille

    // Per-action strings: header verb and size-2 target.
    const char *titleForAction(GestureAction action) const;
    const char *labelForAction(GestureAction action) const;
};
#endif // DEVICE_DISPLAY_MODULE
