#ifndef _VERSION_H_
#define _VERSION_H_

#include "svnrevision.h"

#define GAMENAME "ECWolf"
#define GAMESIG "ECWOLF"
#define DOTVERSIONSTR_NOREV "1.1.1"
#define DOTVERSIONSTR DOTVERSIONSTR_NOREV " (r" SVN_REVISION_STRING ")"

#define MINSAVEVER	1346928390

#define __GETSAVESIG(x) #x
#define GETSAVESIG(x) "ECWOLFSAVE" __GETSAVESIG(x)
#if SVN_REVISION_NUMBER < MINSAVEVER
#define SAVEVER	99999999999
#else
#define SAVEVER	SVN_REVISION_NUMBER
#endif
#define SAVESIG	GETSAVESIG(SAVEVER)

//#define USE_FEATUREFLAGS    // Enables the level feature flags (see bottom of wl_def.h)
//#define USE_DIR3DSPR        // Enables directional 3d sprites (see wl_dir3dspr.cpp)
//#define USE_PARALLAX 16     // Enables parallax sky with 16 textures per sky (see wl_parallax.cpp)
//#define USE_CLOUDSKY        // Enables cloud sky support (see wl_cloudsky.cpp)
//#define USE_STARSKY         // Enables star sky support (see wl_atmos.cpp)
//#define USE_RAIN            // Enables rain support (see wl_atmos.cpp)
//#define USE_SNOW            // Enables snow support (see wl_atmos.cpp)
//#define FIXRAINSNOWLEAKS    // Enables leaking ceilings fix (by Adam Biser, only needed if maps with rain/snow and ceilings exist)

#endif
