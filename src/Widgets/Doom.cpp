#ifdef DEVICE_DISPLAY_MODULE
    #include "Doom.h"
    #include "OpenKNX.h"
    #include <math.h>
    #include <string.h>

namespace
{
    // Static maze (16x16), one row per word, bit x set = wall at column x.
    const uint16_t MAP[16] = {0xFFFF, 0x8001, 0xB7BD, 0xA425, 0xA5E5, 0x8421, 0xBDBD, 0x8181,
                              0xBDBD, 0x8421, 0xA5E5, 0xA425, 0xB7BD, 0x8001, 0x8001, 0xFFFF};

    constexpr float MOVE_SPEED = 0.06f; // cells per tick
    constexpr float TURN_SPEED = 0.10f; // radians per tick
    constexpr float PI_F = 3.14159265f;

    // Nearest cardinal direction index for a heading: 0=E(+x), 1=S(+y), 2=W(-x), 3=N(-y).
    int headingIndex(float dir)
    {
        float a = fmodf(dir, 2.0f * PI_F);
        if (a < 0.0f) a += 2.0f * PI_F;
        return static_cast<int>(lroundf(a / (PI_F / 2.0f))) & 3;
    }

    // Rotate `cur` toward `want` by at most `maxStep` radians (shortest way around).
    float turnToward(float cur, float want, float maxStep)
    {
        float diff = want - cur;
        while (diff > PI_F)
            diff -= 2.0f * PI_F;
        while (diff < -PI_F)
            diff += 2.0f * PI_F;
        if (diff > maxStep) diff = maxStep;
        else if (diff < -maxStep)
            diff = -maxStep;
        return cur + diff;
    }

    // Centered GFX text at a given size/row. CP437 enabled so 'Ç' (0x80) prints for the author name.
    void centerText(Adafruit_SSD1306 *d, const char *txt, uint8_t size, int16_t y, int16_t W)
    {
        d->setTextSize(size);
        d->setTextColor(WHITE);
        d->cp437(true);
        int16_t x = (W - static_cast<int16_t>(strlen(txt)) * 6 * size) / 2; // 6px cell per char at size 1
        d->setCursor(x < 0 ? 0 : x, y);
        d->print(txt);
    }

    // Author card: ERKAN / ÇOLAK, big (Ç = CP437 0x80).
    void authorCard(Adafruit_SSD1306 *d, int16_t W)
    {
        centerText(d, "ERKAN", 3, 8, W);
        centerText(d, "\x80OLAK", 3, 36, W);
    }
} // namespace

WidgetDoom::WidgetDoom(uint32_t displayTime, WidgetFlags action)
    : _state(WidgetState::STOPPED), _displayTime(displayTime), _action(action), _display(nullptr)
{
}

void WidgetDoom::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr) logErrorP("Display is NULL!");
}

void WidgetDoom::start()
{
    if (_state == WidgetState::RUNNING) return;
    logDebugP("Starting...");
    _state = WidgetState::RUNNING;
    _px = 1.5f; // start in an open cell (row 1 is the open top corridor; (2,2) is a wall!)
    _py = 1.5f;
    _dir = 0.0f; // facing East (+x)
    _tgtX = static_cast<int>(_px);
    _tgtY = static_cast<int>(_py);
    pickNextCell(); // choose an initial corridor to walk into
    _frameDone = true;
    _renderCol = 0;
    _lastFrameStart = millis();
    _phase = Phase::Title; // Easter egg: open with the zooming DOOM title
    _phaseStart = _lastFrameStart;
    _lastAnimFrame = 0;
    _bannerIdx = 0;
}

void WidgetDoom::stop()
{
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetDoom::pause()
{
    if (_state == WidgetState::RUNNING) _state = WidgetState::PAUSED;
}

void WidgetDoom::resume()
{
    if (_state == WidgetState::PAUSED) _state = WidgetState::RUNNING;
}

void WidgetDoom::loop()
{
    if (_state != WidgetState::RUNNING || !_display || !_display->display) return;

    const uint32_t now = millis();
    const int16_t W = static_cast<int16_t>(_display->GetDisplayWidth());
    const int16_t H = static_cast<int16_t>(_display->GetDisplayHeight());

    if (_phase == Phase::Title) // intro: DOOM zooms to full-screen, then the walk begins
    {
        if (now - _lastAnimFrame >= ANIM_FRAME_MS)
        {
            _lastAnimFrame = now;
            drawTitle(now - _phaseStart, W, H);
        }
        if (now - _phaseStart >= TITLE_MS)
        {
            _phase = Phase::Play;
            _phaseStart = now;
            _frameDone = true;
            _renderCol = 0;
            _lastFrameStart = now;
        }
        return;
    }

    if (_phase == Phase::Banner) // hidden-feature card, then back to walking
    {
        if (now - _lastAnimFrame >= ANIM_FRAME_MS)
        {
            _lastAnimFrame = now;
            drawBanner(_bannerIdx, now - _phaseStart, W, H);
        }
        if (now - _phaseStart >= BANNER_MS)
        {
            _bannerIdx = (_bannerIdx + 1) % BANNER_COUNT; // cycle DOOM -> name -> (C) -> DeviceDisplay
            _phase = Phase::Play;
            _phaseStart = now;
            _frameDone = true;
            _renderCol = 0;
            _lastFrameStart = now;
        }
        return;
    }

    // Phase::Play -- the chunked raycaster; reveal the next hidden-feature card every PLAY_MS (at a
    // frame boundary so we never cut a half-drawn frame).
    if (_frameDone && (now - _phaseStart) >= PLAY_MS)
    {
        _phase = Phase::Banner;
        _phaseStart = now;
        _lastAnimFrame = 0;
        return;
    }

    if (_frameDone) // start a new frame (fps-capped): advance the bot, then FREEZE its geometry
    {
        if (now - _lastFrameStart < FRAME_MS) return;
        _lastFrameStart = now;
        step();
        rayCamAt(_cam, _px, _py, _dir);
        _display->display->clearDisplay();
        _renderCol = 0;
        _frameDone = false;
    }

    // Render a bounded chunk of columns this loop against the frozen snapshot (non-blocking DRAW).
    int16_t done = 0;
    for (; _renderCol < W && done < RENDER_COLS_PER_LOOP; ++_renderCol, ++done)
        rayColumn(_display->display, MAP, _cam, _renderCol, W, H);

    if (_renderCol >= W) // frame complete -> flush (the flush itself is also chunked/async)
    {
        _display->displayBuff();
        _frameDone = true;
    }
}

uint32_t WidgetDoom::getDisplayTime() const { return _displayTime; }
WidgetFlags WidgetDoom::getAction() const { return _action; }

void WidgetDoom::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetDoom::getDisplayModule() const { return _display; }

// Corridor navigation: walk STRAIGHT toward the current target cell centre (so the bot stays
// centred in corridors, never clips a corner); the camera heading smoothly rotates toward the travel
// direction for a natural turn. On arrival at the target centre, choose the next cell (junction).
void WidgetDoom::step()
{
    const float tx = _tgtX + 0.5f, ty = _tgtY + 0.5f;
    const float dx = tx - _px, dy = ty - _py;
    const float dist = sqrtf(dx * dx + dy * dy);

    if (dist < MOVE_SPEED) // arrived -> snap to centre and pick the next corridor
    {
        _px = tx;
        _py = ty;
        pickNextCell();
        return;
    }

    const float inv = MOVE_SPEED / dist; // unit step toward the target centre
    _px += dx * inv;
    _py += dy * inv;
    _dir = turnToward(_dir, atan2f(dy, dx), TURN_SPEED); // pan the camera toward travel
}

// At a cell centre, pick the next target cell: prefer continuing straight through a junction, else
// take a random open passage; only reverse at a dead-end. Keeps the bot exploring corridors.
void WidgetDoom::pickNextCell()
{
    const int cx = static_cast<int>(_px);
    const int cy = static_cast<int>(_py);
    static const int DX[4] = {1, 0, -1, 0}; // E, S, W, N
    static const int DY[4] = {0, 1, 0, -1};
    const int fwd = headingIndex(_dir);
    const int back = (fwd + 2) & 3;

    int opts[4];
    int n = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (i == back) continue; // don't immediately reverse
        if (!rayWall(MAP, cx + DX[i], cy + DY[i])) opts[n++] = i;
    }

    int choose;
    if (n == 0)
    {
        choose = back; // dead-end -> turn around
    }
    else
    {
        bool fwdOpen = false;
        for (int i = 0; i < n; ++i)
            if (opts[i] == fwd) fwdOpen = true;
        choose = (fwdOpen && random(0, 100) < 70) ? fwd : opts[random(0, n)];
    }
    _tgtX = cx + DX[choose];
    _tgtY = cy + DY[choose];
}

// Intro / DOOM banner: the word "DOOM" grows from small to (nearly) full-screen over ~1.6s, as if you
// were running straight into the lettering, then holds framed. Non-blocking: one diffed flush per call.
void WidgetDoom::drawTitle(uint32_t elapsed, int16_t W, int16_t H)
{
    Adafruit_SSD1306 *d = _display->display;
    d->clearDisplay();
    const uint32_t zt = elapsed > 1600 ? 1600 : elapsed;
    uint8_t size = static_cast<uint8_t>(2 + (zt * 3) / 1600); // 2 -> 5
    if (size > 5) size = 5;
    const int16_t y = (H - 8 * size) / 2;
    centerText(d, "DOOM", size, y < 0 ? 0 : y, W);
    if (size >= 5) d->drawRect(0, 0, W, H, WHITE); // full-screen frame at the climax
    d->setTextSize(1);                             // leave the GFX state tidy for other widgets
    _display->displayBuff();
}

// Hidden-feature cards cycled during the walk: 0=DOOM(zoom) 1=author 2=copyright 3=DeviceDisplay.
void WidgetDoom::drawBanner(uint8_t idx, uint32_t elapsed, int16_t W, int16_t H)
{
    if (idx == 0) // "run into" the DOOM lettering again
    {
        drawTitle(elapsed, W, H);
        return;
    }
    Adafruit_SSD1306 *d = _display->display;
    d->clearDisplay();
    switch (idx)
    {
        case 1:
            authorCard(d, W);
            break;
        case 2: // copyright
            centerText(d, "(C) 2026", 2, 12, W);
            centerText(d, "ERKAN \x80OLAK", 1, 42, W);
            break;
        default: // the display module
            centerText(d, "Device", 2, 10, W);
            centerText(d, "Display", 2, 34, W);
            break;
    }
    d->drawRect(0, 0, W, H, WHITE);
    d->setTextSize(1);
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE
