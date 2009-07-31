#include "m_classes.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"

static const int color_hlite[] = {
    DEACTIVE,
    HIGHLIGHT,
    READHCOLOR,
    0x67
};

static const int color_norml[] = {
    DEACTIVE,
    TEXTCOLOR,
    READCOLOR,
    0x6b
};

bool 			Menu::close = false;
unsigned int	Menu::lastIndexDrawn = 0;

void MenuItem::setTextColor()
{
	if (isSelected())
	{
		SETFONTCOLOR(color_hlite[getActive()], BKGDCOLOR);
	}
	else
	{
		SETFONTCOLOR(color_norml[getActive()], BKGDCOLOR);
	}
}

MenuItem::MenuItem(const char string[36], MENU_LISTENER_PROTOTYPE(activateListener)) : activateListener(activateListener), enabled(true), highlight(false), picture(-1), selected(false), visible(true), pictureX(-1), pictureY(-1)
{
	setText(string);
}

void MenuItem::activate()
{
	if(activateListener != NULL)
		activateListener(menu->getCurrentPosition());
}

void MenuItem::draw()
{
	setTextColor();

	if(picture != -1)
		VWB_DrawPic(pictureX == -1 ? menu->getX() + 32 : pictureX, pictureY == -1 ? PrintY : pictureY, picture);

	if(getActive())
		US_Print(getString());
	else
	{
		SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
		US_Print(getString());
		SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
	}

	PrintX = menu->getX() + menu->getIndent();
}

void MenuItem::setPicture(int picture, int x, int y)
{
	this->picture = picture;
	pictureX = x;
	pictureY = y;
}

void MenuItem::setText(const char string[36])
{
	height = 13;
	strcpy(this->string, string);
	for(unsigned int i = 0;i < 36;i++)
	{
		if(string[i] == '\n')
			height += 13;
		else if(string[i] == '\0')
			break;
	}
}

LabelMenuItem::LabelMenuItem(const char string[36]) : MenuItem(string)
{
	setEnabled(false);
}

void LabelMenuItem::draw()
{
	int oldWindowX = WindowX;
	int oldWindowY = WindowY;
	SETFONTCOLOR (READCOLOR, BKGDCOLOR);
	WindowX = menu->getX();
	WindowW = menu->getWidth();
	US_CPrint(string);
	WindowX = oldWindowX;
	WindowY = oldWindowY;
}

BooleanMenuItem::BooleanMenuItem(const char string[36], boolean &value, MENU_LISTENER_PROTOTYPE(activateListener)) : MenuItem(string, activateListener), value(value)
{
}

void BooleanMenuItem::activate()
{
	value ^= 1;
	MenuItem::activate();
}

void BooleanMenuItem::draw()
{
	if (value)
		VWB_DrawPic (PrintX - 24, PrintY + 3, C_SELECTEDPIC);
	else
		VWB_DrawPic (PrintX - 24, PrintY + 3, C_NOTSELECTEDPIC);
	MenuItem::draw();
}

MenuSwitcherMenuItem::MenuSwitcherMenuItem(const char string[36], Menu &menu, MENU_LISTENER_PROTOTYPE(activateListener)) : MenuItem(string, activateListener), menu(menu)
{
}

void MenuSwitcherMenuItem::activate()
{
	// If there is an activateListener then use it to determine if the menu should switch
	if(activateListener == NULL || activateListener(MenuItem::menu->getCurrentPosition()))
	{
		MenuFadeOut();
		menu.show();
		if(!Menu::areMenusClosed())
		{
			MenuItem::menu->draw();
			MenuFadeIn();
		}
	}
}

SliderMenuItem::SliderMenuItem(int &value, int width, int max, const char begString[36], const char endString[36]) : MenuItem(endString), value(value), width(width), max(max)
{
	strcpy(this->begString, begString);
}

void SliderMenuItem::draw()
{
	setTextColor();

	if (getActive())
		US_Print(begString);
	else
	{
		SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
		US_Print(begString);
		SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
	}
	PrintX += 8;

	VWB_Bar(PrintX, PrintY + 1, width, 10, TEXTCOLOR);
	DrawOutline(PrintX, PrintY + 1, width, 10, 0, HIGHLIGHT);

	//calc position
	int x = int(ceil((double(width-20)/double(max))*double(value)));
	x -= x+20 >= width ? 1 : 0;

	DrawOutline(PrintX + x, PrintY + 1, 20, 10, 0, READCOLOR);
	VWB_Bar(PrintX + 1 + x, PrintY + 2, 19, 9, READHCOLOR);

	PrintX += width+8;
	MenuItem::draw();
}

void SliderMenuItem::left()
{
	value -= value > 0 ? 1 : 0;
	SD_PlaySound(MOVEGUN1SND);
}

void SliderMenuItem::right()
{
	value += value < max ? 1 : 0;
	SD_PlaySound(MOVEGUN1SND);
}

MultipleChoiceMenuItem::MultipleChoiceMenuItem(MENU_LISTENER_PROTOTYPE(changeListener), const char** options, int numOptions, int curOption) : MenuItem("", changeListener), numOptions(numOptions), curOption(curOption)
{
	// Copy all of the options
	this->options = new char *[numOptions];
	for(unsigned int i = 0;i < numOptions;i++)
	{
		if(options[i] == NULL)
			this->options[i] = NULL;
		else
		{
			this->options[i] = new char[strlen(options[i])+1];
			strcpy(this->options[i], options[i]);
		}
	}

	// clamp current option
	if(curOption < 0)
		curOption = 0;
	else if(curOption >= numOptions)
		curOption = numOptions-1;

	while(options[curOption] == NULL)
	{
		curOption--; // Easier to go backwards
		if(curOption < 0)
			curOption = numOptions-1;
	}

	if(numOptions > 0)
		setText(options[curOption]);
}

MultipleChoiceMenuItem::~MultipleChoiceMenuItem()
{
	for(unsigned int i = 0;i < numOptions;i++)
		delete[] options[i];
	delete[] options;
}

void MultipleChoiceMenuItem::activate()
{
	if(activateListener != NULL)
		activateListener(curOption);
}

void MultipleChoiceMenuItem::draw()
{
	VWB_Bar(PrintX, PrintY, menu->getWidth()-menu->getIndent()-menu->getX(), 13, BKGDCOLOR);
	MenuItem::draw();
}

void MultipleChoiceMenuItem::left()
{
	do
	{
		curOption--;
		if(curOption < 0)
			curOption = numOptions-1;
	}
	while(options[curOption] == NULL);
	setText(options[curOption]);
	activate();
	SD_PlaySound(MOVEGUN1SND);
}

void MultipleChoiceMenuItem::right()
{
	do
	{
		curOption++;
		if(curOption >= numOptions)
			curOption = 0;
	}
	while(options[curOption] == NULL);
	setText(options[curOption]);
	activate();
	SD_PlaySound(MOVEGUN1SND);
}

TextInputMenuItem::TextInputMenuItem(std::string text, unsigned int max, MENU_LISTENER_PROTOTYPE(preeditListener), MENU_LISTENER_PROTOTYPE(posteditListener), bool clearFirst) : MenuItem("", posteditListener), clearFirst(clearFirst), max(max), preeditListener(preeditListener)
{
	setValue(text);
}

void TextInputMenuItem::activate()
{
	if(preeditListener == NULL || preeditListener(menu->getCurrentPosition()))
	{
		setTextColor();
		fontnumber = 0;
		char* buffer = new char[max+1];
		if(clearFirst)
			VWB_Bar(menu->getX() + menu->getIndent() + 1, PrintY + 1, menu->getWidth() - menu->getIndent() - 14, 8, BKGDCOLOR);
		bool accept = US_LineInput(menu->getX() + menu->getIndent() + 2, PrintY, buffer, clearFirst ? "" : getValue(), true, max, menu->getWidth() - menu->getIndent() - 16);
		if(accept)
			setValue(buffer);
		delete[] buffer;
		fontnumber = 1;
		if(accept)
			MenuItem::activate();
		else
		{
			SD_PlaySound(ESCPRESSEDSND);
			VWB_Bar(menu->getX() + menu->getIndent() + 1, PrintY + 1, menu->getWidth() - menu->getIndent() - 14, 10, BKGDCOLOR);
			PrintY--;
			draw();
		}
	}
}

void TextInputMenuItem::draw()
{
	setTextColor();
	DrawOutline(menu->getX() + menu->getIndent(), PrintY, menu->getWidth() - menu->getIndent() - 12, 11, fontcolor, fontcolor);
	PrintX = menu->getX() + menu->getIndent() + 2;
	PrintY++;
	fontnumber = 0;
	US_Print(getValue());
	fontnumber = 1;
}

int ControlMenuItem::column = 0;
const char* const ControlMenuItem::keyNames[SDLK_LAST] =
{
	"?","?","?","?","?","?","?","?",                                //   0
	"BkSp","Tab","?","?","?","Ret","?","?",                      //   8
	"?","?","?","Paus","?","?","?","?",                            //  16
	"?","?","?","Esc","?","?","?","?",                              //  24
	"Spce","!","\"","#","$","?","&","'",                           //  32
	"(",")","*","+",",","-",".","/",                                //  40
	"0","1","2","3","4","5","6","7",                                //  48
	"8","9",":",";","<","=",">","?",                                //  56
	"@","A","B","C","D","E","F","G",                                //  64
	"H","I","J","K","L","M","N","O",                                //  72
	"P","Q","R","S","T","U","V","W",                                //  80
	"X","Y","Z","[","\\","]","^","_",                               //  88
	"`","a","b","c","d","e","f","h",                                //  96
	"h","i","j","k","l","m","n","o",                                // 104
	"p","q","r","s","t","u","v","w",                                // 112
	"x","y","z","{","|","}","~","?",                                // 120
	"?","?","?","?","?","?","?","?",                                // 128
	"?","?","?","?","?","?","?","?",                                // 136
	"?","?","?","?","?","?","?","?",                                // 144
	"?","?","?","?","?","?","?","?",                                // 152
	"?","?","?","?","?","?","?","?",                                // 160
	"?","?","?","?","?","?","?","?",                                // 168
	"?","?","?","?","?","?","?","?",                                // 176
	"?","?","?","?","?","?","?","?",                                // 184
	"?","?","?","?","?","?","?","?",                                // 192
	"?","?","?","?","?","?","?","?",                                // 200
	"?","?","?","?","?","?","?","?",                                // 208
	"?","?","?","?","?","?","?","?",                                // 216
	"?","?","?","?","?","?","?","?",                                // 224
	"?","?","?","?","?","?","?","?",                                // 232
	"?","?","?","?","?","?","?","?",                                // 240
	"?","?","?","?","?","?","?","?",                                // 248
	"?","?","?","?","?","?","?","?",                                // 256
	"?","?","?","?","?","?","?","Entr",                            // 264
	"?","Up","Down","Rght","Left","Ins","Home","End",              // 272
	"PgUp","PgDn","F1","F2","F3","F4","F5","F6",                    // 280
	"F7","F8","F9","F10","F11","F12","?","?",                       // 288
	"?","?","?","?","NmLk","CpLk","ScLk","RShf",              // 296
	"Shft","RCtl","Ctrl","RAlt","Alt","?","?","?",                // 304
	"?","?","?","?","PrtS","?","?","?",                            // 312
	"?","?"                                                         // 320
};

ControlMenuItem::ControlMenuItem(ControlScheme &button) : MenuItem(button.name), button(button)
{
}

void ControlMenuItem::activate()
{
	DrawOutline(159 + (52*column), PrintY, 49, 12, 0, HIGHLIGHT);
	VWB_Bar(160 + (52*column), PrintY + 1, 50 - 2, 11, TEXTCOLOR);
	PrintX = 162 + (52*column);
	US_Print("???");
	VW_UpdateScreen();

	IN_ClearKeysDown();
	ControlInfo ci;
	bool exit = false;
	int btn = 0;
	while(!exit)
	{
		SDL_Delay(5);

		switch(column)
		{
			default:
			case 0:
				if(LastScan != 0)
				{
					ControlScheme::setKeyboard(controlScheme, button.button, LastScan);
					ShootSnd();
					IN_ClearKeysDown();
					exit = true;
				}
				break;
			case 1:
				if(!mouseenabled)
				{
					exit = true;
					break;
				}

				btn = IN_MouseButtons();
				for(int i = 0;btn != 0 && i < 32;i++)
				{
					if(btn & (1<<i))
					{
						ControlScheme::setMouse(controlScheme, button.button, i);
						exit = true;
					}
				}
				break;
			case 2:
				if(!joystickenabled)
				{
					exit = true;
					break;
				}

				btn = IN_JoyButtons();
				for(int i = 0;btn != 0 && i < 32;i++)
				{
					if(btn & (1<<i))
					{
						ControlScheme::setJoystick(controlScheme, button.button, i);
						exit = true;
					}
				}
				break;
		}

		ReadAnyControl(&ci);
		if(LastScan == sc_Escape)
			break;
	}

	PrintX = menu->getX() + menu->getIndent();

	MenuItem::activate();

	// Setting one vale could affect another
	menu->draw();
}

void ControlMenuItem::draw()
{
	VWB_Bar(159, PrintY, ((52)*3) - 1, 13, BKGDCOLOR);
	if(isSelected())
	{
		DrawOutline(159 + (52*column), PrintY, 49, 12, 0, HIGHLIGHT);
		VWB_Bar(160 + (52*column), PrintY + 1, 50 - 2, 11, TEXTCOLOR);
	}

	setTextColor();

	if(!getActive())
	{
		SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
	}

	US_Print(getString());

	if(button.keyboard != -1)
	{
		PrintX = 162;
		US_Print(keyNames[button.keyboard]);
	}
	if(button.mouse != -1)
	{
		PrintX = 214;
		char btn[8];
		sprintf(btn, "MS%d", button.mouse);
		US_Print(btn);
	}
	if(button.joystick != -1)
	{
		PrintX = 266;
		char btn[8];
		sprintf(btn, "JY%d", button.mouse);
		US_Print(btn);
	}

	if(!getActive())
	{
		SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
	}

	PrintX = menu->getX() + menu->getIndent();
}

void ControlMenuItem::left()
{
	if(column != 0)
		column--;
}

void ControlMenuItem::right()
{
	if(column != 2)
		column++;
}

void Menu::drawGun(int x, int &y, int basey)
{
	VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
	y = getY() + getHeight(curPos);
	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);

	PrintX = getX() + getIndent();
	PrintY = getY() + getHeight(curPos);
	getIndex(curPos)->setSelected(true);
	getIndex(curPos)->draw();

	VW_UpdateScreen();
	SD_PlaySound(MOVEGUN2SND);
}

void Menu::drawGunHalfStep(int x, int y)
{
	VWB_DrawPic (x, y, C_CURSOR1PIC);
	VW_UpdateScreen ();
	SD_PlaySound (MOVEGUN1SND);
	SDL_Delay (8 * 100 / 7);
}

void Menu::eraseGun(int x, int y)
{
	VWB_Bar(x - 1, y, 25, 16, BKGDCOLOR);

	PrintX = getX() + getIndent();
	PrintY = getY() + getHeight(curPos);
	getIndex(curPos)->setSelected(false);
	getIndex(curPos)->draw();
	VW_UpdateScreen();
}

Menu::Menu(int x, int y, int w, int indent, MENU_LISTENER_PROTOTYPE(entryListener)) : x(x), y(y), w(w), entryListener(entryListener), indent(indent), headPicture(-1), curPos(0), height(0), itemOffset(0), headTextInStripes(false)
{
	for(unsigned int i = 0;i < 36;i++)
		headText[i] = '\0';
}
Menu::~Menu()
{
	clear();
}

void Menu::addItem(MenuItem *item)
{
	item->setMenu(this);
	items.push_back(item);
	if(!item->isEnabled() && items.size()-1 == curPos)
		curPos++;
	height += item->getHeight();
}

void Menu::clear()
{
	for(unsigned int i = 0;i < items.size();i++)
		delete items[i];
	items.clear();
}

unsigned int Menu::countItems() const
{
	unsigned int num = 0;
	for(unsigned int i = 0;i < items.size();i++)
	{
		if(items[i]->isVisible())
			num++;
	}
	return num;
}

int Menu::getHeight(int position) const
{
	// Make sure we have the position we think we have.
	if(position != -1)
	{
		for(unsigned int i = 0;i < items.size() && i < position;i++)
		{
			if(!items[i]->isVisible())
				position++;
		}
	}

	unsigned int num = 0;
	for(unsigned int i = itemOffset;i < items.size();i++)
	{
		if(position == i)
			break;

		if(items[i]->isVisible())
		{
			if(getY() + num + items[i]->getHeight() + 6 >= 200)
				break;
			num += items[i]->getHeight();
		}
	}
	if(position >= 0)
		return num;
	return num + 6;
}

MenuItem *Menu::getIndex(int index) const
{
	unsigned int idx = 0;
	for(idx = 0;idx < items.size() && index >= 0;idx++)
	{
		if(items[idx]->isVisible())
			index--;
	}
	idx--;
	return idx >= items.size() ? items[items.size()-1] : items[idx];
}

void Menu::drawMenu() const
{
	lastIndexDrawn = 0;

	WindowX = PrintX = getX() + getIndent();
	WindowY = PrintY = getY();
	WindowW = 320;
	WindowH = 200;

	PrintY = getY();
	int y = getY();
	int selectedY = getY(); // See later

	for (unsigned int i = itemOffset; i < countItems(); i++)
	{
		if(i == curPos)
			selectedY = y;
		else
		{
			PrintY = y;
			if(PrintY + getIndex(i)->getHeight() + 6 >= 200)
				break;
			getIndex(i)->draw();
			lastIndexDrawn = i;
		}
		y += getIndex(i)->getHeight();
	}

	// In order to draw the skill menu correctly we need to draw the selected option now
	if(curPos >= itemOffset)
	{
		PrintY = selectedY;
		getIndex(curPos)->setSelected(true);
		getIndex(curPos)->draw();
		if(curPos > lastIndexDrawn)
			lastIndexDrawn = curPos;
	}
}

void Menu::draw() const
{
	ClearMScreen();
	if(headPicture != -1)
	{
		DrawStripes(10);
		VWB_DrawPic(84, 0, headPicture);
	}
	VWB_DrawPic(112, 184, C_MOUSELBACKPIC);

	SETFONTCOLOR (READHCOLOR, BKGDCOLOR);
	WindowX = 0;
	WindowW = 320;
    PrintY = getY() - 22;
	if(controlHeaders)
	{
		PrintX = getX() + getIndent();
		US_Print("Control");
		PrintX = 168;
		US_Print("Key");
		PrintX = 220;
		US_Print("Mse");
		PrintX = 272;
		US_Print("Joy");
	}
	else
	{
		if(headTextInStripes)
		{
			DrawStripes(10);
			PrintY = 15;
		}
		US_CPrint(headText);
	}

	DrawWindow(getX() - 8, getY() - 3, getWidth(), getHeight(), BKGDCOLOR);
	drawMenu();
	VW_UpdateScreen ();
}

int Menu::handle()
{
	char key;
	static int redrawitem = 1, lastitem = -1;
	int i, x, y, basey, exit, shape;
	int32_t lastBlinkTime, timer;
	ControlInfo ci;

	if(close)
		return -1;

	x = getX() & -8;
	basey = getY() - 2;
	y = basey + getHeight(curPos);

	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);
	if (redrawitem)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
	}
	VW_UpdateScreen ();

	shape = C_CURSOR1PIC;
	timer = 8;
	exit = 0;
	lastBlinkTime = GetTimeCount ();
	IN_ClearKeysDown ();


	do
	{
		//
		// CHANGE GUN SHAPE
		//
		if (getIndent() != 0 && (int32_t)GetTimeCount () - lastBlinkTime > timer)
		{
			lastBlinkTime = GetTimeCount ();
			if (shape == C_CURSOR1PIC)
			{
				shape = C_CURSOR2PIC;
				timer = 8;
			}
			else
			{
				shape = C_CURSOR1PIC;
				timer = 70;
			}
			VWB_DrawPic (x, y, shape);
			VW_UpdateScreen ();
		}
		else SDL_Delay(5);

		CheckPause ();

		//
		// SEE IF ANY KEYS ARE PRESSED FOR INITIAL CHAR FINDING
		//
		key = LastASCII;
		if (key)
		{
			int ok = 0;

			if (key >= 'a')
				key -= 'a' - 'A';

			for (i = curPos + 1; i < countItems(); i++)
				if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
				{
					eraseGun(x, y);
					curPos = i;
					drawGun(x, y, basey);
					ok = 1;
					IN_ClearKeysDown ();
					break;
				}

			//
			// DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN.
			//
			if (!ok)
			{
				for (i = 0; i < curPos; i++)
				{
					if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
					{
						eraseGun(x, y);
						curPos = i;
						drawGun(x, y, basey);
						IN_ClearKeysDown ();
						break;
					}
				}
			}
		}

		//
		// GET INPUT
		//
		ReadAnyControl (&ci);
		switch (ci.dir)
		{
				////////////////////////////////////////////////
				//
				// MOVE UP
				//
			case dir_North:
				if(countItems() <= 1)
					break;

				eraseGun(x, y);

				//
				// ANIMATE HALF-STEP
				//
				if (curPos != itemOffset && getIndex(curPos - 1)->isEnabled())
				{
					y -= 6;
					drawGunHalfStep(x, y);
				}
				if (curPos == itemOffset && curPos != 0)
				{
					itemOffset--;
					draw();
					getIndex(curPos)->setSelected(false); // Undo a change made by draw.
					PrintY = getY() + getHeight(curPos);
					getIndex(curPos)->draw();
				}

				//
				// MOVE TO NEXT AVAILABLE SPOT
				//
				do
				{
					if (curPos == 0 && lastIndexDrawn != 0)
					{
						curPos = countItems() - 1;
						itemOffset = (countItems() - 1) - lastIndexDrawn;
						draw();
					}
					else
						curPos--;
				}
				while (!getIndex(curPos)->isEnabled());

				drawGun(x, y, basey);
				//
				// WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
				//
				TicDelay (20);
				break;

				////////////////////////////////////////////////
				//
				// MOVE DOWN
				//
			case dir_South:
				if(countItems() <= 1)
					break;

				eraseGun(x, y);
				//
				// ANIMATE HALF-STEP
				//
				if (curPos != lastIndexDrawn && curPos != countItems() - 1 && getIndex(curPos + 1)->isEnabled())
				{
					y += 6;
					drawGunHalfStep(x, y);
				}
				if (curPos == lastIndexDrawn && curPos != countItems() - 1)
				{
					itemOffset++;
					draw();
					getIndex(curPos)->setSelected(false); // Undo a change made by draw.
					PrintY = getY() + getHeight(curPos);
					getIndex(curPos)->draw();
				}

				do
				{
					if (curPos == countItems() - 1)
					{
						curPos = 0;
						itemOffset = 0;
						draw();
					}
					else
						curPos++;
				}
				while (!getIndex(curPos)->isEnabled());

				drawGun(x, y, basey);

				//
				// WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
				//
				TicDelay (20);
				break;
			case dir_West:
				getIndex(curPos)->left();
				PrintX = getX() + getIndent();
				PrintY = getY() + getHeight(curPos);
				getIndex(curPos)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
			case dir_East:
				getIndex(curPos)->right();
				PrintX = getX() + getIndent();
				PrintY = getY() + getHeight(curPos);
				getIndex(curPos)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
		}

		if (ci.button0 || Keyboard[sc_Space] || Keyboard[sc_Enter])
			exit = 1;

		if (ci.button1 && !Keyboard[sc_Alt] || Keyboard[sc_Escape])
			exit = 2;

	}
	while (!exit);


	IN_ClearKeysDown ();

	//
	// ERASE EVERYTHING
	//
	if (lastitem != curPos)
	{
		VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
		redrawitem = 1;
	}
	else
		redrawitem = 0;

	VW_UpdateScreen ();

	lastitem = curPos;
	switch (exit)
	{
		case 1:
			SD_PlaySound (SHOOTSND);
			getIndex(curPos)->activate();
			VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
			PrintX = getX() + getIndent();
			PrintY = getY() + getHeight(curPos);
			getIndex(curPos)->draw();
			VW_UpdateScreen();
			return curPos;

		case 2:
			SD_PlaySound(ESCPRESSEDSND);
			return -1;
	}

	return 0;                   // JUST TO SHUT UP THE ERROR MESSAGES!
}

void Menu::setHeadText(const char text[36], bool drawInStripes)
{
	strcpy(headText, text);
	headTextInStripes = drawInStripes;
}

void Menu::show()
{
	if(entryListener != NULL)
		entryListener(0);

	if(countItems() == 0) // Do nothing.
		return;
	if(curPos >= countItems())
		curPos = countItems()-1;

	draw();
	MenuFadeIn();
	WaitKeyUp();

	int item = 0;
	while((item = handle()) != -1);

	MenuFadeOut ();
}
