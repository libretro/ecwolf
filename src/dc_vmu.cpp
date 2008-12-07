/*
    dc_vmu.cpp
    This file is part of the Wolf4SDL\DC project.
    It has the LCD drawing and soon saving functions.

    LCD drawing code found in Sam Steele's DreamZZT.
    Saving code found in OneThirty8's SDLWolf-3D port.

    Cyle Terry <cyle.terry@gmail.com>
*/

#ifdef _arch_dreamcast

#include <kos.h>
#include <zlib/zlib.h>
#include "wl_def.h"
#include "dc_vmu.h"

maple_device_t *lcds[8];
uint8 bitmap[48*32/8];

void StatusDrawLCD(int idxLCD) {
    const char *c;
    int x, y, xi, xb, i;
    maple_device_t *dev;

    memset(bitmap, 0, sizeof(bitmap));

    c = BJFacesLCD[idxLCD-FACE1APIC];

    if(c) {
        for(y = 0; y < 32; y++)    {
            for(x = 0; x < 48; x++)    {
                xi = x / 8;
                xb = 0x80 >> (x % 8);
                if(c[(31 - y) * 48 + (47 - x)] == '.')
                    bitmap[y * (48 / 8) + xi] |= xb;
            }
        }
    }

    i = 0;
    while( (dev = maple_enum_type(i++, MAPLE_FUNC_LCD)) ) {
        vmu_draw_lcd(dev, bitmap);
    }

    vmu_shutdown();
}

int DC_SaveToVMU(char *src, int tp) {
    char dst[32];
    file_t file;
    int filesize = 0;
    unsigned long zipsize = 0;
    uint8 *data;
    uint8 *zipdata;
    vmu_pkg_t pkg;
    uint8 *pkg_out;
    int pkg_size;

    strcpy(dst, "/vmu/a1/");
    strcat(dst, src);

    file = fs_open(src, O_RDONLY);
    filesize = fs_total(file);
    data = (uint8*)malloc(filesize);
    fs_read(file, data, filesize);
    fs_close(file);

    zipsize = filesize * 2;
    zipdata = (uint8*)malloc(zipsize);
    compress(zipdata, &zipsize, data, filesize);

    // Required VMU header
#ifndef SPEAR
    strcpy(pkg.desc_short, "Wolf4SDL\\DC");
    if(tp == 1)
        strcpy(pkg.desc_long, "Configuration");
    else
        strcpy(pkg.desc_long, "Game Save");
    strcpy(pkg.app_id, "Wolf4SDL\\DC");
#else
    strcpy(pkg.desc_short, "Sod4SDL\\DC");
    if(tp == 1)
        strcpy(pkg.desc_long, "Configuration");
    else
        strcpy(pkg.desc_long, "Game Save");
    strcpy(pkg.app_id, "Sod4SDL\\DC");
#endif
    pkg.icon_cnt = 1;
    pkg.icon_anim_speed = 0;
    memcpy(&pkg.icon_pal[0], icon_data, 32);
    pkg.icon_data = icon_data + 32;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = zipsize;
    pkg.data = zipdata;
    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    fs_unlink(dst);
    file = fs_open(dst, O_WRONLY);
    fs_write(file, pkg_out, pkg_size);
    fs_close(file);

    free(pkg_out);
    free(data);
    free(zipdata);

    return 0;
}

int DC_LoadFromVMU(char *dst) {
    char src[32];
    int file;
    int filesize;
    unsigned long unzipsize;
    uint8* data;
    uint8* unzipdata;
    vmu_pkg_t pkg;

    strcpy(src, "/vmu/a1/");
    strcat(src, dst);

    // Remove VMU header
    file = fs_open(src, O_RDONLY);
    if(file == 0) return -1;
    filesize = fs_total(file);
    if(filesize <= 0) return -1;
    data = (uint8*)malloc(filesize);
    fs_read(file, data, filesize);
    vmu_pkg_parse(data, &pkg);
    fs_close(file);

    unzipdata = (uint8 *)malloc(65536);
    unzipsize = 65536;

    uncompress(unzipdata, &unzipsize, (uint8 *)pkg.data, pkg.data_len);
    fs_unlink(dst);
    file = fs_open(dst, O_WRONLY);
    fs_write(file, unzipdata, unzipsize);
    fs_close(file);

    free(data);
    free(unzipdata);

    return 0;
}

#endif // _arch_dreamcast
