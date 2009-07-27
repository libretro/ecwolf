//
// WL_MENU.H
//

#ifndef __WL_MENU_H__
#define __WL_MENU_H__

#include "id_in.h"

#ifdef SPEAR

#define BORDCOLOR       0x99
#define BORD2COLOR      0x93
#define DEACTIVE        0x9b
#define BKGDCOLOR       0x9d
//#define STRIPE                0x9c

#define MenuFadeOut()   VL_FadeOut(0,255,0,0,51,10)

#else

#define BORDCOLOR       0x29
#define BORD2COLOR      0x23
#define DEACTIVE        0x2b
#define BKGDCOLOR       0x2d
#define STRIPE          0x2c

#define MenuFadeOut()   VL_FadeOut(0,255,43,0,0,10)

#endif

#define READCOLOR       0x4a
#define READHCOLOR      0x47
#define VIEWCOLOR       0x7f
#define TEXTCOLOR       0x17
#define HIGHLIGHT       0x13
#define MenuFadeIn()    VL_FadeIn(0,255,gamepal,10)


#define MENUSONG        WONDERIN_MUS

#ifndef SPEAR
#define INTROSONG       NAZI_NOR_MUS
#else
#define INTROSONG       XTOWER2_MUS
#endif

#define SENSITIVE       60
#define CENTERX         ((int) screenWidth / 2)
#define CENTERY         ((int) screenHeight / 2)

#define MENU_X  76
#define MENU_Y  55
#define MENU_W  178
#ifndef SPEAR
#ifndef GOODTIMES
#define MENU_H  13*10+6
#else
#define MENU_H  13*9+6
#endif
#else
#define MENU_H  13*9+6
#endif

#define SM_X    48
#define SM_W    250

#define SM_Y1   20
#define SM_H1   4*13-7
#define SM_Y2   SM_Y1+5*13
#define SM_H2   4*13-7
#define SM_Y3   SM_Y2+5*13
#define SM_H3   3*13-7

#define CTL_X   24
#define CTL_Y   86
#define CTL_W   284
#define CTL_H   75

#define LSM_X   85
#define LSM_Y   55
#define LSM_W   175
#define LSM_H   10*13+10

#define NM_X    50
#define NM_Y    100
#define NM_W    225
#define NM_H    13*4+15

#define NE_X    10
#define NE_Y    23
#define NE_W    320-NE_X*2
#define NE_H    200-NE_Y*2

#define CST_X           20
#define CST_Y           48
#define CST_START       60
#define CST_SPC 60


//
// FUNCTION PROTOTYPES
//

void CreateMenus();

void US_ControlPanel(ScanCode);

void SetupControlPanel(void);
void SetupSaveGames();
void CleanupControlPanel(void);

void ClearMScreen(void);
void DrawWindow(int x,int y,int w,int h,int wcolor);
void DrawOutline(int x,int y,int w,int h,int color1,int color2);
void WaitKeyUp(void);
void ReadAnyControl(ControlInfo *ci);
void TicDelay(int count);
void CacheLump(int lumpstart,int lumpend);
void UnCacheLump(int lumpstart,int lumpend);
int StartCPMusic(int song);
int  Confirm(const char *string);
void Message(const char *string);
void CheckPause(void);
void ShootSnd(void);
void CheckSecretMissions(void);
void BossKey(void);

void DrawStripes(int y);

void DrawLSAction(int which);

void DrawNewGameDiff(int w);
void FixupCustom(int w);

int CP_ExitOptions(int);
int  CP_EndGame(int);
int  CP_CheckQuick(ScanCode scancode);
int CustomControls(int);

void CheckForEpisodes(void);

void FreeMusic(void);


enum {MOUSE,JOYSTICK,KEYBOARDBTNS,KEYBOARDMOVE};        // FOR INPUT TYPES

//
// WL_INTER
//
typedef struct {
                int kill,secret,treasure;
                int32_t time;
                } LRstruct;

extern LRstruct LevelRatios[];

void Write (int x,int y,const char *string);
void NonShareware(void);
int GetYorN(int x,int y,int pic);

#endif
