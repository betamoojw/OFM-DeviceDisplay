#pragma once
/**
 * @file        DeviceDisplay.h
 * @brief       Main display module for OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-12-17
 * @copyright   Copyright (c) 2024, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

#ifdef DEVICE_DISPLAY_MODULE
    #define WIDGET_MANAGER

    #define DISPLAY_LOW_LEVEL_COMMANDS // Enable low-level commands

    #define DeviceDisplay_Display_Name "DeviceDisplay"

    #include "ddc_console.h"
    #include "Devices/ButtonManager.h"
    #include "Devices/i2cDisplay.h"
    #include "Gesture/GestureEngine.h"
    #include "Menu/MenuConfig.h"
    #include "Menu/MenuRegistry.h"
    #include "OpenKNX.h"
    #include "Settings/DisplaySettingsStore.h"
    #include "Toast.h"
    #include "WidgetsManager.h"

class WidgetsManager;
class ButtonManager;
class DdcConsole;
class i2cDisplay;
class WidgetConsole;
class WidgetProgMode;
class WidgetGestureOverlay;
class WidgetAbout;
class WidgetOTA;
class MenuWidget;
class MenuRegistry;
class Widget;

// Screensaver selection. setScreenSaverType() owns the value->widget mapping.
// The enum value is the persisted field (screenSaverType); lists map it via screenSaverListPos().
// 5 and 8 (former Matrix, Rain) stay unused, a stored 5/8 loads as MatrixClassic.
enum class ScreenSaverType : uint8_t
{
    Clock = 0,         // WidgetClock (analog)
    Cube3D = 1,        // WidgetCube3D
    Doom = 2,          // WidgetDoom (raycaster)
    Fireworks = 3,     // WidgetFireworks
    Life = 4,          // WidgetLife (Conway)
    MatrixClassic = 6, // WidgetMatrixClassic, listed as "Matrix"
    Pong = 7,          // WidgetPong
    Starfield = 9,     // WidgetStarfield
    Off = 10           // no screensaver widget -> WidgetsManager goes straight to SLEEP
};

    // Hardware validation
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_INST
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_INST);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SDA
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SDA);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SCL
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SCL);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_WIDTH
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_WIDTH);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_HEIGHT
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_HEIGHT);
    #endif

/**
 * @brief Main Display Module
 *
 * Orchestrates:
 * - Display hardware (I2C SSD1306)
 * - Widget system (via WidgetsManager)
 * - Button input (via ButtonManager)
 * - Console commands (via DdcConsole)
 * - ProgMode handling
 */
class DeviceDisplay : public OpenKNX::Module
{
  private:
    // Core components
    i2cDisplay* _displayModule = nullptr;
    WidgetsManager* _widgetManager = nullptr;
    ButtonManager* _buttonManager = nullptr;
    DdcConsole* _ddcConsole = nullptr;

    // Central menu-item registry (module member, not a global singleton)
    MenuRegistry* _menuRegistry = nullptr;

    // RAM-side persisted display + widget settings (no flash access here)
    DisplaySettingsStore _settingsStore;

    // Rotation widgets in registration order with the values they were registered with.
    struct WidgetDefault
    {
        Widget* widget;
        uint32_t displayTimeMs;
        bool enabled;
    };
    WidgetDefault _widgetDefaults[WIDGET_SETTINGS_MAX];
    uint8_t _widgetDefaultCount = 0;

    // Widget references
    WidgetConsole* _consoleWidget = nullptr;
    MenuWidget* _menuWidget = nullptr;               // kept so setup() can hand it the registry
    WidgetProgMode* _progModeWidget = nullptr;       // exclusive prog widget
    WidgetGestureOverlay* _gestureOverlay = nullptr; // hold/auto confirm overlay
    WidgetAbout* _aboutWidget = nullptr;             // menu "About" SciFi HUD (shown on demand)
    WidgetOTA* _otaWidget = nullptr;                 // SYSTEM-priority OTA update overlay (over everything)

    // "Display" hardware tuning: live-previewed values NOT yet persisted. The store holds the saved
    // value; these hold the currently-applied one. disp_save copies pending -> store. Boot/apply/reset
    // seed pending FROM the store. So the menu always shows what is live, save commits it.
    bool _pendDispRotate = false;
    uint8_t _pendDispPrechargeIdx = 5;
    uint8_t _pendDispRefreshIdx = 3;

    uint8_t _konamiPos = 0;                          // KONAMI sequence match position (see handleButtonEvent)
    static constexpr uint8_t KONAMI_FIRST_OK = 9;    // _konamiPos after the first OK has matched

    // About pages: shown from the menu or by the KONAMI sequence; they get every key while shown.
    bool _aboutActive = false; // About currently displayed
    // One bit per ButtonType: swallow the release (RELEASE/LONG_PRESS/VERY_LONG_PRESS) of a button still held
    // after a gesture forced the display OFF, or of the press that woke a dark panel.
    uint8_t _swallowMask = 0;
    static uint8_t buttonBit(ButtonType type) { return static_cast<uint8_t>(1u << static_cast<uint8_t>(type)); }

    // Hold-to-confirm gesture engine. Owned by the module (value member so it
    // lives as long as DeviceDisplay). Wired to the WidgetsManager/knx callbacks in init().
    GestureEngine _gestureEngine;

    // Per-button gesture bookkeeping. When a directional/OK PRESS arrives we arm the
    // engine; on RELEASE we decide short-press (navigate) vs gesture-consumed (no navigation).
    ButtonType _heldGestureButton = ButtonType::SELECT; // which button armed the current hold
    bool _gestureButtonArmed = false;                   // a hold is currently armed for _heldGestureButton
    // Latched once the armed gesture reaches Counting/fires, so the eventual release is
    // consumed even if the engine already folded back to Idle while the button was still held.
    bool _gestureReachedBar = false;

    // Remember whether we forced the prog-exclusive display so we can restore normal
    // operation (rotation + screensaver) on exit.
    bool _progExclusiveActive = false;
    Widget* _savedScreenSaverWidget = nullptr; // screensaver suppressed during prog, restored after

    // The screensaver instance owned by DeviceDisplay (the WidgetsManager only borrows it),
    // so we delete the old one ourselves when switching the screensaver type.
    Widget* _screenSaverOwned = nullptr;
    #ifdef DEVICE_DISPLAY_BOOT_ANIM
    // BootLogoAnim requested by the product (useBootLogoAnim), handed to the boot widget in initializeWidgets()
    bool _bootAnim = false, _bootAnimVersion = true, _bootAnimDate = true;
    const char* _bootAnimAuthor = nullptr;
    #endif
    // screensaver family installed by setScreenSaverType() (0xFF = none); makes a repeated apply a no-op.
    uint8_t _screenSaverTypeInstalled = 0xFF;

    // --- Screenshot (RIGHT-hold gesture / "ddc screenshot") -------------------------------------
    // The gesture callback and console command only REQUEST; the SD write runs from loop() (a
    // blocking write in a button callback reboots the RP2040). The CLEAN framebuffer is frozen into
    // _shotBuf BEFORE the confirm overlay appears, so the saved image never shows the overlay.
    enum class ShotState : uint8_t { Idle, Rows, Close, Done, Failed };
    static constexpr uint16_t SHOT_MAX_BYTES = 1024;  // 128 * 64 / 8
    static constexpr uint16_t SHOT_ROWS_PER_TICK = 8; // chunked write, well under the loop budget
    uint8_t _shotBuf[SHOT_MAX_BYTES] = {0};           // frozen framebuffer snapshot (no heap)
    ShotState _shotState = ShotState::Idle;
    bool _shotPending = false;                        // a request is latched (snapshot already frozen)
    bool _shotFrozen = false;                         // _shotBuf holds a valid frozen frame
    uint16_t _shotW = 0, _shotH = 0, _shotRow = 0;
    uint16_t _nextShot = 0;                           // cached next /screenshot_NNN.bmp index
    std::string _shotPath;                            // current target path (for the toast)

    void captureFramebuffer();              // freeze the live framebuffer into _shotBuf
    void screenshotTick();                  // loop-driven writer FSM

    // Toasts: queue + forms, drawn by i2cDisplay into the frame for the panel.
    DisplayToast _toast;
    uint32_t _toastKey = 0;   // DisplayToast::frameKey() at the last check in loop()
    uint32_t _overlayNow = 0; // millis() of the last overlay key, used by the draw step
    uint32_t toastOverlayKey();
    void composeToast(Adafruit_SSD1306& d);
    DisplayToast::Scene toastScene() const;
    void toastTick();

    // Internal functions
    void handleProgMode();
    void handleOTA(); // poll NetworkModule OTA status -> drive the OTA overlay
    // Push EVERY persisted display setting onto the live runtime (power-save, brightness, invert,
    // autoPaging, screensaver type, icon-menu, home-key map). Single canonical path used at boot AND
    // after a settings reset, so the two never diverge and no switch is left unapplied.
    void applyAllSettingsToRuntime();
    // Widget settings: a stored record sets enabled, duration and position of its widget; a widget
    // without a record gets the values it was registered with and follows in registration order.
    void onWidgetAdded(Widget* widget);
    const WidgetSetting* widgetRecord(Widget* widget) const;
    uint8_t registeredIndex(Widget* widget) const;
    size_t widgetRank(Widget* widget) const;
    void restoreWidget(Widget* widget, bool registeredIfNone);
    void applyWidgetOrder();
    void applyWidgetSettings();
    void captureWidgetSettings();
    // KONAMI: advance the sequence matcher with one button PRESS; returns true when the full code is entered.
    bool matchKonami(ButtonType t);
    void initializeWidgets();

    // Register the display-submenu onValueChanged callbacks and wire the
    // screen/gesture hooks the MenuWidget declared but left unset. Called once from setup().
    void wireMenuCallbacks();

    // Re-seed the GestureEngine's Home-key map from the persisted display settings (used at
    // init/setup and after a "Home keys" menu change).
    void seedGestureKeyMapFromSettings();

    // Is the Home screen currently shown? Directional gestures only arm there.
    bool isHomeScreen() const;
    void armGesture(ButtonType type);

    // On the Home screen a short directional press steps through the widget rotation manually.
    void handleHomeNavigation(ButtonType type);

    void updateGestureOverlay();

    // Show / dismiss the About HUD (menu "About"). See the implementation for the display handoff.
    void showAbout(bool unlocked = false);
    void hideAbout();

    // Enter/leave the prog-exclusive display mode (suppress rotation/menu/screensaver).
    void enterProgExclusive();
    void leaveProgExclusive();

  public:
    DeviceDisplay();
    ~DeviceDisplay();

    /**
     * @brief Shows a short message over the current screen; the lines are copied (21 characters each).
     * @param icon9 9x9 row icon, nullptr = icon of the level
     * @param open target for OK on an error card (Error only), nullptr = OK closes
     */
    void toast(ToastLevel level, const uint8_t* icon9, const char* line1, const char* line2 = nullptr, void (*open)() = nullptr);
    /** @brief Text split at the first '\n'; a second line wider than the toast keeps its end. */
    void toastText(ToastLevel level, const char* text);
    /** @brief Removes the toast with this line1, on screen or waiting: its cause is gone. */
    void retractToast(const char* line1);
    /** @brief Info toast from text (see toastText()). */
    void showToast(const char* msg) { toastText(ToastLevel::Info, msg); }
    #ifdef OPENKNX_DISPLAY_TOAST_LOG
    void printToastLog() const { _toast.printLog(); }
    #endif

    // True while a button is held (a gesture hold is armed). Lets a long background job (e.g. the SD
    // Low-Level format) yield for that moment so the gesture countdown overlay animates smoothly.
    inline bool isGestureActive() const { return _gestureButtonArmed; }

    // OpenKNX::Module interface
    const std::string logPrefix() { return DeviceDisplay_Display_Name; } // Logger prefix

    inline const std::string name() { return DeviceDisplay_Display_Name; }      // Library name
    inline const std::string version() { return MODULE_DeviceDisplay_Version; } // Library version

    void init() override;
    void setup(bool configured) override;
    void loop(bool configured) override;
    #if (MASK_VERSION & 0x0900) != 0x0900 // Couplers (e.g. IP-Router 0x091A) have no GroupObjects
    void processInputKo(GroupObject& ko) override;
    #endif
    void showHelp() override;
    bool processCommand(const std::string command, bool diagnose) override;

    // Fixed, constant flash reservation for the DeviceDisplay module.
    // = 1(format version) + sizeof(DisplaySettings) + 1(widget count)
    //   + WIDGET_SETTINGS_MAX * sizeof(WidgetSetting).
    uint16_t flashSize() override;

    // Persist / restore the DisplaySettingsStore blob (1-byte format version +
    // serialize()); readFlash() validates the version or falls back to defaults.
    void writeFlash() override;
    void readFlash(const uint8_t* data, const uint16_t size) override;

    // Public accessors (for testing/debugging)
    i2cDisplay* getDisplayModule() { return _displayModule; }
    WidgetsManager* getWidgetManager() { return _widgetManager; }
    ButtonManager* getButtonManager() { return _buttonManager; }

    // Access point for modules to register their own menu items.
    // Mirrors getWidgetManager(); may be nullptr before init() has run.
    MenuRegistry* getMenuRegistry() { return _menuRegistry; }

    // RAM settings store (display + per-widget settings)
    DisplaySettingsStore& getSettingsStore() { return _settingsStore; }

    // Console (ddc config) hooks: after mutating the store via getSettingsStore(), push it live +
    // persist + refresh the menu; or restore all defaults the same way.
    void consoleApplyAndSave();
    void consoleResetToDefaults();

    // web hook: like consoleApplyAndSave() but without the forced flash write (web fires per slider step).
    void webApplyAndSave();

    // After a change of a widget's visibility, duration or position: take the live rotation into the
    // store and request the save.
    void saveWidgetSettings();

    // Read-only access to the RAM settings store (e.g. for the "ddc i" info dump).
    // Non-mutating callers should prefer this const overload so they cannot dirty the store.
    const DisplaySettingsStore& settingsStore() const { return _settingsStore; }

    // Access to the hold-to-confirm gesture engine.
    GestureEngine& getGestureEngine() { return _gestureEngine; }

    // Route a raw button event through the gesture engine BEFORE the widget (called
    // by the ButtonManager). Decides short-press (navigate) vs gesture (consume).
    void handleButtonEvent(const ButtonEvent& event);

    // Force the UI back to a known state: menu closed, About/overlays gone, gesture disarmed, all
    // the "swallow the next event" latches cleared, display awake. Exists because reaching the home
    // screen by injecting keys depends on where you currently are -- which makes automated tests
    // guess. This does not guess.
    void forceHome();

    // Let the menu release / retake the screen for an externally-managed modal background widget
    // (the SD file browser). Two background widgets carrying DisplayEnabled|ManagedExternally at
    // once make WidgetsManager::findActiveBackgroundWidget() return the FIRST one in list order —
    // the menu — so the modal gets drawn while the menu silently eats the navigation. The modal
    // must therefore call this with false on open and true when it hands control back.
    // The menu keeps its tree and cursor; only DisplayEnabled is toggled.
    void setMenuDisplayEnabled(bool on);

    // Capture the framebuffer and save it as a BMP to SD, driven from loop() (non-blocking).
    // captureNow=true freezes the live frame immediately (console "ddc screenshot"); the gesture path
    // pre-freezes the clean frame before the overlay and calls with captureNow=false.
    void requestScreenshot(bool captureNow = true);

    // Select the active screensaver from the full family. Off -> nullptr (manager goes
    // straight to SLEEP). The previous widget is freed before the new one is installed.
    void setScreenSaverType(ScreenSaverType type);

    #ifdef DEVICE_DISPLAY_BOOT_ANIM
    /**
     * @brief Boot screen BootLogoAnim instead of the static logo; call before openknx.setup(). Needs DEVICE_DISPLAY_BOOT_ANIM.
     * @param author "by <author>" line (UTF-8, at most 18 chars shown), nullptr = none; read during openknx.setup()
     * @param version firmware version and the "(Dev)" of the product name in the info line
     * @param buildDate build date in the info line
     */
    void useBootLogoAnim(const char* author = nullptr, bool version = true, bool buildDate = true)
    {
        _bootAnim = true;
        _bootAnimAuthor = author;
        _bootAnimVersion = version;
        _bootAnimDate = buildDate;
    }
    #endif

    // Presence-guarded registration helpers, no-ops when the manager/registry is absent.
    // tryAddWidget takes ownership of `widget` (deleted on the no-op path to avoid a leak).
    bool tryAddWidget(Widget* widget);
    bool tryRegisterRootItems(const std::vector<MenuConfig::MenuOption>& items);
    bool tryRegisterRootItem(const MenuConfig::MenuOption& item);
    bool tryRegisterAction(const std::string& key, std::function<void()> fn);

    #ifdef WIDGET_CONSOLE
    WidgetConsole* getConsoleWidget() { return _consoleWidget; }
    #endif
};

extern DeviceDisplay openknxDisplayModule;

#endif // DEVICE_DISPLAY_MODULE
