#ifndef __WL_MAIN_H__
#define __WL_MAIN_H__

/*
=============================================================================

							WL_MAIN DEFINITIONS

=============================================================================
*/

extern  boolean  loadedgame;
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

extern  int      dirangle[9];

extern  int      mouseadjustment;
extern  int      shootdelta;
extern  unsigned screenofs;

extern  boolean  startgame;
extern  char     str[80];

//
// Command line parameter variables
//
extern  boolean  param_nowait;
extern  int      param_difficulty;
extern  const char* param_tedlevel;
extern  int      param_joystickindex;
extern  int      param_joystickhat;
extern  int      param_samplerate;
extern  int      param_audiobuffer;
extern  int      param_mission;
extern  boolean  param_goodtimes;
extern  boolean  param_ignorenumchunks;


void            NewGame (int difficulty,const class FString &map);
void            CalcProjection (int32_t focal);
void            NewViewSize (int width);
boolean         LoadTheGame(FILE *file,int x,int y);
boolean         SaveTheGame(FILE *file,int x,int y);
void            ShutdownId (void);

#endif
