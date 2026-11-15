/**
 * @file        BootLogoDate.cpp
 * @brief       Build date for the animated boot logo
 * @version     0.0.1
 * @date        2026-11-16
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/
#ifdef DEVICE_DISPLAY_BOOT_ANIM
// buildtime.h changes on every build; it is read here alone so only this small file recompiles.
    #if __has_include("buildtime.h")
        #include "buildtime.h" // BUILD_DATETIME "YYYY-MM-DD hh:mm:ss", written by OGM-Common before each build
    #endif

const char *bootLogoBuildDate()
{
#ifdef BUILD_DATETIME
    static const char date[11] = {BUILD_DATETIME[0], BUILD_DATETIME[1], BUILD_DATETIME[2], BUILD_DATETIME[3],
                                  BUILD_DATETIME[4], BUILD_DATETIME[5], BUILD_DATETIME[6], BUILD_DATETIME[7],
                                  BUILD_DATETIME[8], BUILD_DATETIME[9], '\0'};
    return date;
#else
    return nullptr;
#endif
}

#endif // DEVICE_DISPLAY_BOOT_ANIM
