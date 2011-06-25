// WL_ACT1.C

#include "wl_act.h"
#include "wl_def.h"
#include "id_ca.h"
#include "id_sd.h"
#include "thingdef.h"

/*
=============================================================================

														STATICS

=============================================================================
*/

struct StatInfo
{
	short      picnum;
	wl_stat_t  type;
	uint32_t   specialFlags;    // they are ORed to the statobj_t flags
};

StatInfo statinfo[] =
{
	{SPR_STAT_0},                           // puddle          spr1v
	{SPR_STAT_1,block},                     // Green Barrel    "
	{SPR_STAT_2,block},                     // Table/chairs    "
	{SPR_STAT_3,block,FL_FULLBRIGHT},       // Floor lamp      "
	{SPR_STAT_4,none,FL_FULLBRIGHT},        // Chandelier      "
	{SPR_STAT_5,block},                     // Hanged man      "
	{SPR_STAT_6,bo_alpo},                   // Bad food        "
	{SPR_STAT_7,block},                     // Red pillar      "
	//
	// NEW PAGE
	//
	{SPR_STAT_8,block},                     // Tree            spr2v
	{SPR_STAT_9},                           // Skeleton flat   "
	{SPR_STAT_10,block},                    // Sink            " (SOD:gibs)
	{SPR_STAT_11,block},                    // Potted plant    "
	{SPR_STAT_12,block},                    // Urn             "
	{SPR_STAT_13,block},                    // Bare table      "
	{SPR_STAT_14,none,FL_FULLBRIGHT},       // Ceiling light   "
	#ifndef SPEAR
	{SPR_STAT_15},                          // Kitchen stuff   "
	#else
	{SPR_STAT_15,block},                    // Gibs!
	#endif
	//
	// NEW PAGE
	//
	{SPR_STAT_16,block},                    // suit of armor   spr3v
	{SPR_STAT_17,block},                    // Hanging cage    "
	{SPR_STAT_18,block},                    // SkeletoninCage  "
	{SPR_STAT_19},                          // Skeleton relax  "
	{SPR_STAT_20,bo_key1},                  // Key 1           "
	{SPR_STAT_21,bo_key2},                  // Key 2           "
	{SPR_STAT_22,block},                    // stuff             (SOD:gibs)
	{SPR_STAT_23},                          // stuff
	//
	// NEW PAGE
	//
	{SPR_STAT_24,bo_food},                  // Good food       spr4v
	{SPR_STAT_25,bo_firstaid},              // First aid       "
	{SPR_STAT_26,bo_clip},                  // Clip            "
	{SPR_STAT_27,bo_machinegun},            // Machine gun     "
	{SPR_STAT_28,bo_chaingun},              // Gatling gun     "
	{SPR_STAT_29,bo_cross},                 // Cross           "
	{SPR_STAT_30,bo_chalice},               // Chalice         "
	{SPR_STAT_31,bo_bible},                 // Bible           "
	//
	// NEW PAGE
	//
	{SPR_STAT_32,bo_crown},                 // crown           spr5v
	{SPR_STAT_33,bo_fullheal,FL_FULLBRIGHT},// one up          "
	{SPR_STAT_34,bo_gibs},                  // gibs            "
	{SPR_STAT_35,block},                    // barrel          "
	{SPR_STAT_36,block},                    // well            "
	{SPR_STAT_37,block},                    // Empty well      "
	{SPR_STAT_38,bo_gibs},                  // Gibs 2          "
	{SPR_STAT_39,block},                    // flag            "
	//
	// NEW PAGE
	//
	#ifndef SPEAR
	{SPR_STAT_40,block},                    // Call Apogee          spr7v
	#else
	{SPR_STAT_40},                          // Red light
	#endif
	//
	// NEW PAGE
	//
	{SPR_STAT_41},                          // junk            "
	{SPR_STAT_42},                          // junk            "
	{SPR_STAT_43},                          // junk            "
	#ifndef SPEAR
	{SPR_STAT_44},                          // pots            "
	#else
	{SPR_STAT_44,block},                    // Gibs!
	#endif
	{SPR_STAT_45,block},                    // stove           " (SOD:gibs)
	{SPR_STAT_46,block},                    // spears          " (SOD:gibs)
	{SPR_STAT_47},                          // vines           "
	//
	// NEW PAGE
	//
	#ifdef SPEAR
	{SPR_STAT_48,block},                    // marble pillar
	{SPR_STAT_49,bo_25clip},                // bonus 25 clip
	{SPR_STAT_50,block},                    // truck
	{SPR_STAT_51,bo_spear},                 // SPEAR OF DESTINY!
	#endif

	{SPR_STAT_26,bo_clip2},                 // Clip            "
#ifdef USE_DIR3DSPR
	// These are just two examples showing the new way of using dir 3d sprites.
	// You can find the allowed values in the objflag_t enum in wl_def.h.
	{SPR_STAT_47,none,FL_DIR_VERT_MID},
	{SPR_STAT_47,block,FL_DIR_HORIZ_MID},
#endif
	{-1}                                    // terminator
};

