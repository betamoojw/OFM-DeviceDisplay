/**
 * @file        Raycaster.cpp
 * @brief       Column raycaster on a 16x16 bitmask map, shared by the Doom screensaver and its scene
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
    #include "Raycaster.h"
    #include <math.h>

void rayCamAt(RayCam &c, float x, float y, float a)
{
    c.px = x;
    c.py = y;
    c.dirX = cosf(a);
    c.dirY = sinf(a);
    c.planeX = -c.dirY * 0.66f;
    c.planeY = c.dirX * 0.66f;
}

// DDA through the map; NS faces solid, EW faces 50 % checker, far walls sparse.
float rayColumn(Adafruit_SSD1306 *d, const uint16_t *map, const RayCam &c, int16_t x, int16_t w, int16_t h)
{
    const float cameraX = 2.0f * x / w - 1.0f;
    const float rayX = c.dirX + c.planeX * cameraX;
    const float rayY = c.dirY + c.planeY * cameraX;
    int mapX = static_cast<int>(c.px);
    int mapY = static_cast<int>(c.py);
    const float deltaX = (rayX == 0.0f) ? 1e30f : fabsf(1.0f / rayX);
    const float deltaY = (rayY == 0.0f) ? 1e30f : fabsf(1.0f / rayY);
    const int stepX = rayX < 0 ? -1 : 1;
    const int stepY = rayY < 0 ? -1 : 1;
    float sideX = (rayX < 0 ? c.px - mapX : mapX + 1.0f - c.px) * deltaX;
    float sideY = (rayY < 0 ? c.py - mapY : mapY + 1.0f - c.py) * deltaY;

    int side = 0;
    for (uint8_t guard = 0; guard < 64; ++guard)
    {
        if (sideX < sideY)
        {
            sideX += deltaX;
            mapX += stepX;
            side = 0;
        }
        else
        {
            sideY += deltaY;
            mapY += stepY;
            side = 1;
        }
        if (rayWall(map, mapX, mapY)) break;
    }

    const float perp = (side == 0) ? (sideX - deltaX) : (sideY - deltaY);
    const float dist = (perp < 0.1f) ? 0.1f : perp;
    int lineH = static_cast<int>(h / dist);
    if (lineH > h) lineH = h;
    int y0 = h / 2 - lineH / 2;
    int y1 = h / 2 + lineH / 2;
    if (y0 < 0) y0 = 0;
    if (y1 >= h) y1 = h - 1;

    for (int16_t y = static_cast<int16_t>(y0); y <= y1; ++y)
    {
        bool on = (side == 1) || (((x + y) & 1) == 0);
        if (dist > 4.5f) on = on && (((x + y) & 3) == 0);
        if (on) d->drawPixel(x, y, WHITE);
    }
    if (lineH > 2) // crisp top/bottom edge
    {
        d->drawPixel(x, static_cast<int16_t>(y0), WHITE);
        d->drawPixel(x, static_cast<int16_t>(y1), WHITE);
    }
    return dist;
}
#endif // DEVICE_DISPLAY_MODULE
