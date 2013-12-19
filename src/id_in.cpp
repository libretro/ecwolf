//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//


#include "wl_def.h"
#include "c_cvars.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"


#if SDL_VERSION_ATLEAST(2,0,0)
// TODO: Remove this dependency!
#define SDLK_LAST 512
#else
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
typedef SDLMod SDL_Keymod;

inline void SDL_SetRelativeMouseMode(bool enabled)
{
	SDL_WM_GrabInput(enabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
}
inline void SDL_WarpMouseInWindow(struct SDL_Window* window, int x, int y)
{
	SDL_WarpMouse(x, y);
}
#endif

/*
=============================================================================

					GLOBAL VARIABLES

=============================================================================
*/


//
// configuration variables
//
bool MousePresent;


// 	Global variables
volatile bool		Keyboard[SCANCODE_UNMASK(SDLK_LAST)];
volatile bool		Paused;
volatile char		LastASCII;
volatile ScanCode	LastScan;

static KeyboardDef KbdDefs = {
	sc_Control,             // button0
	sc_Alt,                 // button1
	sc_Home,                // upleft
	sc_UpArrow,             // up
	sc_PgUp,                // upright
	sc_LeftArrow,           // left
	sc_RightArrow,          // right
	sc_End,                 // downleft
	sc_DownArrow,           // down
	sc_PgDn                 // downright
};

static SDL_Joystick *Joystick;
int JoyNumButtons;
int JoyNumAxes;
static int JoyNumHats;

static bool GrabInput = false;

/*
=============================================================================

					LOCAL VARIABLES

=============================================================================
*/
static	bool		IN_Started;

static	Direction	DirTable[] =		// Quick lookup for total direction
{
	dir_NorthWest,	dir_North,	dir_NorthEast,
	dir_West,		dir_None,	dir_East,
	dir_SouthWest,	dir_South,	dir_SouthEast
};


///////////////////////////////////////////////////////////////////////////
//
//	INL_GetMouseButtons() - Gets the status of the mouse buttons from the
//		mouse driver
//
///////////////////////////////////////////////////////////////////////////
static int
INL_GetMouseButtons(void)
{
	int buttons = SDL_GetMouseState(NULL, NULL);
	int middlePressed = buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
	int rightPressed = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	buttons &= ~(SDL_BUTTON(SDL_BUTTON_MIDDLE) | SDL_BUTTON(SDL_BUTTON_RIGHT));
	if(middlePressed) buttons |= 1 << 2;
	if(rightPressed) buttons |= 1 << 1;

	return buttons;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyDelta() - Returns the relative movement of the specified
//		joystick (from +/-127)
//
///////////////////////////////////////////////////////////////////////////
void IN_GetJoyDelta(int *dx,int *dy)
{
	if(!Joystick)
	{
		*dx = *dy = 0;
		return;
	}

	SDL_JoystickUpdate();
	int x = SDL_JoystickGetAxis(Joystick, 0) >> 8;
	int y = SDL_JoystickGetAxis(Joystick, 1) >> 8;

	if(param_joystickhat != -1)
	{
		uint8_t hatState = SDL_JoystickGetHat(Joystick, param_joystickhat);
		if(hatState & SDL_HAT_RIGHT)
			x += 127;
		else if(hatState & SDL_HAT_LEFT)
			x -= 127;
		if(hatState & SDL_HAT_DOWN)
			y += 127;
		else if(hatState & SDL_HAT_UP)
			y -= 127;

		if(x < -128) x = -128;
		else if(x > 127) x = 127;

		if(y < -128) y = -128;
		else if(y > 127) y = 127;
	}

	*dx = x;
	*dy = y;
}

int IN_GetJoyAxis(int axis)
{
	return SDL_JoystickGetAxis(Joystick, axis);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyFineDelta() - Returns the relative movement of the specified
//		joystick without dividing the results by 256 (from +/-127)
//
///////////////////////////////////////////////////////////////////////////
void IN_GetJoyFineDelta(int *dx, int *dy)
{
	if(!Joystick)
	{
		*dx = 0;
		*dy = 0;
		return;
	}

	SDL_JoystickUpdate();
	int x = SDL_JoystickGetAxis(Joystick, 0);
	int y = SDL_JoystickGetAxis(Joystick, 1);

	if(x < -128) x = -128;
	else if(x > 127) x = 127;

	if(y < -128) y = -128;
	else if(y > 127) y = 127;

	*dx = x;
	*dy = y;
}

/*
===================
=
= IN_JoyButtons
=
===================
*/

int IN_JoyButtons()
{
	if(!Joystick) return 0;

	SDL_JoystickUpdate();

	int res = 0;
	int i;
	for(i = 0; i < JoyNumButtons && i < 32; i++)
		res |= SDL_JoystickGetButton(Joystick, i) << i;

	// Need four buttons for hat
	if(i < 28 && param_joystickhat != -1)
	{
		uint8_t hatState = SDL_JoystickGetHat(Joystick, param_joystickhat);
		if(hatState & SDL_HAT_UP)
			res |= 0x1 << i;
		else if(hatState & SDL_HAT_DOWN)
			res |= 0x4 << i;
		if(hatState & SDL_HAT_RIGHT)
			res |= 0x2 << i;
		else if(hatState & SDL_HAT_LEFT)
			res |= 0x8 << i;
	}
	return res;
}

int IN_JoyAxes()
{
	if(!Joystick) return 0;

	SDL_JoystickUpdate();

	int res = 0;
	for(int i = 0; i < JoyNumAxes && i < 16; ++i)
	{
		SWORD pos = SDL_JoystickGetAxis(Joystick, i);
		if(pos <= -64)
			res |= 1 << (i*2);
		else if(pos >= 64)
			res |= 1 << (i*2+1);
	}
	return res;
}

bool IN_JoyPresent()
{
	return Joystick != NULL;
}

static void processEvent(SDL_Event *event)
{
	switch (event->type)
	{
		// exit if the window is closed
		case SDL_QUIT:
			Quit(NULL);

		// check for keypresses
		case SDL_KEYDOWN:
		{
			static const byte ASCIINames[] = // Unshifted ASCII for scan codes       // TODO: keypad
			{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,8  ,9  ,0  ,0  ,0  ,13 ,0  ,0  ,	// 0
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,	// 1
				' ',0  ,0  ,0  ,0  ,0  ,0  ,39 ,0  ,0  ,'*','+',',','-','.','/',	// 2
				'0','1','2','3','4','5','6','7','8','9',0  ,';',0  ,'=',0  ,0  ,	// 3
				'`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',	// 4
				'p','q','r','s','t','u','v','w','x','y','z','[',92 ,']',0  ,0  ,	// 5
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0		// 7
			};
			static const byte ShiftNames[] = // Shifted ASCII for scan codes
			{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,8  ,9  ,0  ,0  ,0  ,13 ,0  ,0  ,	// 0
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,	// 1
				' ',0  ,0  ,0  ,0  ,0  ,0  ,34 ,0  ,0  ,'*','+','<','_','>','?',	// 2
				')','!','@','#','$','%','^','&','*','(',0  ,':',0  ,'+',0  ,0  ,	// 3
				'~','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',	// 4
				'P','Q','R','S','T','U','V','W','X','Y','Z','{','|','}',0  ,0  ,	// 5
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0		// 7
			};

			if(event->key.keysym.sym==SDLK_SCROLLLOCK || event->key.keysym.sym==SDLK_F12)
			{
				GrabInput = !GrabInput;
				SDL_SetRelativeMouseMode(GrabInput ? SDL_TRUE : SDL_FALSE);
				return;
			}

			LastScan = SCANCODE_UNMASK(event->key.keysym.sym);
			SDL_Keymod mod = SDL_GetModState();
			if(Keyboard[sc_Alt])
			{
				if(LastScan==SCANCODE_UNMASK(SDLK_F4))
					Quit(NULL);
			}

			if(LastScan == SCANCODE_UNMASK(SDLK_KP_ENTER)) LastScan = SCANCODE_UNMASK(SDLK_RETURN);
			else if(LastScan == SCANCODE_UNMASK(SDLK_RSHIFT)) LastScan = SCANCODE_UNMASK(SDLK_LSHIFT);
			else if(LastScan == SCANCODE_UNMASK(SDLK_RALT)) LastScan = SCANCODE_UNMASK(SDLK_LALT);
			else if(LastScan == SCANCODE_UNMASK(SDLK_RCTRL)) LastScan = SCANCODE_UNMASK(SDLK_LCTRL);
			else
			{
				if((mod & KMOD_NUM) == 0)
				{
					switch(LastScan)
					{
						case SCANCODE_UNMASK(SDLK_KP_2): LastScan = SCANCODE_UNMASK(SDLK_DOWN); break;
						case SCANCODE_UNMASK(SDLK_KP_4): LastScan = SCANCODE_UNMASK(SDLK_LEFT); break;
						case SCANCODE_UNMASK(SDLK_KP_6): LastScan = SCANCODE_UNMASK(SDLK_RIGHT); break;
						case SCANCODE_UNMASK(SDLK_KP_8): LastScan = SCANCODE_UNMASK(SDLK_UP); break;
					}
				}
			}

			int sym = LastScan;
			if(sym >= 'a' && sym <= 'z')
				sym -= 32;  // convert to uppercase

			if(mod & KMOD_SHIFT)
			{
				if((unsigned)sym < lengthof(ShiftNames) && ShiftNames[sym])
					LastASCII = ShiftNames[sym];
			}
			else
			{
				if((unsigned)sym < lengthof(ASCIINames) && ASCIINames[sym])
					LastASCII = ASCIINames[sym];
			}
			if(LastASCII && sym >= 'A' && sym <= 'Z' && (mod & KMOD_CAPS))
				LastASCII ^= 0x20;

			if(LastScan<SCANCODE_UNMASK(SDLK_LAST))
				Keyboard[LastScan] = 1;
			if(LastScan == SCANCODE_UNMASK(SDLK_PAUSE))
				Paused = true;
			break;
		}

		case SDL_KEYUP:
		{
			int key = event->key.keysym.sym;
			if(key == SDLK_KP_ENTER) key = SDLK_RETURN;
			else if(key == SDLK_RSHIFT) key = SDLK_LSHIFT;
			else if(key == SDLK_RALT) key = SDLK_LALT;
			else if(key == SDLK_RCTRL) key = SDLK_LCTRL;
			else
			{
				if((SDL_GetModState() & KMOD_NUM) == 0)
				{
					switch(key)
					{
						case SDLK_KP_2: key = SDLK_DOWN; break;
						case SDLK_KP_4: key = SDLK_LEFT; break;
						case SDLK_KP_6: key = SDLK_RIGHT; break;
						case SDLK_KP_8: key = SDLK_UP; break;
					}
				}
			}

			if(SCANCODE_UNMASK(key)<SDLK_LAST)
				Keyboard[SCANCODE_UNMASK(key)] = 0;
			break;
		}

		/*case SDL_ACTIVEEVENT:
		{
			if(fullscreen && (event->active.state & SDL_APPACTIVE) != 0)
			{
					if(event->active.gain)
					{
						NeedRestore = false;
					}
					else NeedRestore = true;
			}
		}*/
	}
}

void IN_WaitAndProcessEvents()
{
	SDL_Event event;
	if(!SDL_WaitEvent(&event)) return;
	do
	{
		processEvent(&event);
	}
	while(SDL_PollEvent(&event));
}

void IN_ProcessEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		processEvent(&event);
	}
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_Startup() - Starts up the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Startup(void)
{
	if (IN_Started)
		return;

	IN_ClearKeysDown();

	if(param_joystickindex >= 0 && param_joystickindex < SDL_NumJoysticks())
	{
		Joystick = SDL_JoystickOpen(param_joystickindex);
		if(Joystick)
		{
			JoyNumButtons = SDL_JoystickNumButtons(Joystick);
			if(JoyNumButtons > 32) JoyNumButtons = 32;      // only up to 32 buttons are supported
			JoyNumAxes = SDL_JoystickNumAxes(Joystick);
			JoyNumHats = SDL_JoystickNumHats(Joystick);
			if(param_joystickhat >= JoyNumHats)
				Quit("The joystickhat param must be between 0 and %i!", JoyNumHats - 1);
			else if(param_joystickhat < 0 && JoyNumHats > 0) // Default to hat 0
				param_joystickhat = 0;
		}
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	IN_GrabMouse();

	// I didn't find a way to ask libSDL whether a mouse is present, yet...
	MousePresent = true;

	IN_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Shutdown() - Shuts down the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Shutdown(void)
{
	if (!IN_Started)
		return;

	if(Joystick)
		SDL_JoystickClose(Joystick);

	IN_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears the keyboard array
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	LastASCII = key_None;
	memset ((void *) Keyboard,0,sizeof(Keyboard));
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_ReadControl() - Reads the device associated with the specified
//		player and fills in the control info struct
//
///////////////////////////////////////////////////////////////////////////
void
IN_ReadControl(int player,ControlInfo *info)
{
	word		buttons;
	int			dx,dy;
	Motion		mx,my;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	IN_ProcessEvents();

	if (Keyboard[KbdDefs.upleft])
		mx = motion_Left,my = motion_Up;
	else if (Keyboard[KbdDefs.upright])
		mx = motion_Right,my = motion_Up;
	else if (Keyboard[KbdDefs.downleft])
		mx = motion_Left,my = motion_Down;
	else if (Keyboard[KbdDefs.downright])
		mx = motion_Right,my = motion_Down;

	if (Keyboard[KbdDefs.up])
		my = motion_Up;
	else if (Keyboard[KbdDefs.down])
		my = motion_Down;

	if (Keyboard[KbdDefs.left])
		mx = motion_Left;
	else if (Keyboard[KbdDefs.right])
		mx = motion_Right;

	if (Keyboard[KbdDefs.button0])
		buttons += 1 << 0;
	if (Keyboard[KbdDefs.button1])
		buttons += 1 << 1;

	dx = mx * 127;
	dy = my * 127;

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = (buttons & (1 << 0)) != 0;
	info->button1 = (buttons & (1 << 1)) != 0;
	info->button2 = (buttons & (1 << 2)) != 0;
	info->button3 = (buttons & (1 << 3)) != 0;
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//		returns the scan code
//
///////////////////////////////////////////////////////////////////////////
ScanCode
IN_WaitForKey(void)
{
	ScanCode	result;

	while ((result = LastScan)==0)
		IN_WaitAndProcessEvents();
	LastScan = 0;
	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForASCII() - Waits for an ASCII char, then clears LastASCII and
//		returns the ASCII value
//
///////////////////////////////////////////////////////////////////////////
char
IN_WaitForASCII(void)
{
	char		result;

	while ((result = LastASCII)==0)
		IN_WaitAndProcessEvents();
	LastASCII = '\0';
	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
///////////////////////////////////////////////////////////////////////////

bool	btnstate[NUMBUTTONS];

void IN_StartAck(void)
{
	IN_ProcessEvents();
//
// get initial state of everything
//
	IN_ClearKeysDown();
	memset(btnstate, 0, sizeof(btnstate));

	int buttons = IN_JoyButtons() << 4;

	if(MousePresent)
		buttons |= IN_MouseButtons();

	for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
		if(buttons & 1)
			btnstate[i] = true;
}


bool IN_CheckAck (void)
{
	IN_ProcessEvents();
//
// see if something has been pressed
//
	if(LastScan)
		return true;

	int buttons = IN_JoyButtons() << 4;

	if(MousePresent)
		buttons |= IN_MouseButtons();

	for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
	{
		if(buttons & 1)
		{
			if(!btnstate[i])
			{
				// Wait until button has been released
				do
				{
					IN_WaitAndProcessEvents();
					buttons = IN_JoyButtons() << 4;

					if(MousePresent)
						buttons |= IN_MouseButtons();
				}
				while(buttons & (1 << i));

				return true;
			}
		}
		else
			btnstate[i] = false;
	}

	return false;
}


void IN_Ack (void)
{
	IN_StartAck ();

	do
	{
		IN_WaitAndProcessEvents();
	}
	while(!IN_CheckAck ());
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_UserInput() - Waits for the specified delay time (in ticks) or the
//		user pressing a key or a mouse button. If the clear flag is set, it
//		then either clears the key or waits for the user to let the mouse
//		button up.
//
///////////////////////////////////////////////////////////////////////////
bool IN_UserInput(longword delay)
{
	longword	lasttime;

	lasttime = GetTimeCount();
	IN_StartAck ();
	do
	{
		IN_ProcessEvents();
		if (IN_CheckAck())
			return true;
		SDL_Delay(5);
	} while (GetTimeCount() - lasttime < delay);
	return(false);
}

//===========================================================================

/*
===================
=
= IN_MouseButtons
=
===================
*/
int IN_MouseButtons (void)
{
	if (MousePresent)
		return INL_GetMouseButtons();
	else
		return 0;
}

void IN_ReleaseMouse()
{
	GrabInput = false;
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void IN_GrabMouse()
{
	if(fullscreen || forcegrabmouse)
	{
		GrabInput = true;
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}

bool IN_IsInputGrabbed()
{
	return GrabInput;
}

void IN_CenterMouse()
{
	SDL_WarpMouseInWindow(NULL, screenWidth / 2, screenHeight / 2);
}
