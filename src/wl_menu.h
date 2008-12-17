//
// WL_MENU.H
//
#include <vector>

class Menu;

class MenuItem
{
	protected:
		bool		enabled;
		bool		highlight; // Makes the font a different color, not to be confused with the item being selected.
		bool		selected;
		char		string[36];
		bool		visible;
		const Menu	*menu;

		void	setTextColor();

	public:
				MenuItem(const char string[36]);
		virtual	~MenuItem() {}

		const short	getActive() { return enabled ? (highlight ? 2 : 1) : 0; }
		const char	*getString() { return string; }
		bool		isEnabled() { return enabled; }
		bool		isHighlighted() { return highlight; }
		bool		isSelected() { return selected; }
		bool		isVisible() { return visible; }
		void		setEnabled(bool enabled=true) { this->enabled = enabled; }
		void		setHighlighted(bool highlight=true) {this->highlight = highlight; }
		void		setMenu(const Menu *menu) { this->menu = menu; }
		void		setSelected(bool selected=true) {this->selected = selected; }
		void		setText(const char string[36]) { strcpy(this->string, string); }
		void		setVisible(bool visible=true) { this->visible = visible; }

		virtual void	activate() {}
		virtual void	draw();
		virtual void	left() {}
		virtual void	right() {}
};

class LabelMenuItem : public MenuItem
{
	public:
		LabelMenuItem(const char string[36]);

		void draw();
};

class BooleanMenuItem : public MenuItem
{
	protected:
		boolean	&value;

	public:
		BooleanMenuItem(const char string[36], boolean &value);

		void	activate();
		void	draw();
};

class FunctionMenuItem : public MenuItem
{
	protected:
		bool	fadeEnabled;
		int		(*function)(int);

	public:
		FunctionMenuItem(const char string[36], int (*function)(int), bool fadeEnabled=true);

		void	activate();
		void	setEnableFade(bool fadeEnabled=true) { this->fadeEnabled = fadeEnabled; }
};

class MenuSwitcherMenuItem : public MenuItem
{
	protected:
		Menu	&menu;

	public:
		MenuSwitcherMenuItem(const char string[36], Menu &menu);

		void	activate();
};

class SliderMenuItem : public MenuItem
{
	protected:
		char		begString[36];
		int			&value;
		const int	width;
		const int	max;

	public:
		SliderMenuItem(int &value, int width, int max, const char begString[36]="", const char endString[36]="");

		void	draw();
		void	left();
		void	right();
};

class Menu
{
	protected:
		int						curPos;
		void					(*handler)(unsigned int);
		int						headPicture;
		char					headText[36];
		const int				indent;
		std::vector<MenuItem *>	items;
		const int				x;
		const int				y;
		const int				w;

		void	drawGun(int x, int &y, int which, int basey);
		void	eraseGun(int x, int y, int which);

	public:
		Menu(int x, int y, int w, int indent, void (*handler)(unsigned int)=NULL, const char headText[36]="");
		~Menu();

		void				addItem(MenuItem *item);
		const unsigned int	countItems() const;
		void				drawMenu();
		void				draw();
		int					handle();
		const int			getHeight() const { return countItems()*13 + 6; }
		const int			getIndent() const { return indent; }
		MenuItem			*getIndex(int index);
		const int			getWidth() const { return w; }
		const int			getX() const { return x; }
		const int			getY() const { return y; }
		void				setHeadPicture(int picture) { headPicture = picture; }
		void				setHeadText(const char text[36]) { strcpy(headText, text); }
		void				show();
		MenuItem			*operator[] (int index) { return getIndex(index); }
};

extern Menu mainMenu;

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
// TYPEDEFS
//
typedef struct {
                short x,y,amount,curpos,indent;
                } CP_iteminfo;

typedef struct {
                short active;
                char string[36];
                int (* routine)(int temp1);
                } CP_itemtype;

typedef struct {
                short allowed[4];
                } CustomCtrls;

//
// FUNCTION PROTOTYPES
//

void CreateMenus();

void US_ControlPanel(ScanCode);

void SetupControlPanel(void);
void SetupSaveGames();
void CleanupControlPanel(void);

void DrawMenu(CP_iteminfo *item_i,CP_itemtype *items);
int  HandleMenu(CP_iteminfo *item_i,
                CP_itemtype *items,
                void (*routine)(int w));
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

void DrawGun(CP_iteminfo *item_i,CP_itemtype *items,int x,int *y,int which,int basey,void (*routine)(int w));
void DrawHalfStep(int x,int y);
void EraseGun(CP_iteminfo *item_i,CP_itemtype *items,int x,int y,int which);
void SetTextColor(CP_itemtype *items,int hlight);
void DrawMenuGun(CP_iteminfo *iteminfo);
void DrawStripes(int y);

void DefineMouseBtns(void);
void DefineJoyBtns(void);
void DefineKeyBtns(void);
void DefineKeyMove(void);
void EnterCtrlData(int index,CustomCtrls *cust,void (*DrawRtn)(int),void (*PrintRtn)(int),int type);

void DrawSoundMenu(void);
void DrawLoadSaveScreen(int loadsave);
void DrawNewEpisode(void);
void DrawNewGame(void);
void DrawChangeView(int view);
void DrawMouseSens(void);
void DrawCustomScreen(void);
void DrawLSAction(int which);
void DrawCustMouse(int hilight);
void DrawCustJoy(int hilight);
void DrawCustKeybd(int hilight);
void DrawCustKeys(int hilight);
void PrintCustMouse(int i);
void PrintCustJoy(int i);
void PrintCustKeybd(int i);
void PrintCustKeys(int i);

void PrintLSEntry(int w,int color);
void TrackWhichGame(int w);
void DrawNewGameDiff(int w);
void FixupCustom(int w);

int CP_NewGame(int);
int CP_Sound(int);
int  CP_LoadGame(int quick);
int  CP_SaveGame(int quick);
int CP_Control(int);
int CP_ChangeView(int);
int CP_ExitOptions(int);
int CP_Quit(int);
int CP_ViewScores(int);
int  CP_EndGame(int);
int  CP_CheckQuick(ScanCode scancode);
int CustomControls(int);
int MouseSensitivity(int);

void CheckForEpisodes(void);

void FreeMusic(void);


enum {MOUSE,JOYSTICK,KEYBOARDBTNS,KEYBOARDMOVE};        // FOR INPUT TYPES

enum menuitems
{
        newgame,
        soundmenu,
        control,
        loadgame,
        savegame,
        changeview,

#ifndef GOODTIMES
#ifndef SPEAR
        readthis,
#endif
#endif

        viewscores,
        backtodemo,
        quit
};

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
