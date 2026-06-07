#ifndef _VERSION_H_
#define _VERSION_H_

const char *GetVersionDescription();
const char *GetVersionHash();
const char *GetVersionTime();
unsigned long long GetSaveVersion();
const char *GetSaveSignature();
const char *GetGameCaption();

#include "versiondefs.h"
#define MAIN_PK3 "ecwolf.pk3"

#define SAVEVERUNDEFINED 99999999999ull
#define MINSAVEVER	1370923175ull
/* Save-version floor for the halo-lighting field in AActor::Serialize. Any */
/* save written by a build that includes the feature has GIT_DATE_INT at or */
/* above this; older saves lack the field and default haloLightMask to 0. */
#define SAVEVER_HALOLIGHT 1780000000ull
/* The following will be used as a less accurate fallback for non-version control builds */
#define MINSAVEPRODVER 0x00100201

#endif
