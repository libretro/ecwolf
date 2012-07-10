#ifndef _VERSION_H_
#define _VERSION_H_

#define GAMENAME "ECWolf"
#define GAMESIG "ECWOLF"
#define DOTVERSIONSTR_NOREV "0.9999"
#define DOTVERSIONSTR DOTVERSIONSTR_NOREV

#ifndef VERSIONALREADYCHOSEN              // used for batch compiling

/* Defines used for different versions */

#define GOODTIMES

/*
    Wolf3d Full v1.4 Apogee (with ReadThis)   -
    Wolf3d Full v1.4 GT/ID/Activision         - define GOODTIMES
    Wolf3d Shareware v1.4                     - define UPLOAD
    Spear of Destiny Full and Mission Disks   - define SPEAR
                                                (and GOODTIMES for no FormGen quiz)
    Spear of Destiny Demo                     - define SPEAR and SPEARDEMO
*/

#endif

//#define USE_FEATUREFLAGS    // Enables the level feature flags (see bottom of wl_def.h)
//#define USE_SHADING         // Enables shading support (see wl_shade.cpp)
//#define USE_DIR3DSPR        // Enables directional 3d sprites (see wl_dir3dspr.cpp)
#define USE_FLOORCEILINGTEX // Enables floor and ceiling textures stored in the third mapplane (see wl_floorceiling.cpp)
//#define USE_PARALLAX 16     // Enables parallax sky with 16 textures per sky (see wl_parallax.cpp)
//#define USE_CLOUDSKY        // Enables cloud sky support (see wl_cloudsky.cpp)
//#define USE_STARSKY         // Enables star sky support (see wl_atmos.cpp)
//#define USE_RAIN            // Enables rain support (see wl_atmos.cpp)
//#define USE_SNOW            // Enables snow support (see wl_atmos.cpp)
//#define FIXRAINSNOWLEAKS    // Enables leaking ceilings fix (by Adam Biser, only needed if maps with rain/snow and ceilings exist)

#define PLAYDEMOLIKEORIGINAL  // When playing or recording demos, several bug fixes do not take
                              // effect to let the original demos work as in the original Wolf3D v1.4
                              // (actually better, as the second demo rarely worked)

#endif
