#ifndef _VERSION_H_
#define _VERSION_H_

const char *GetVersionDescription();
const char *GetVersionHash();
const char *GetVersionTime();
unsigned long long GetSaveVersion();
const char *GetSaveSignature();
const char *GetGameCaption();

#define GAMENAME "ECWolf"
#define GAMESIG "ECWOLF"
#define BINNAME "ecwolf"
#define MAIN_PK3 "ecwolf.pk3"
#if defined(__APPLE__) || defined(_WIN32)
#define GAME_DIR GAMENAME
#else
#define GAME_DIR "ecwolf"
#endif
#define DOTVERSIONSTR_NOREV "1.3.99999"
#define SAVEPRODVER 0x001003FF // 0xMMMmmmrr in hex
#define SAVEVERUNDEFINED 99999999999ull

// Windows RC files have weird syntax so we need an unquoted version
#define RCGAMENAME ECWolf

#define MINSAVEVER	1370923175ull
// The following will be used as a less accurate fallback for non-version control builds
#define MINSAVEPRODVER 0x00100201

#endif
