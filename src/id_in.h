//
//	ID Engine
//	ID_IN.h - Header file for Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

#ifndef	__ID_IN__
#define	__ID_IN__

#ifdef	__DEBUG__
#define	__DEBUG_InputMgr__
#endif

typedef	int		ScanCode;


#define	key_None		0

enum Demo {
	demo_Off,demo_Record,demo_Playback,demo_PlayDone
};
enum ControlType {
	ctrl_Keyboard,
	ctrl_Keyboard1 = ctrl_Keyboard,ctrl_Keyboard2,
	ctrl_Joystick,
	ctrl_Joystick1 = ctrl_Joystick,ctrl_Joystick2,
	ctrl_Mouse
};
enum Motion {
	motion_Left = -1,motion_Up = -1,
	motion_None = 0,
	motion_Right = 1,motion_Down = 1
};
enum Direction {
	dir_North,dir_NorthEast,
	dir_East,dir_SouthEast,
	dir_South,dir_SouthWest,
	dir_West,dir_NorthWest,
	dir_None
};
struct CursorInfo {
	bool		button0,button1,button2,button3;
	short		x,y;
	Motion		xaxis,yaxis;
	Direction	dir;
};
typedef	CursorInfo	ControlInfo;
struct KeyboardDef {
	ScanCode	button0,button1,
				upleft,		up,		upright,
				left,				right,
				downleft,	down,	downright;
};

struct JoystickSens
{
	int sensitivity;
	int deadzone;
};
extern JoystickSens *JoySensitivity;

// Global variables
extern bool Keyboard[];
extern bool MousePresent;
extern bool MouseWheel[4];
extern unsigned short Paused;
extern char LastASCII;
extern ScanCode LastScan;
extern int JoyNumButtons;
extern int JoyNumAxes;


// Function prototypes

void IN_Startup();
void IN_Shutdown();
void IN_ClearKeysDown();
void IN_ClearWheel();
void IN_ReadControl(int,ControlInfo *);
void IN_GetJoyAbs(word joy,word *xp,word *yp);
void IN_SetupJoy(word joy,word minx,word maxx,word miny,word maxy);
void IN_StopDemo();
void IN_FreeDemoBuffer();
void IN_Ack();
bool IN_UserInput(longword delay);
char IN_WaitForASCII();
ScanCode IN_WaitForKey();
word IN_GetJoyButtonsDB(word joy);
const char *IN_GetScanName(ScanCode);

void IN_WaitAndProcessEvents();
void IN_ProcessEvents();

int IN_MouseButtons (void);
void IN_ReleaseMouse();
void IN_GrabMouse();
void IN_AdjustMouse();

bool IN_JoyPresent();
void IN_SetJoyCurrent(int joyIndex);
int IN_JoyButtons (void);
int IN_JoyAxes (void);
void IN_GetJoyDelta(int *dx,int *dy);
int IN_GetJoyAxis(int axis);

void IN_StartAck(void);
bool IN_CheckAck (void);
bool IN_IsInputGrabbed();
void IN_CenterMouse();

#endif
