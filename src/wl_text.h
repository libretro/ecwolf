#ifndef __WL_TEXT_H__
#define __WL_TEXT_H__

#include "id_vh.h"

/*
=============================================================================

							WL_TEXT DEFINITIONS

=============================================================================
*/

enum ETSAlignment
{
	TS_Left,
	TS_Center,
	TS_Right
};
enum ETSAnchor
{
	TS_Top = MENU_TOP,
	TS_Middle = MENU_CENTER,
	TS_Bottom = MENU_BOTTOM
};

extern void DrawMultiLineText(const char *str, FFont *font, EColorRange color, ETSAlignment align, ETSAnchor anchor);

// HelpScreens is declared in the libretro state_machine.h (needs wl_state_t).

// Returns true if a screen as displayed.
extern  bool    EndText(int exitClusterNum, int enterClusterNum=-1);

// For displaying text going into an episode.
extern void EnterText(unsigned int cluster);

#endif
