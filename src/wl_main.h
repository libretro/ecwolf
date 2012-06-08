#ifndef __WL_MAIN_H__
#define __WL_MAIN_H__

/*
=============================================================================

							WL_MAIN DEFINITIONS

=============================================================================
*/

extern  bool     loadedgame;
extern  fixed    focallength;
extern  int      viewscreenx, viewscreeny;
extern  int      viewwidth;
extern  int      viewheight;
extern  int      statusbarx;
extern  int      statusbary;
extern  short    centerx;
extern  short    centerxwide;
extern  int32_t  heightnumerator;
extern  fixed    scale;
extern  fixed    pspritexscale;
extern  fixed    pspriteyscale;
extern  fixed    yaspect;

extern	angle_t	dirangle[9];

extern  int      mouseadjustment;
extern  int      shootdelta;
extern  unsigned screenofs;

extern  bool     startgame;
extern  char     str[80];

//
// Command line parameter variables
//
extern  int      param_difficulty;
extern  const char* param_tedlevel;
extern  int      param_joystickindex;
extern  int      param_joystickhat;
extern  int      param_samplerate;
extern  int      param_audiobuffer;
extern  int      param_mission;

void            NewGame (int difficulty,const class FString &map);
void            CalcProjection (int32_t focal);
void            NewViewSize (int width);
bool            LoadTheGame(FILE *file,int x,int y);
bool            SaveTheGame(FILE *file,int x,int y);
void            ShutdownId (void);

#endif
