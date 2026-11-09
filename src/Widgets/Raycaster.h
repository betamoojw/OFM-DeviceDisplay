/**
 * @file        Raycaster.h
 * @brief       Column raycaster on a 16x16 bitmask map, shared by the Doom screensaver and its scene
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    #include <Adafruit_SSD1306.h>
    #include <stdint.h>

/** @brief Camera frozen for one frame: position, direction and camera plane (~66 deg FOV). */
struct RayCam
{
    float px, py, dirX, dirY, planeX, planeY;
};

/** @brief True for a wall or a cell outside the map (bit x of map[y] set = wall). */
inline bool rayWall(const uint16_t *map, int x, int y)
{
    return static_cast<unsigned>(x) > 15 || static_cast<unsigned>(y) > 15 || ((map[y] >> x) & 1);
}

/** @brief Freeze the camera at (x, y) looking along angle a. */
void rayCamAt(RayCam &c, float x, float y, float a);

/**
 * @brief Cast screen column x and draw its wall slice into the top h rows (dithered by side and distance).
 * @return perpendicular wall distance, at least 0.1
 */
float rayColumn(Adafruit_SSD1306 *d, const uint16_t *map, const RayCam &c, int16_t x, int16_t w, int16_t h);
#endif // DEVICE_DISPLAY_MODULE
