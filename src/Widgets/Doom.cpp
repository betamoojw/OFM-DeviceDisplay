#ifdef DEVICE_DISPLAY_MODULE
    #include "Doom.h"
    #include "OpenKNX.h"
    #include "OpenKNX/I2C/Wire1Lock.h"
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

// --- Interactive scene on the same raycaster -------------------------------------------
namespace
{
    enum : uint8_t
    {
        CREDITS,
        TITLE,
        PLAY,
        LEVEL,
        OVER
    };
    constexpr uint8_t K_UP = 1, K_DOWN = 2, K_OK = 4, K_LEFT = 8, K_RIGHT = 16;
    constexpr int16_t VH = 55;             // 3D view rows, HUD below
    constexpr uint32_t SCENE_FRAME_MS = 55; // frame cap
    constexpr int16_t SCENE_COLS = 32;      // columns rendered per loop()
    constexpr float MOVE = 0.11f, TURN = 0.14f, RADIUS = 0.22f;
    const char TITLE_TXT[] = "OKNX";

    // Technician face 9x8 (normal, hurt, grin); bit 8 = left.
    const uint16_t FACE[3][8] = {{0x07C, 0x0FE, 0x101, 0x145, 0x101, 0x139, 0x082, 0x07C},
                                 {0x07C, 0x0FE, 0x101, 0x155, 0x129, 0x139, 0x0AA, 0x07C},
                                 {0x07C, 0x0FE, 0x101, 0x16D, 0x101, 0x145, 0x0BA, 0x07C}};
    // Probe gun 16x12 as GFX bitmaps: outline (lit) and body (dark, opaque).
    const uint8_t GUN_LIT[24] = {0x03, 0xC0, 0x02, 0x40, 0x02, 0x40, 0x0F, 0xF0, 0x08, 0x10, 0x0B, 0xD0, 0x0A, 0x50, 0x0B, 0xD0, 0x18, 0x18, 0x20, 0x04, 0x48, 0x12, 0x88, 0x11};
    const uint8_t GUN_DARK[24] = {0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x07, 0xE0, 0x04, 0x20, 0x05, 0xA0, 0x04, 0x20, 0x07, 0xE0, 0x1F, 0xF8, 0x37, 0xEC, 0x77, 0xEE};

    // Move by (mx, my) per axis unless the cell r ahead on that axis is a wall.
    void slide(const uint16_t *m, float &x, float &y, float mx, float my, float r)
    {
        if (!rayWall(m, static_cast<int>(x + mx + (mx > 0 ? r : -r)), static_cast<int>(y))) x += mx;
        if (!rayWall(m, static_cast<int>(x), static_cast<int>(y + my + (my > 0 ? r : -r)))) y += my;
    }
    const int8_t MUZZLE[9][2] = {{0, -2}, {-2, -3}, {2, -3}, {0, -4}, {-3, -1}, {3, -1}, {0, -6}, {-1, -3}, {1, -3}};

    void face(Adafruit_SSD1306 *d, uint8_t f, int16_t x, int16_t y, uint8_t sc)
    {
        for (uint8_t r = 0; r < 8; ++r)
            for (uint8_t b = 0; b < 9; ++b)
                if ((FACE[f][r] >> (8 - b)) & 1) d->fillRect(x + b * sc, y + r * sc, sc, sc, WHITE);
    }

    void text(Adafruit_SSD1306 *d, const char *s, int16_t x, int16_t y, uint8_t size)
    {
        if (x < 0) return centerText(d, s, size, y, 128); // -1 = centred
        d->setTextSize(size);
        d->setCursor(x, y);
        d->print(s);
    }
} // namespace

DoomScene::DoomScene(uint16_t hi)
{
    memset(this, 0, sizeof(*this)); // plain data only; state CREDITS = 0
    _t = millis();
    _hi = hi;
    _col = 128;
}

// Panel invert for the flash, relative to the user's invert setting; one command, no extra frame.
void DoomScene::invert(i2cDisplay *disp, bool on)
{
    if (on == _invOn) return;
    _invOn = on;
    OPENKNX_WIRE1_LOCK();
    disp->display->invertDisplay(on != disp->isInverted());
}

uint16_t DoomScene::rnd(uint16_t n)
{
    _rnd = _rnd * 1103515245u + 12345u;
    return static_cast<uint16_t>((_rnd >> 16) % n);
}

void DoomScene::setState(uint8_t st)
{
    _st = st;
    _t = millis();
}

// Level maze = the screensaver map, transposed and/or flipped per level (4 layouts).
void DoomScene::gen()
{
    _rnd = 7919u * _level + 17u;
    const uint8_t v = (_level - 1) & 3;
    for (uint8_t y = 0; y < 16; ++y)
    {
        uint16_t row = 0;
        for (uint8_t x = 0; x < 16; ++x)
        {
            uint8_t sx = x, sy = y;
            if (v & 1) sx = y, sy = x;
            if (v & 2) sy = 15 - sy;
            row |= ((MAP[sy] >> sx) & 1u) << x;
        }
        _map[y] = row;
    }
    _px = _py = 1.5f;
    _a = 0;
    rayCamAt(_cam, _px, _py, _a);
    memset(_zbuf, 255, sizeof(_zbuf)); // a shot before the first frame sees no wall
    _spd = _level > 6 ? 0.088f : (0.8f + 0.12f * _level) * 0.055f; // bug speed: 0.0506 (L1) .. 0.088 (L7+) cells/frame = 0.92 .. 1.6 cells/s at the 55 ms frame cap
    _nBug = 0;
    const uint8_t want = _level + 2 > 6 ? 6 : _level + 2;
    for (uint8_t guard = 0; _nBug < want && guard < 200; ++guard)
    {
        const uint8_t x = 1 + rnd(14), y = 1 + rnd(14);
        const float dx = x + 0.5f - _px, dy = y + 0.5f - _py;
        if (!rayWall(_map, x, y) && dx * dx + dy * dy > 25.0f)
            _bug[_nBug++] = {x + 0.5f, y + 0.5f, static_cast<uint8_t>(_level >= 3 ? 2 : 1)};
    }
}

bool DoomScene::press(uint8_t bit)
{
    if (_st == CREDITS) setState(TITLE);
    else if ((_st == TITLE || _st == OVER) && bit == K_OK)
    {
        _fireAt = millis(); // the starting OK fires only if it is still held after the autofire delay
        _level = 1;
        _lives = 3;
        _score = 0;
        gen();
        setState(PLAY);
    }
    else if ((_st == TITLE || _st == OVER) && bit == K_LEFT) return true;
    else if (_st == PLAY && bit == K_OK) shoot(true);
    return false;
}

bool DoomScene::los(float x, float y) const
{
    const float dx = x - _px, dy = y - _py;
    const int n = static_cast<int>(sqrtf(dx * dx + dy * dy) * 4.0f) + 1;
    for (int i = 1; i < n; ++i)
        if (rayWall(_map, static_cast<int>(_px + dx * i / n), static_cast<int>(_py + dy * i / n))) return false;
    return true;
}

// Hitscan along the centre column against the last frame's wall depth.
void DoomScene::shoot(bool single)
{
    const uint32_t now = millis();
    if (now - _fireAt < 300) return;
    _fireAt = now;
    _flash = 3;
    if (single) _inv = 2; // two inverted frames; autofire does not flash
    Bug *hit = nullptr;
    float best = _zbuf[64] / 16.0f;
    for (uint8_t i = 0; i < _nBug; ++i)
    {
        Bug &b = _bug[i];
        if (!b.hp) continue;
        const float rx = b.x - _px, ry = b.y - _py;
        const float ty = _cam.dirX * rx + _cam.dirY * ry;
        const float tx = _cam.dirX * ry - _cam.dirY * rx;
        if (ty < 0.2f || ty > best || fabsf(tx) > 0.32f + 0.04f * ty) continue;
        hit = &b;
        best = ty;
    }
    if (hit && !--hit->hp)
    {
        _score += 10 * _level;
        _face = 2;
        _faceUntil = now + 700;
    }
}

void DoomScene::step(uint8_t held)
{
    const uint32_t now = millis();
    if (held & K_LEFT) _a -= TURN;
    if (held & K_RIGHT) _a += TURN;
    rayCamAt(_cam, _px, _py, _a);
    const int f = ((held & K_UP) ? 1 : 0) - ((held & K_DOWN) ? 1 : 0);
    if (f)
    {
        slide(_map, _px, _py, _cam.dirX * MOVE * f, _cam.dirY * MOVE * f, RADIUS);
        _cam.px = _px;
        _cam.py = _py;
    }
    if ((held & K_OK) && now - _fireAt > 350) shoot(false);

    bool alive = false;
    for (uint8_t i = 0; i < _nBug; ++i)
    {
        Bug &b = _bug[i];
        if (!b.hp) continue;
        alive = true;
        const float dx = _px - b.x, dy = _py - b.y, d2 = dx * dx + dy * dy;
        if (d2 < 49.0f && d2 > 0.2f && los(b.x, b.y))
        {
            const float k = _spd / sqrtf(d2);
            slide(_map, b.x, b.y, dx * k, dy * k, 0);
        }
        if (d2 < 0.36f && now - _hurtAt > 1200)
        {
            _hurtAt = now;
            _face = 1;
            _faceUntil = now + 800;
            if (!--_lives)
            {
                if (_score > _hi) _hi = _score;
                setState(OVER);
                return;
            }
        }
    }
    if (!alive)
    {
        ++_level;
        setState(LEVEL);
    }
    if (_face && now > _faceUntil) _face = 0;
}

// Sprites, weapon, HUD and the invert flash on top of the finished wall columns.
void DoomScene::drawFrame(i2cDisplay *disp)
{
    Adafruit_SSD1306 *d = disp->display;
    const uint32_t now = millis();

    // Bugs far to near so a nearer one covers a farther one.
    uint8_t order[6], n = 0;
    float dist[6];
    for (uint8_t i = 0; i < _nBug; ++i)
    {
        if (!_bug[i].hp) continue;
        const float rx = _bug[i].x - _px, ry = _bug[i].y - _py;
        dist[i] = rx * rx + ry * ry;
        uint8_t j = n++;
        for (; j && dist[order[j - 1]] < dist[i]; --j)
            order[j] = order[j - 1];
        order[j] = i;
    }
    const bool leg = (now / 180) & 1;
    for (uint8_t k = 0; k < n; ++k)
    {
        const Bug &b = _bug[order[k]];
        const float rx = b.x - _px, ry = b.y - _py;
        const float ty = _cam.dirX * rx + _cam.dirY * ry;
        if (ty < 0.25f) continue;
        const float tx = (_cam.dirX * ry - _cam.dirY * rx) * 1.515f; // 1 / plane length
        int s = static_cast<int>(VH * 0.62f / ty);
        if (s > 44) s = 44;
        if (s < 3) continue;
        const int scx = static_cast<int>(64.0f * (1.0f + tx / ty)), h = s >> 1, cy = VH / 2 + (s >> 3);
        const int e = s >> 3 ? s >> 3 : 1;
        const uint8_t zt = ty * 16.0f > 254.0f ? 254 : static_cast<uint8_t>(ty * 16.0f);
        for (int c = -h; c <= h; ++c)
        {
            const int col = scx + c, ac = c < 0 ? -c : c;
            if (col < 0 || col > 127 || _zbuf[col] < zt) continue;
            const int w = (h - ac) >> 1;
            d->drawFastVLine(col, cy - w - 1, 2 * w + 3, WHITE);
            const int ex = ac - (h >> 1);
            if (ex > -e && ex < e)
                for (int r = 1 - e; r < e; ++r)
                    d->drawPixel(col, cy - (w >> 1) + r, BLACK);
            if (ac == (h >> 1)) // antenna
                for (int a = 1; a <= (s >> 2); ++a)
                    d->drawPixel(col + (c < 0 ? -(a >> 1) : (a >> 1)), cy - w - 1 - a, WHITE);
            if (ac == (h >> 2) + 1 || ac == h - 1) // legs, two-step walk
                for (int l = 1; l <= (s >> 3) + 1; ++l)
                    d->drawPixel(col + (leg ? (l >> 1) : -(l >> 1)), cy + w + l, WHITE);
        }
    }

    // Crosshair, gun with recoil, muzzle flash.
    d->drawPixel(64, 25, WHITE);
    d->drawPixel(64, 29, WHITE);
    d->drawPixel(62, 27, WHITE);
    d->drawPixel(66, 27, WHITE);
    const int16_t gy = VH - 12 + (_flash > 1 ? 2 : 0);
    d->drawBitmap(56, gy, GUN_DARK, 16, 12, BLACK);
    d->drawBitmap(56, gy, GUN_LIT, 16, 12, WHITE);
    if (_flash > 1)
        for (uint8_t i = 0; i < 9; ++i)
            d->drawPixel(64 + MUZZLE[i][0], gy + MUZZLE[i][1], WHITE);
    if (_flash) --_flash;

    // HUD: face, lives, score, high score, level.
    d->fillRect(0, VH, 128, 64 - VH, BLACK);
    d->drawFastHLine(0, VH, 128, WHITE);
    face(d, _face, 0, VH + 1, 1);
    char buf[24];
    snprintf(buf, sizeof(buf), "\x03%u %04u HI%04u L%u", _lives, _score, _hi, _level);
    text(d, buf, 12, VH + 1, 1);

    // Shot: two inverted frames; hit: two blinks of 80 ms. Panel command, no extra frame.
    const uint32_t ht = now - _hurtAt;
    const bool inv = _inv || (_hurtAt && ht < 240 && (ht < 80 || ht >= 160));
    if (_inv) --_inv;
    invert(disp, inv);
}

void DoomScene::loop(i2cDisplay *disp, uint8_t held)
{
    Adafruit_SSD1306 *d = disp->display;
    if (_col < 128) // finish the wall columns of the current frame
    {
        for (int16_t n = 0; _col < 128 && n < SCENE_COLS; ++_col, ++n)
        {
            const float z = rayColumn(d, _map, _cam, _col, 128, VH) * 16.0f;
            _zbuf[_col] = z > 255.0f ? 255 : static_cast<uint8_t>(z);
        }
        if (_col == 128)
        {
            drawFrame(disp);
            disp->displayBuff();
        }
        return;
    }
    const uint32_t now = millis();
    if (now - _frameAt < SCENE_FRAME_MS) return;
    _frameAt = now;
    d->setTextColor(WHITE);
    d->setTextWrap(false);
    d->cp437(true);
    if (_st == PLAY)
    {
        step(held);
        if (_st == PLAY)
        {
            d->clearDisplay();
            _col = 0;
            return;
        }
    }
    d->clearDisplay();
    char buf[20];
    switch (_st)
    {
        case CREDITS:
            authorCard(d, 128);
            d->drawRect(0, 0, 128, 64, WHITE);
            if (now - _t > 2600) setState(TITLE);
            break;
        case TITLE:
            face(d, 2, 4, 14, 4);
            d->drawRect(0, 0, 128, 64, WHITE);
            text(d, TITLE_TXT, 50, 8, 2);
            snprintf(buf, sizeof(buf), "HI %04u", _hi);
            text(d, buf, 50, 34, 1);
            if ((now / 500) & 1) text(d, "OK start", 50, 50, 1);
            break;
        case LEVEL:
            snprintf(buf, sizeof(buf), "LEVEL %u", _level);
            text(d, buf, -1, 22, 2);
            if (now - _t > 1500)
            {
                gen();
                setState(PLAY);
            }
            break;
        default: // OVER
            text(d, "GAME OVER", -1, 6, 2);
            snprintf(buf, sizeof(buf), "SCORE %04u", _score);
            text(d, buf, -1, 28, 1);
            snprintf(buf, sizeof(buf), "HI %04u", _hi);
            text(d, buf, -1, 38, 1);
            text(d, "OK again  \x11 exit", -1, 52, 1);
            break;
    }
    invert(disp, false); // never leave a flash behind on a static screen
    disp->displayBuff();
}

void DoomScene::end(i2cDisplay *disp)
{
    if (disp && disp->display) invert(disp, false);
}
#endif // DEVICE_DISPLAY_MODULE
