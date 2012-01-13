#ifndef WL_DEF_H
#define WL_DEF_H

// Defines which version shall be built and configures supported extra features
#include "version.h"

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(_arch_dreamcast)
#	include <kos.h>
#elif !defined(_WIN32)
#	include <stdint.h>
#	include <string.h>
#	include <stdarg.h>
#elif defined(__GNUC__)
#	include <stdint.h>
#endif
#include <SDL.h>

#if !defined O_BINARY
#	define O_BINARY 0
#endif

#ifdef _arch_dreamcast
typedef uint8 uint8_t;
typedef uint16 uint16_t;
typedef uint32 uint32_t;
typedef int8 int8_t;
typedef int16 int16_t;
typedef int32 int32_t;
typedef int64 int64_t;
typedef ptr_t uintptr_t;
#endif

#define FRACBITS 16
#define FRACUNIT (1<<FRACBITS)

typedef uint8_t byte;
typedef uint8_t BYTE;
typedef int8_t SBYTE;
typedef uint16_t word;
typedef uint16_t WORD;
typedef int16_t SWORD;
typedef int32_t fixed;
typedef fixed fixed_t;
typedef uint32_t longword;
#ifndef USE_WINDOWS_DWORD
typedef uint32_t DWORD;
#endif
typedef int32_t SDWORD;
typedef uint64_t QWORD;
typedef int64_t SQWORD;
#ifndef USE_WINDOWS_BOOLEAN
typedef int8_t boolean;
#endif
typedef void * memptr;
typedef uint32_t uint32;
typedef uint32_t BITFIELD;

typedef struct
{
	int x,y;
} Point;
typedef struct
{
	Point ul,lr;
} Rect;

// Screenshot buffer image data types
enum ESSType
{
	SS_PAL,
	SS_RGB,
	SS_BGRA
};

void Quit(const char *errorStr, ...);

#define SIGN(x)         ((x)>0?1:-1)
#define ABS(x)          ((int)(x)>0?(x):-(x))
#define LABS(x)         ((int32_t)(x)>0?(x):-(x))

#define abs(x) ABS(x)

#ifdef WINDOWS
#define stricmp _stricmp
#endif

/*
=============================================================================

							GLOBAL CONSTANTS

=============================================================================
*/

#define MAXPLAYERS		8 // You wish! :P  (This is just here to satisfy ZDoom stuff)
#define BODYQUESIZE		32
#define NUMCOLORMAPS	32

#define MAXTICS 10
#define DEMOTICS        4

//
// tile constants
//

#define ICONARROWS      90
#define PUSHABLETILE    98
#define EXITTILE        99          // at end of castle
#define AREATILE        107         // first of NUMAREAS floor tiles
#define NUMAREAS        37
#define ELEVATORTILE    21
#define AMBUSHTILE      106
#define ALTELEVATORTILE 107

#define NUMBERCHARS     9


//----------------

#define EXTRAPOINTS     40000

#define PLAYERSPEED     3000
#define RUNSPEED        6000

#define SCREENSEG       0xa000

#define SCREENBWIDE     80

#define HEIGHTRATIO     0.50            // also defined in id_mm.c

#define BORDERCOLOR     3
#define FLASHCOLOR      5
#define FLASHTICS       4

#ifndef SPEAR
	#define LRpack      8       // # of levels to store in endgame
#else
	#define LRpack      20
#endif

#undef M_PI
#define PI              3.141592657
#define M_PI PI

#define GLOBAL1         (1l<<16)
#define TILEGLOBAL      GLOBAL1
#define TILESHIFT       16l
#define UNSIGNEDSHIFT   8

#define ANGLES          360             // must be divisable by 4
#define ANGLEQUAD       (ANGLES/4)
#define FINEANGLES      8192
#define FINEMASK        (FINEANGLES-1)
#define ANG90           (FINEANGLES/4)
#define ANG180          (ANG90*2)
#define ANG270          (ANG90*3)
#define ANG360          (ANG90*4)
#define VANG90          (ANGLES/4)
#define VANG180         (VANG90*2)
#define VANG270         (VANG90*3)
#define VANG360         (VANG90*4)

#define MINDIST         (0x5800l)

#define TEXTURESHIFT    6
#define TEXTURESIZE     (1<<TEXTURESHIFT)
#define TEXTUREFROMFIXEDSHIFT 4
#define TEXTUREMASK     (TEXTURESIZE*(TEXTURESIZE-1))

#define SPRITESCALEFACTOR 2

#define NORTH   0
#define EAST    1
#define SOUTH   2
#define WEST    3


#define STATUSLINES     40

#define SCREENSIZE      (SCREENBWIDE*208)
#define PAGE1START      0
#define PAGE2START      (SCREENSIZE)
#define PAGE3START      (SCREENSIZE*2u)
#define FREESTART       (SCREENSIZE*3u)


#define PIXRADIUS       512

#define STARTAMMO       8


// object flag values

typedef enum
{
	FL_SHOOTABLE        = 0x00000001,
	FL_BONUS            = 0x00000002,
	FL_VISABLE          = 0x00000008,
	FL_ATTACKMODE       = 0x00000010,
	FL_FIRSTATTACK      = 0x00000020,
	FL_AMBUSH           = 0x00000040,
	FL_BRIGHT           = 0x00000100,
#ifdef USE_DIR3DSPR
	// you can choose one of the following values in wl_act1.cpp
	// to make a static sprite a directional 3d sprite
	// (see example at the end of the statinfo array)
	FL_DIR_HORIZ_MID    = 0x00000200,
	FL_DIR_HORIZ_FW     = 0x00000400,
	FL_DIR_HORIZ_BW     = 0x00000600,
	FL_DIR_VERT_MID     = 0x00000a00,
	FL_DIR_VERT_FW      = 0x00000c00,
	FL_DIR_VERT_BW      = 0x00000e00,

	// these values are just used to improve readability of code
	FL_DIR_NONE         = 0x00000000,
	FL_DIR_POS_MID      = 0x00000200,
	FL_DIR_POS_FW       = 0x00000400,
	FL_DIR_POS_BW       = 0x00000600,
	FL_DIR_POS_MASK     = 0x00000600,
	FL_DIR_VERT_FLAG    = 0x00000800,
	FL_DIR_MASK         = 0x00000e00,
#endif
	FL_ISMONSTER        = 0x00001000,
	FL_CANUSEWALLS		= 0x00002000,
	FL_COUNTKILL		= 0x00004000,
	FL_SOLID			= 0x00008000,
	FL_PATHING			= 0x00010000,
	FL_PICKUP			= 0x00020000,
	FL_MISSILE			= 0x00040000,

	IF_AUTOACTIVATE		= 0x00000001,
} objflag_t;

/*
=============================================================================

							GLOBAL TYPES

=============================================================================
*/

typedef enum {
	di_north,
	di_east,
	di_south,
	di_west
} controldir_t;

typedef enum {
	dr_normal,
	dr_lock1,
	dr_lock2,
	dr_lock3,
	dr_lock4,
	dr_elevator
} door_t;

typedef enum {
	nothing,
	playerobj,
	inertobj,
	guardobj,
	officerobj,
	ssobj,
	dogobj,
	bossobj,
	schabbobj,
	fakeobj,
	mechahitlerobj,
	mutantobj,
	needleobj,
	fireobj,
	bjobj,
	ghostobj,
	realhitlerobj,
	gretelobj,
	giftobj,
	fatobj,
	rocketobj,

	spectreobj,
	angelobj,
	transobj,
	uberobj,
	willobj,
	deathobj,
	hrocketobj,
	sparkobj
} classtype;

typedef enum {
	none,
	block,
	bo_gibs,
	bo_alpo,
	bo_firstaid,
	bo_key1,
	bo_key2,
	bo_key3,
	bo_key4,
	bo_cross,
	bo_chalice,
	bo_bible,
	bo_crown,
	bo_clip,
	bo_clip2,
	bo_machinegun,
	bo_chaingun,
	bo_food,
	bo_fullheal,
	bo_25clip,
	bo_spear
} wl_stat_t;

typedef enum {
	east,
	northeast,
	north,
	northwest,
	west,
	southwest,
	south,
	southeast,
	nodir
} dirtype;


#define NUMENEMIES  22
typedef enum {
	en_guard,
	en_officer,
	en_ss,
	en_dog,
	en_boss,
	en_schabbs,
	en_fake,
	en_hitler,
	en_mutant,
	en_blinky,
	en_clyde,
	en_pinky,
	en_inky,
	en_gretel,
	en_gift,
	en_fat,
	en_spectre,
	en_angel,
	en_trans,
	en_uber,
	en_will,
	en_death
} enemy_t;

typedef void (* statefunc) (void *);

typedef struct statestruct
{
	boolean rotate;
	short   shapenum;           // a shapenum of -1 means get from ob->temp1
	short   tictime;
	void    (*think) (void *),(*action) (void *);
	struct  statestruct *next;
} statetype;


//---------------------
//
// trivial actor structure
//
//---------------------

typedef struct statstruct
{
	byte      tilex,tiley;
	short     shapenum;           // if shapenum == -1 the obj has been removed
	byte      *visspot;
	uint32_t  flags;
	byte      itemnumber;
} statobj_t;


//---------------------
//
// door actor structure
//
//---------------------

typedef enum
{
	dr_open,dr_closed,dr_opening,dr_closing
} doortype;

typedef struct doorstruct
{
	byte     tilex,tiley;
	boolean  vertical;
	byte     lock;
	doortype action;
	short    ticcount;
} doorobj_t;


//--------------------
//
// thinking actor structure
//
//--------------------

class AActor;
typedef AActor objtype;

enum Button
{
	bt_nobutton=-1,
	bt_attack=0,
	bt_strafe,
	bt_run,
	bt_use,
	bt_slot0,
	bt_slot1,
	bt_slot2,
	bt_slot3,
	bt_slot4,
	bt_slot5,
	bt_slot6,
	bt_slot7,
	bt_slot8,
	bt_slot9,
	bt_nextweapon,
	bt_prevweapon,
	bt_esc,
	bt_pause,
	bt_strafeleft,
	bt_straferight,
	bt_moveforward,
	bt_movebackward,
	bt_turnleft,
	bt_turnright,
	NUMBUTTONS
};

struct ControlScheme
{
	public:
		static void	setKeyboard(ControlScheme* scheme, Button button, int value);
		static void setJoystick(ControlScheme* scheme, Button button, int value);
		static void setMouse(ControlScheme* scheme, Button button, int value);

		Button		button;
		const char*	name;
		int			joystick;
		int			keyboard;
		int			mouse;
};

extern ControlScheme controlScheme[];

enum
{
	gd_baby,
	gd_easy,
	gd_medium,
	gd_hard
};

//---------------
//
// gamestate structure
//
//---------------

typedef struct
{
	short       difficulty;
	short       mapon;
	short       keys;

	short       faceframe;
	short       attackframe,attackcount,weaponframe;

	short       episode,secretcount,treasurecount,killcount,
				secrettotal,treasuretotal,killtotal;
	int32_t     TimeCount;
	int32_t     killx,killy;
	boolean     victoryflag;            // set during victory animations
} gametype;


typedef enum
{
	ex_stillplaying,
	ex_completed,
	ex_died,
	ex_warped,
	ex_resetgame,
	ex_loadedgame,
	ex_victorious,
	ex_abort,
	ex_demodone,
	ex_secretlevel
} exit_t;


extern int mapon;

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
extern  int      param_tedlevel;
extern  int      param_joystickindex;
extern  int      param_joystickhat;
extern  int      param_samplerate;
extern  int      param_audiobuffer;
extern  int      param_mission;
extern  boolean  param_goodtimes;
extern  boolean  param_ignorenumchunks;


void            NewGame (int difficulty,int episode);
void            CalcProjection (int32_t focal);
void            NewViewSize (int width);
boolean         LoadTheGame(FILE *file,int x,int y);
boolean         SaveTheGame(FILE *file,int x,int y);
void            ShowViewSize (int width);
void            ShutdownId (void);


/*
=============================================================================

						WL_GAME DEFINITIONS

=============================================================================
*/

extern  gametype        gamestate;
extern  byte            bordercol;
extern  char            demoname[13];

void    SetupGameLevel (void);
void    GameLoop (void);
void    DrawPlayBorder (void);
void    DrawStatusBorder (byte color);
void    DrawPlayScreen (bool noborder=false);
void    DrawPlayBorderSides (void);
void    ShowActStatus();

void    PlayDemo (int demonumber);
void    RecordDemo (void);


#ifdef SPEAR
extern  int32_t            spearx,speary;
extern  unsigned        spearangle;
extern  boolean         spearflag;
#endif


#define ClearMemory SD_StopDigitized


// JAB
#define PlaySoundLocMapSpot(s,spot)     PlaySoundLocGlobal(s,(((int32_t)spot->GetX() << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)spot->GetY() << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocTile(s,tx,ty)       PlaySoundLocGlobal(s,(((int32_t)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)ty << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocActor(s,ob)         PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_GENERIC)
#define PlaySoundLocActorBoss(s,ob)     PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_BOSSWEAPONS)
void    PlaySoundLocGlobal(const char* s,fixed gx,fixed gy,int chan);
void UpdateSoundLoc(void);


/*
=============================================================================

							WL_PLAY DEFINITIONS

=============================================================================
*/

#define BASEMOVE                35
#define RUNMOVE                 70
#define BASETURN                35
#define RUNTURN                 70

#define JOYSCALE                2

extern  bool noadaptive;
extern  unsigned        tics;
extern  int             viewsize;

extern  int             lastgamemusicoffset;

//
// current user input
//
extern  int         controlx,controly;              // range from -100 to 100
extern  boolean     buttonstate[NUMBUTTONS];
extern  boolean     buttonheld[NUMBUTTONS];
extern  exit_t      playstate;
extern  boolean     madenoise;
extern  objtype     *newobj,*killerobj;
extern  int         godmode;

extern  boolean     demorecord,demoplayback;
extern  int8_t      *demoptr, *lastdemoptr;
extern  memptr      demobuffer;

//
// control info
//
extern  boolean		alwaysrun;
extern  boolean     mouseenabled,mouseyaxisdisabled,joystickenabled;
extern  int         dirscan[4];
extern  int         buttonscan[NUMBUTTONS];
extern  int         buttonmouse[4];
extern  int         buttonjoy[32];

void    PlayLoop (void);

void    CenterWindow(word w,word h);

void    InitRedShifts (void);
void    FinishPaletteShifts (void);

void    PollControls (void);
int     StopMusic(void);
void    StartMusic(void);
void    ContinueMusic(int offs);
void    StartDamageFlash (int damage);
void    StartBonusFlash (void);

#ifdef SPEAR
extern  int32_t     funnyticount;           // FOR FUNNY BJ FACE
#endif

extern  objtype     *objfreelist;     // *obj,*player,*lastobj,

extern  boolean     noclip,ammocheat;
extern  int         singlestep;
extern  unsigned int extravbls;

/*
=============================================================================

								WL_INTER

=============================================================================
*/

void IntroScreen (void);
void PG13(void);
void DrawHighScores(void);
void CheckHighScore (int32_t score,word other);
void Victory (void);
void LevelCompleted (void);
void ClearSplitVWB (void);

void PreloadGraphics(void);


/*
=============================================================================

								WL_DEBUG

=============================================================================
*/

int DebugKeys (void);

/*
=============================================================================

							WL_DRAW DEFINITIONS

=============================================================================
*/

//
// math tables
//
extern  short *pixelangle;
extern  int32_t finetangent[FINEANGLES/4];
extern  fixed sintable[];
extern  fixed finesine[FINEANGLES];
extern  fixed *costable;
extern  int *wallheight;
extern  word horizwall[],vertwall[];
extern  int32_t    lasttimecount;
extern  int32_t    frameon;

extern  unsigned screenloc[3];

extern  boolean fizzlein, fpscounter;

extern  fixed   viewx,viewy;                    // the focal point
extern  fixed   viewsin,viewcos;

void    ThreeDRefresh (void);
void    CalcTics (void);

typedef struct
{
	word leftpix,rightpix;
	word dataofs[64];
// table data after dataofs[rightpix-leftpix+1]
} t_compshape;

/*
=============================================================================

							WL_STATE DEFINITIONS

=============================================================================
*/

void    InitHitRect (objtype *ob, unsigned radius);
void    SpawnNewObj (unsigned tilex, unsigned tiley, statetype *state);
void    NewState (objtype *ob, statetype *state);

bool TryWalk (AActor *ob);
void    SelectChaseDir (objtype *ob);
void    SelectDodgeDir (objtype *ob);
void    SelectRunDir (objtype *ob);
void    MoveObj (objtype *ob, int32_t move);
bool SightPlayer (AActor *ob, double minseedist, double maxseedist, double maxheardist, double fov);

void    DamageActor (AActor *ob, unsigned damage);

bool CheckLine (AActor *ob);

/*
=============================================================================

							WL_TEXT DEFINITIONS

=============================================================================
*/

extern  void    HelpScreens(void);
extern  void    EndText(void);


/*
=============================================================================

							DREAMCAST DEFINITIONS

=============================================================================
*/

#ifdef _arch_dreamcast

// defined in dc_main.cpp
void DC_Main(void);
void DC_CheckParameters(void);
int DC_MousePresent(void);

// defined in dc_vmu.cpp
void StatusDrawLCD(int lcd_index);
int DC_SaveToVMU(char *src, int tp);
int DC_LoadFromVMU(char *dst);

#else

// ignore calls to this function
#define StatusDrawLCD(x)

#endif


/*
=============================================================================

							GP2X DEFINITIONS

=============================================================================
*/

#ifdef GP2X

void GP2X_Init();
void GP2X_Shutdown();
void GP2X_ButtonDown(int button);
void GP2X_ButtonUp(int button);

#endif


/*
=============================================================================

							MISC DEFINITIONS

=============================================================================
*/

void atterm(void (*func)(void));

extern const struct RatioInformation
{
	int baseWidth;
	int baseHeight;
	int viewGlobal;
	fixed tallscreen;
	int multiplier;
	bool isWide;
} AspectCorrection[];
#define CorrectWidthFactor(x)	((x)*AspectCorrection[vid_aspect].multiplier/48)
#define CorrectHeightFactor(x)	((x)*48/AspectCorrection[vid_aspect].multiplier)

static inline fixed FixedMul(fixed a, fixed b)
{
	return (fixed)(((int64_t)a * b + 0x8000) >> 16);
}

static inline fixed FixedDiv(fixed a, fixed b)
{
	return (fixed)(((((int64_t)a)<<32) / b) >> 16);
}

#ifdef PLAYDEMOLIKEORIGINAL
	#define DEMOCHOOSE_ORIG_SDL(orig, sdl) ((demorecord || demoplayback) ? (orig) : (sdl))
	#define DEMOCOND_ORIG                  (demorecord || demoplayback)
	#define DEMOIF_SDL                     if(DEMOCOND_SDL)
#else
	#define DEMOCHOOSE_ORIG_SDL(orig, sdl) (sdl)
	#define DEMOCOND_ORIG                  false
	#define DEMOIF_SDL
#endif
#define DEMOCOND_SDL                   (!DEMOCOND_ORIG)

#define GetTicks() ((SDL_GetTicks()*7)/100)

#define ISPOINTER(x) ((((uintptr_t)(x)) & ~0xffff) != 0)

#define CHECKMALLOCRESULT(x) if(!(x)) Quit("Out of memory at %s:%i", __FILE__, __LINE__)

#ifdef _WIN32
	//#define strcasecmp stricmp
	//#define strncasecmp strnicmp
#else
	static inline char* itoa(int value, char* string, int radix)
	{
		sprintf(string, "%d", value);
		return string;
	}

	static inline char* ltoa(long value, char* string, int radix)
	{
		sprintf(string, "%ld", value);
		return string;
	}
#endif

#define typeoffsetof(type,variable) ((int)(size_t)&((type*)1)->variable - 1)

#define lengthof(x) (sizeof(x) / sizeof(*(x)))
#define endof(x)    ((x) + lengthof(x))

static inline word READWORD(byte *&ptr)
{
	word val = ptr[0] | ptr[1] << 8;
	ptr += 2;
	return val;
}

static inline longword READLONGWORD(byte *&ptr)
{
	longword val = ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
	ptr += 4;
	return val;
}


/*
=============================================================================

						FEATURE DEFINITIONS

=============================================================================
*/

#ifdef USE_FEATUREFLAGS
	// The currently available feature flags
	#define FF_STARSKY      0x0001
	#define FF_PARALLAXSKY  0x0002
	#define FF_CLOUDSKY     0x0004
	#define FF_RAIN         0x0010
	#define FF_SNOW         0x0020

	// The ffData... variables contain the 16-bit values of the according corners of the current level.
	// The corners are overwritten with adjacent tiles after initialization in SetupGameLevel
	// to avoid interpretation as e.g. doors.
	extern int ffDataTopLeft, ffDataTopRight, ffDataBottomLeft, ffDataBottomRight;

	/*************************************************************
	* Current usage of ffData... variables:
	* ffDataTopLeft:     lower 8-bit: ShadeDefID
	* ffDataTopRight:    FeatureFlags
	* ffDataBottomLeft:  CloudSkyDefID or ParallaxStartTexture
	* ffDataBottomRight: unused
	*************************************************************/

	// The feature flags are stored as a wall in the upper right corner of each level
	static inline word GetFeatureFlags()
	{
		return ffDataTopRight;
	}

#endif

#ifdef USE_FLOORCEILINGTEX
	void DrawFloorAndCeiling(byte *vbuf, unsigned vbufPitch, int min_wallheight);
#endif

#ifdef USE_PARALLAX
	void DrawParallax(byte *vbuf, unsigned vbufPitch);
#endif

#ifdef USE_DIR3DSPR
	void Scale3DShape(byte *vbuf, unsigned vbufPitch, statobj_t *ob);
#endif

#endif
