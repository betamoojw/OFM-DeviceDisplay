# Changes

Display module for OpenKNX devices: OLED output, widgets, menu, gestures and a web view.
Entries are grouped by module version; every line traces to a commit in this repository.

## Unreleased

* Change (flush): without the I2C queue (ESP32) a display update sends one 32-column chunk per loop pass instead of a whole page, and the page and column window of a chunk in one transfer instead of six; on an ESP32-S3 at 400 kHz the longest pass goes from about 8.2 to 1.2 ms (calculated with the 120 us driver time per transfer of ScheduledWire); after each chunk the next changed page follows and a page starts its next lap at its first changed column, so on 128x64 every changed column is on the panel within 40 calls of the flush; after markAllDirty() (rotation) each page goes out once in full on this path too; RP2040/RP2350 take this path only while the I2C queue is not ready
* Feature (boot logo): with `DEVICE_DISPLAY_BOOT_ANIM`, `useBootLogoAnim(author, version, buildDate)` before `openknx.setup()` replaces the static boot logo by an animated one: a ping runs from the left KNX square along the bus line to the right one and back, the logo grows from the line and stands 2 s, then rolls up while the product name, the version with the build date and an optional "by <author>" are typed in; 8.4 s on a 128x64 panel, 1 KB heap while it runs. Without the flag no animation code is built; without the call the boot logo stays static for 3 s. `i2cDisplay::flushPending()` tells whether a frame still waits for the panel
* Change (toast): Info toasts show on a dimmed panel, Info and Ok toasts and a Warn shown before over the screensaver, without waking it; a new Warn and an Error wake it as before, sleep and the switched-off panel still drop Info and Ok
* Fix (Doom): each column is cleared right before it is drawn instead of the whole frame at its start; a toast that composes the panel between two passes shows the last frame on the right instead of a black area, except in the first frame after the title or a banner
* Change (text): `FastSSD1306` draws the classic font column by column into the frame buffer, with the same pixels as Adafruit; 100 characters of size 1 and 5 of size 2 took 620 instead of 1432 us on an RP2350
* Change (flush): `displayBuff()` skips the 1 KB copy and diff when the widget frame and the toast are the same as at its last call (a 1 KB compare instead, under a toast a 1 KB hash); the display part of a loop pass went from about 395 to 270 us on average on an RP2350 (rotation, no toast). `i2cDisplay::setOverlay()` takes a key function next to the draw function; `WidgetsManager::setFrameCurrent()` is removed
* Change (toast): the toast step returns at once while no toast is queued and none was retracted in the last 60 s
* Change (screensavers): Matrix and Rain are removed; Matrix Classic is listed as "Matrix" and is the default. The stored value stays the type number, a stored Matrix (5) or Rain (8) loads as Matrix. Menu, web page and `ddc config` show the list position 0..8
* Change (KONAMI): the sequence opens About and no longer restores the display defaults; that is done by Display -> Reset in the menu or `ddc config reset`. Neither of its two OKs selects a menu entry or arms a gesture (the first one after the eight arrows is dropped). While ProgMode is on or a menu editor is open, it opens nothing and drops no key
* Change (About): every key goes to About while it is shown, so no hold gesture can arm there; a direction key, OK in an About opened from the menu, or 20 s without a key closes it, and the release of the closing key is swallowed
* Fix (live view): `/display/fb.bin`, the live stream and screenshots show the last frame sent to the panel; the Doom screensaver draws a frame over several loops, so the view showed half-drawn frames
* Fix (menu): a rebuild with the menu open, e.g. Display -> Reset, frees the old menu tree before it builds the new one instead of holding both, and moves the display entries into the new tree instead of copying them
* Change (Doom): the raycaster column code moved to `Raycaster.cpp` and the maze is a 16x16 bitmask; the picture is unchanged
* Feature (widgets): `OpenKNX::Ui` in `src/DisplayUi.h` draws what every widget shares: header with page dots, rows, icon rows, status card, big value, badge, bar, key hint and graph. `src/Icons/UiIcons.h` holds the 9x9 row icons, 13x13 badges and 32x32 menu icons. The line under the title is dotted and swaps its phase every minute, so each of its pixels is lit half of the time
* Change (menu): lists show the menu name and "n/m" in the header, rows in 10 px steps and a scrollbar from 6 entries; toggles read "On" / "Off" instead of a switch bitmap
* Change (icon menu): the root is a carousel with the 32x32 icon of the entry, half-size neighbours, name and info line. The icon comes from `src/Icons/UiIcons.h` by the label; `MenuOption::icon` is removed
* Change (text editor): neighbour characters and the groups abc ABC 123 #+= are shown; holding UP or DOWN jumps to the next or previous group. A text item can limit its length (`ip[0]`) and its characters (`dropdownOptions[0]`)
* Change (ProgMode, OTA, hold gesture): the full-screen views use the shared header; the hold gesture names the action and shows the remaining time as "2.6 s"
* Change: the console widget SysInfoLite (`ddc sysinfo`) is removed; the Time widget shows the time, the System-Info widget of OGM-Common the up time and the address
* Change (rotation): Auto paging is off by default: each widget comes in on its first page, UP/DOWN flip the sub-pages and a flipped page gets the full display time. The icon menu is on by default. These are defaults: stored display settings stay, so a device that stored its display settings before keeps auto paging on and the text menu until Display -> Auto paging, Icon menu or Reset
* Change (manager corner): play or pause is shown whenever widgets rotate, also without auto paging; the pinned widget shows a lock
* Feature (toast): `toast()`, `toastText()` and `retractToast()` show short messages over the current view: at the bottom for Info and Ok (2.5 s) and Warn (5 s), as a banner while the menu is open, as an error card until OK or LEFT. One is shown and three wait, errors first; the same first line within 10 s counts up ("x3"); a retracted message that returns within 60 s counts on and does not wake the panel again. Warn and Error wake a dimmed panel, the screensaver or sleep; a panel switched off by the gesture lets them wait. Console: `ddc toast <i|o|w|e> <text>`
* Change (toast): `showToast()` takes a `const char*` and shows an Info toast over the view; before, it cleared the screen and held the text 1.8 s
* Feature: switching dimming, screensaver or sleep off shows "Screen may burn in" once
* Feature (pin): `WidgetsManager::pin()` / `unpin()` keep a widget on screen instead of the rotation; LEFT/RIGHT/UP/DOWN still work, and 30 s after the last of them the pinned widget is back on its first page. A status widget, the menu and the screensaver still go first. `showDefaultWidget()` is removed
* Fix (rotation): a manual widget switch shortly before the `millis()` wrap (49.7 days) caused several widget switches within a second
* Docs: `doc/design` holds the display design handbook (`index.html`), the rules, the fixed values, the toast rules and a widget template

## v0.1.2: 2026-11-05

* Fix (SSD1306): multi-byte commands (contrast, dimming, offset, clock, precharge, scroll) go out as one I2C transfer; a transfer split between parameters could leave garbage on the panel. Leaving dim restores the contrast of the brightness level instead of a fixed one
* Change: the menu "Network" is no longer built here; OFM-Network registers it as a module root. A lazy submenu re-seeds its keys from the builder on entry, so a dropped edit does not stay
* Feature: a module can pass a 13x13 bitmap for the icon menu with `MenuOption::icon`; without one the menu picks the glyph from the label as before
* Fix (keys): on a dark panel (Sleep or Off) a short key press only wakes it, and holding a key still fires its gesture; before, the press woke the panel and its release acted, so a short OK opened the menu. Dimmed and Screensaver still wake and act on the same press
* Fix (gestures): a hold as the first key press after the DisplayOff gesture fires its gesture; before, the engine was still in the DisplayOff hold, ignored it, and its release acted as a short press
* Fix (menu): the key press after a LEFT-hold menu exit is handled; before it was swallowed, because the exit waited for a RELEASE that the buttons do not send after LONG_PRESS. `ddc key <k> long` sends PRESS and LONG_PRESS like the buttons and acts once instead of twice

## v0.1.1: 2026-10-29

Tagged as `v0.1.1`, which is the version `library.json` carries -- the tag and the file state the same
thing. The live view needs OFM-Network `ec/v0.9.0-beta.2`, which adds `trySendBinaryMessage()`.

**Web view**
* Feature: "Live (max. Rate)" streams the framebuffer as the binary message `dspFrame` over the UI WebSocket of OFM-Network. The device sends a frame only in a loop pass with free loop time and drops it when the send queue has no room; the page shows the frames per second and asks for the stream only while it is visible
* Change: the refresh choices are Aus, Alle 1 s (default), Alle 2 s, Alle 5 s and Live, before 4x/s, 2x/s (default), 1x/s and alle 5s. The invert checkbox is removed; the preview uses the stored invert setting
* Fix: while the panel is off the device sends no framebuffer: `/display/fb.bin` and `/display/shot.bmp` answer 204, the stream sends one frame without data, and the page clears the preview instead of showing the last image. The page tells a switched-off display apart from a switched-off transfer
* Change: the menu item has priority 40 (before 120) and follows WiFi (30)

**Display output**
* Fix: after a rotation the whole frame is sent again -- SEGREMAP applies only to data written after it, so unchanged columns stayed mirrored
* Change: on RP2040/RP2350 with `OPENKNX_I2C_SCHEDULER` the page chunks go to the I2C scheduler queue and `loop()` returns at once; a full queue leaves the rest of the page for the next pass, a failed chunk is sent again. Other platforms keep the synchronous push

**Widgets**
* Feature: `WidgetsManager::showDefaultWidget()` switches the rotation to a given widget; it acts only from the rotation view and returns whether the widget is shown, so a caller can retry
* Feature: `WidgetQRCode::drawQr()` draws a QR code with filled modules and a quiet zone; the WiFi widget of OFM-Network uses it too
* Fix: the QR widget drew its modules as outlines, hollow from 3 px up, and with a white background black modules on the black display -- both are drawn filled now
* Change: the QR code uses a fixed mask, 1.4 ms instead of 22 ms for a version 2 code on the RP2350; `OPENKNX_QR_AUTO_MASK` selects the scored mask. The buffers are sized for version 3 instead of 40

**Buttons**
* Change: with all five front buttons on one expander, a check reads the port once instead of five times

**Build**
* Change: the ddc console is built with `OPENKNX_CONSOLE_DEV`; `DDC_CONSOLE_DISABLE` remains as opt-out, and the console widget is built only together with the console
* Change: the root menu is ordered without `std::stable_sort` -- the same order, and the image shrinks by 14466 bytes

**Texts**
* Change: every text on the display is English -- menu, options, confirmations, toasts, the gesture and programming overlays and the clock. Dim after, Saver after and Sleep after fit one row with their value; the panel tuning submenu is Panel, the display settings submenu Display. The web page keeps its German labels

**Settings**
* Fix: the stored display settings take effect after a restart -- they were read from flash after the setup had applied the defaults, and nothing applied them afterwards
* Feature: visibility, duration and order of the rotation widgets are stored and restored; a widget without a stored record keeps the values it was registered with
* Change: a change is written to flash two seconds after the last edit instead of at most once in three minutes, and not at all when the state equals the stored one
* Change: the stored format is version 4; the settings of version 3 are kept, its widget records -- a fixed placeholder list -- are dropped
* Change: a setting added later is appended to the stored block and keeps the version, so an update keeps the settings

## v0.1.0: 2026-09-15

Tagged as `v0.1.0`, which is the version `library.json` carries -- the tag and the file state the same
thing.

**Widget manager**
* Feature: the manager owns the top-right corner and blinks a "BM" badge while a hardware busmonitor runs, so the state is visible on whatever widget is on screen
* Feature: a play triangle while rotation runs, two bars while it is paused
* Change: `drawPauseOverlay` is `drawManagerOverlay`, since it no longer shows only the pause state; widgets keep their page dots clear of `x >= w-12`

**Widgets**
* Fix: clock, console header and the system-info widget printed UTC where the user reads wall time -- all three use `getLocalTime()` now

**Web view**
* Feature: a web page shows the live OLED image, offers the joystick as buttons, and edits widgets and settings from the browser
* Feature: expanded settings with an expert group and a display-off overlay

**Build and robustness**
* Fix: the OTA overlay is rendered to completion during an RP2040 flash, where the loop is otherwise starved
* Fix: drop a duplicated `DDISP_HAS_NETWORK_MODULE` guard
* Change: `DDC_CONSOLE_DISABLE` also hides the "ddc" help line, which offered a command that was compiled out
* Change: add the ignore file the module was missing, so local build output stops showing up as untracked

## 0.1.0 - 2026-07-19

The release that turned the module from a widget carousel into an operable device front end:
persistent settings, a gesture engine, a menu with in-place editors, and an OTA overlay.

**Settings and menu**
* Feature: display settings are flash-backed and survive a restart
* Feature: menu system v2 -- a registry, value editors, and network IP configuration on the device itself
* Feature: in-place editors in menu v2, so a value is changed where it is shown
* Feature: power-save settings by the minute, plus control over the widget rotation
* Fix: the menu shows the persisted settings, a display switched off stays off, and the ESP loop task gets a larger stack

**Gestures**
* Feature: a 5-way gesture engine with an on-screen overlay, wired into the module core

**Widgets**
* Feature: About, Doom and WidgetTime, plus updates to the widget base class
* Feature: a full-screen overlay widget for OTA updates

**Diagnostics**
* Feature: the framebuffer can be written to SD as a 1-bit BMP screenshot
* Change: `DDCLoggerHelp` is replaced by a `ddc` console handler
* Change: non-blocking i2c diff-flush, so a display update no longer holds the loop

**Portability**
* Change: coupler guard, front buttons, CamelCase enums and portability fixes
* Change: module version 0.0.1 -> 0.1.0

## 0.0.1 - 2024-10-26 .. 2025-11-02

The build-up: display driver, widget system, manager, menu and the hardware breadth.

**Display driver**
* Feature: display geometry and I2C settings come from the build target instead of being hard-coded
* Feature: only the changed part of the framebuffer is sent to the display
* Feature: the transfer is split across loop passes (four columns each), to keep the loop time down
* Change: the driver uses `Wire` instead of a raw `i2c_inst_t`, and moved to `src/Devices`
* Feature: SSD1315 support next to SSD1306
* Feature: ESP32 target support next to RP2040, switchable by macro for multi-target builds

**Widgets and manager**
* Feature: dynamic text lines instead of fixed ones, with per-line alignment and optional empty lines
* Feature: QR-code widget with a fixed version and low ECC, because longer content is not readable on the display
* Feature: boot logo, prog-mode, clock, console, Cube3D, FireWorks and further widgets
* Feature: `WidgetFlags` replace the earlier `WidgetsAction`, with a background flag for widgets drawn behind others
* Feature: priority levels CRITICAL > HIGH > NORMAL > LOW decide which widget owns the screen
* Feature: several default widgets can share the rotation
* Feature: boot logo can be supplied externally
* Feature: a console widget scrolls log output onto the display
* Change: drawing of boot logo, prog mode and the default widget is split across loop passes
* Change: a background cache makes the manager 2.6 times faster
* Change: stopped and paused widgets are skipped in the loop
* Fix: prog mode was not shown while a partial transfer was running
* Fix: a widget with an invalid flag blocked the DEFAULT state
* Fix: Cube3D disappeared because its display pointer was reset

**Menu and input**
* Feature: menu widget, driven by the front-plate buttons and disabled when no front plate is detected
* Feature: support for the front-controller revision with a 5-button joystick
* Change: centralized button handling instead of per-widget reads
* Change: the obsolete GPIO-expander module is replaced by the common GPIO implementation

**Clock**
* Feature: analog and digital clock, the digital one carrying device information
* Feature: the clock shows the days since the last update once a threshold is reached

**Power and idle**
* Feature: the display dims after 60 s and resumes on the prog button
* Feature: power-save state flow with defined transitions back to ACTIVE
* Change: the idle timer has a 5 s minimum, because a default-widget rotation can take longer

**Structure and diagnostics**
* Change: `DeviceDisplay` is split into modular components; the old `Widgets` sources are gone
* Change: runtime statistics are collected for the widget class, to support loop-time work
* Feature: an `info` console command plus hardware validation
* Change: the module is excluded when the target device does not support a display

**Logo and QR fixes** (Cornelius Köpp)
* Fix: missing corners in the large OpenKNX logo, and its spacing to the bus symbol
* Fix: a missing corner in the small OpenKNX logo
* Fix: QR codes could not be scanned -- the colours were swapped
