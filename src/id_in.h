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

#define KEYCODE_UNMASK(x) (((x)&(~(1<<30)))|0x100)
#define SCANCODE_UNMASK(x) ((x)&(1<<30) ? KEYCODE_UNMASK(x) : (x))
typedef	int		ScanCode;
#define	sc_None			0
#define	sc_Bad			0xff
#define	sc_Return		SCANCODE_UNMASK(SDLK_RETURN)
#define	sc_Enter		sc_Return
#define	sc_Escape		SCANCODE_UNMASK(SDLK_ESCAPE)
#define	sc_Space		SCANCODE_UNMASK(SDLK_SPACE)
#define	sc_BackSpace	SCANCODE_UNMASK(SDLK_BACKSPACE)
#define	sc_Tab			SCANCODE_UNMASK(SDLK_TAB)
#define	sc_Alt			SCANCODE_UNMASK(SDLK_LALT)
#define	sc_Control		SCANCODE_UNMASK(SDLK_LCTRL)
#define	sc_CapsLock		SCANCODE_UNMASK(SDLK_CAPSLOCK)
#define	sc_LShift		SCANCODE_UNMASK(SDLK_LSHIFT)
#define	sc_RShift		SCANCODE_UNMASK(SDLK_RSHIFT)
#define	sc_UpArrow		SCANCODE_UNMASK(SDLK_UP)
#define	sc_DownArrow	SCANCODE_UNMASK(SDLK_DOWN)
#define	sc_LeftArrow	SCANCODE_UNMASK(SDLK_LEFT)
#define	sc_RightArrow	SCANCODE_UNMASK(SDLK_RIGHT)
#define	sc_Insert		SCANCODE_UNMASK(SDLK_INSERT)
#define	sc_Delete		SCANCODE_UNMASK(SDLK_DELETE)
#define	sc_Home			SCANCODE_UNMASK(SDLK_HOME)
#define	sc_End			SCANCODE_UNMASK(SDLK_END)
#define	sc_PgUp			SCANCODE_UNMASK(SDLK_PAGEUP)
#define	sc_PgDn			SCANCODE_UNMASK(SDLK_PAGEDOWN)
#define	sc_F1			SCANCODE_UNMASK(SDLK_F1)
#define	sc_F2			SCANCODE_UNMASK(SDLK_F2)
#define	sc_F3			SCANCODE_UNMASK(SDLK_F3)
#define	sc_F4			SCANCODE_UNMASK(SDLK_F4)
#define	sc_F5			SCANCODE_UNMASK(SDLK_F5)
#define	sc_F6			SCANCODE_UNMASK(SDLK_F6)
#define	sc_F7			SCANCODE_UNMASK(SDLK_F7)
#define	sc_F8			SCANCODE_UNMASK(SDLK_F8)
#define	sc_F9			SCANCODE_UNMASK(SDLK_F9)
#define	sc_F10			SCANCODE_UNMASK(SDLK_F10)
#define	sc_F11			SCANCODE_UNMASK(SDLK_F11)
#define	sc_F12			SCANCODE_UNMASK(SDLK_F12)

#define sc_ScrollLock		SCANCODE_UNMASK(SDLK_SCROLLOCK)
#define sc_PrintScreen		SCANCODE_UNMASK(SDLK_PRINT)

#define	sc_1			SCANCODE_UNMASK(SDLK_1)
#define	sc_2			SCANCODE_UNMASK(SDLK_2)
#define	sc_3			SCANCODE_UNMASK(SDLK_3)
#define	sc_4			SCANCODE_UNMASK(SDLK_4)
#define	sc_5			SCANCODE_UNMASK(SDLK_5)
#define	sc_6			SCANCODE_UNMASK(SDLK_6)
#define	sc_7			SCANCODE_UNMASK(SDLK_7)
#define	sc_8			SCANCODE_UNMASK(SDLK_8)
#define	sc_9			SCANCODE_UNMASK(SDLK_9)
#define	sc_0			SCANCODE_UNMASK(SDLK_0)

#define	sc_A			SCANCODE_UNMASK(SDLK_a)
#define	sc_B			SCANCODE_UNMASK(SDLK_b)
#define	sc_C			SCANCODE_UNMASK(SDLK_c)
#define	sc_D			SCANCODE_UNMASK(SDLK_d)
#define	sc_E			SCANCODE_UNMASK(SDLK_e)
#define	sc_F			SCANCODE_UNMASK(SDLK_f)
#define	sc_G			SCANCODE_UNMASK(SDLK_g)
#define	sc_H			SCANCODE_UNMASK(SDLK_h)
#define	sc_I			SCANCODE_UNMASK(SDLK_i)
#define	sc_J			SCANCODE_UNMASK(SDLK_j)
#define	sc_K			SCANCODE_UNMASK(SDLK_k)
#define	sc_L			SCANCODE_UNMASK(SDLK_l)
#define	sc_M			SCANCODE_UNMASK(SDLK_m)
#define	sc_N			SCANCODE_UNMASK(SDLK_n)
#define	sc_O			SCANCODE_UNMASK(SDLK_o)
#define	sc_P			SCANCODE_UNMASK(SDLK_p)
#define	sc_Q			SCANCODE_UNMASK(SDLK_q)
#define	sc_R			SCANCODE_UNMASK(SDLK_r)
#define	sc_S			SCANCODE_UNMASK(SDLK_s)
#define	sc_T			SCANCODE_UNMASK(SDLK_t)
#define	sc_U			SCANCODE_UNMASK(SDLK_u)
#define	sc_V			SCANCODE_UNMASK(SDLK_v)
#define	sc_W			SCANCODE_UNMASK(SDLK_w)
#define	sc_X			SCANCODE_UNMASK(SDLK_x)
#define	sc_Y			SCANCODE_UNMASK(SDLK_y)
#define	sc_Z			SCANCODE_UNMASK(SDLK_z)

#define sc_Equals		SCANCODE_UNMASK(SDLK_EQUALS)
#define sc_Minus		SCANCODE_UNMASK(SDLK_MINUS)

#define sc_Comma		SCANCODE_UNMASK(SDLK_COMMA)
#define sc_Peroid		SCANCODE_UNMASK(SDLK_PERIOD)

#define	key_None		0

typedef	enum		{
						demo_Off,demo_Record,demo_Playback,demo_PlayDone
					} Demo;
typedef	enum		{
						ctrl_Keyboard,
						ctrl_Keyboard1 = ctrl_Keyboard,ctrl_Keyboard2,
						ctrl_Joystick,
						ctrl_Joystick1 = ctrl_Joystick,ctrl_Joystick2,
						ctrl_Mouse
					} ControlType;
typedef	enum		{
						motion_Left = -1,motion_Up = -1,
						motion_None = 0,
						motion_Right = 1,motion_Down = 1
					} Motion;
typedef	enum		{
						dir_North,dir_NorthEast,
						dir_East,dir_SouthEast,
						dir_South,dir_SouthWest,
						dir_West,dir_NorthWest,
						dir_None
					} Direction;
typedef	struct		{
						bool		button0,button1,button2,button3;
						short		x,y;
						Motion		xaxis,yaxis;
						Direction	dir;
					} CursorInfo;
typedef	CursorInfo	ControlInfo;
typedef	struct		{
						ScanCode	button0,button1,
									upleft,		up,		upright,
									left,				right,
									downleft,	down,	downright;
					} KeyboardDef;
typedef	struct		{
						word		joyMinX,joyMinY,
									threshMinX,threshMinY,
									threshMaxX,threshMaxY,
									joyMaxX,joyMaxY,
									joyMultXL,joyMultYL,
									joyMultXH,joyMultYH;
					} JoystickDef;
// Global variables
extern  volatile bool		Keyboard[];
extern           bool		MousePresent;
extern  volatile bool		Paused;
extern  volatile char       LastASCII;
extern  volatile ScanCode   LastScan;
extern           int        JoyNumButtons;


// Function prototypes
#define	IN_KeyDown(code)	(Keyboard[(code)])
#define	IN_ClearKey(code)	{Keyboard[code] = false;\
							if (code == LastScan) LastScan = sc_None;}

// DEBUG - put names in prototypes
extern	void		IN_Startup(void),IN_Shutdown(void);
extern	void		IN_ClearKeysDown(void);
extern	void		IN_ReadControl(int,ControlInfo *);
extern	void		IN_GetJoyAbs(word joy,word *xp,word *yp);
extern	void		IN_SetupJoy(word joy,word minx,word maxx,
								word miny,word maxy);
extern	void		IN_StopDemo(void),IN_FreeDemoBuffer(void),
					IN_Ack(void);
extern	bool		IN_UserInput(longword delay);
extern	char		IN_WaitForASCII(void);
extern	ScanCode	IN_WaitForKey(void);
extern	word		IN_GetJoyButtonsDB(word joy);
extern	const char *IN_GetScanName(ScanCode);

void    IN_WaitAndProcessEvents();
void    IN_ProcessEvents();

int     IN_MouseButtons (void);
void	IN_ReleaseMouse();
void	IN_GrabMouse();

bool	IN_JoyPresent();
void    IN_SetJoyCurrent(int joyIndex);
int     IN_JoyButtons (void);
int     IN_JoyAxes (void);
void    IN_GetJoyDelta(int *dx,int *dy);
void    IN_GetJoyFineDelta(int *dx, int *dy);
int		IN_GetJoyAxis(int axis);

void    IN_StartAck(void);
bool	IN_CheckAck (void);
bool    IN_IsInputGrabbed();
void    IN_CenterMouse();

#endif
