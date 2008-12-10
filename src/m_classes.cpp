#include "wl_def.h"

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

MenuItem::MenuItem(const char string[36]) : enabled(true), highlight(false), selected(false), visible(true)
{
	setText(string);
}

void MenuItem::draw()
{
	setTextColor();

	if (getActive())
		US_Print(getString());
	else
	{
		SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
		US_Print(getString());
		SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
	}

	US_Print ("\n");
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

BooleanMenuItem::BooleanMenuItem(const char string[36], boolean &value) : MenuItem(string), value(value)
{
}

void BooleanMenuItem::activate()
{
	value ^= 1;
}

void BooleanMenuItem::draw()
{
	if (value)
		VWB_DrawPic (PrintX - 24, PrintY + 3, C_SELECTEDPIC);
	else
		VWB_DrawPic (PrintX - 24, PrintY + 3, C_NOTSELECTEDPIC);
	MenuItem::draw();
}

FunctionMenuItem::FunctionMenuItem(const char string[36], int (*function)(int), bool fadeEnabled) : MenuItem(string), function(function)
{
	setEnableFade(fadeEnabled);
}

void FunctionMenuItem::activate()
{
	if(fadeEnabled)
		MenuFadeOut();
	if(function != 0)
		function(0);
}

MenuSwitcherMenuItem::MenuSwitcherMenuItem(const char string[36], Menu &menu) : MenuItem(string), menu(menu)
{
}

void MenuSwitcherMenuItem::activate()
{
	MenuFadeOut();
	menu.show();
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
	SD_PlaySound (MOVEGUN1SND);
}

void Menu::drawGun(int x, int &y, int which, int basey)
{
	VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
	y = basey + which * 13;
	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);

	PrintX = getX() + getIndent();
	PrintY = getY() + which * 13;
	getIndex(which)->setSelected(true);
	getIndex(which)->draw();

	VW_UpdateScreen();
	SD_PlaySound(MOVEGUN2SND);
}

void Menu::eraseGun(int x, int y, int which)
{
	VWB_Bar(x - 1, y, 25, 16, BKGDCOLOR);

	PrintX = getX() + getIndent();
	PrintY = getY() + which * 13;
	getIndex(which)->setSelected(false);
	getIndex(which)->draw();
	VW_UpdateScreen();
}

Menu::Menu(int x, int y, int w, int indent, void (*handler)(unsigned int), const char headText[36]) : x(x), y(y), w(w), indent(indent), headPicture(-1), curPos(0), handler(handler)
{
	strcpy(this->headText, headText);
}
Menu::~Menu()
{
	for(unsigned int i = 0;i < items.size();i++)
		delete items[i];
	items.clear();
}

void Menu::addItem(MenuItem *item)
{
	item->setMenu(this);
	items.push_back(item);
	if(!item->isEnabled() && items.size()-1 == curPos)
		curPos++;
}

const unsigned int Menu::countItems() const
{
	unsigned int num = 0;
	for(unsigned int i = 0;i < items.size();i++)
	{
		if(items[i]->isVisible())
			num++;
	}
	return num;
}

MenuItem *Menu::getIndex(int index)
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

void Menu::drawMenu()
{
	int which = curPos;

	WindowX = PrintX = getX() + getIndent();
	WindowY = PrintY = getY();
	WindowW = 320;
	WindowH = 200;

	for (unsigned int i = 0; i < countItems(); i++)
	{
		getIndex(i)->setSelected(i == curPos);
		PrintY = getY() + i * 13;
		getIndex(i)->draw();
	}
}

void Menu::draw()
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
    PrintY = getY() - 16;
	US_CPrint(headText);

	DrawWindow(getX() - 8, getY() - 3, getWidth(), getHeight(), BKGDCOLOR);
	drawMenu();
	VW_UpdateScreen ();
}

int Menu::handle()
{
	char key;
	static int redrawitem = 1, lastitem = -1;
	int i, x, y, basey, exit, which, shape;
	int32_t lastBlinkTime, timer;
	ControlInfo ci;


	which = curPos;
	x = getX() & -8;
	basey = getY() - 2;
	y = basey + which * 13;

	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);
	if (redrawitem)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + which * 13;;
		getIndex(which)->draw();
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

			for (i = which + 1; i < countItems(); i++)
				if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
				{
					eraseGun(x, y, which);
					which = i;
					drawGun(x, y, which, basey);
					ok = 1;
					IN_ClearKeysDown ();
					break;
				}

			//
			// DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN.
			//
			if (!ok)
			{
				for (i = 0; i < which; i++)
					if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
					{
						eraseGun(x, y, which);
						which = i;
						drawGun(x, y, which, basey);
						IN_ClearKeysDown ();
						break;
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

				eraseGun(x, y, which);

				//
				// ANIMATE HALF-STEP
				//
				if (which && getIndex(which - 1)->isEnabled())
				{
					y -= 6;
					DrawHalfStep (x, y);
				}

				//
				// MOVE TO NEXT AVAILABLE SPOT
				//
				do
				{
					if (!which)
						which = countItems() - 1;
					else
						which--;
				}
				while (!getIndex(which)->isEnabled());

				drawGun(x, y, which, basey);
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

				eraseGun(x, y, which);
				//
				// ANIMATE HALF-STEP
				//
				if (which != countItems() - 1 && getIndex(which + 1)->isEnabled())
				{
					y += 6;
					DrawHalfStep (x, y);
				}

				do
				{
					if (which == countItems() - 1)
						which = 0;
					else
						which++;
				}
				while (!getIndex(which)->isEnabled());

				drawGun(x, y, which, basey);

				//
				// WAIT FOR BUTTON-UP OR DELAY NEXT MOVE
				//
				TicDelay (20);
				break;
			case dir_West:
				getIndex(which)->left();
				PrintX = getX() + getIndent();
				PrintY = getY() + which * 13;
				getIndex(which)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
			case dir_East:
				getIndex(which)->right();
				PrintX = getX() + getIndent();
				PrintY = getY() + which * 13;
				getIndex(which)->draw();
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
	if (lastitem != which)
	{
		VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
		PrintX = getX() + getIndent();
		PrintY = getY() + which * 13;
		getIndex(which)->draw();
		redrawitem = 1;
	}
	else
		redrawitem = 0;

	VW_UpdateScreen ();

	curPos = which;

	lastitem = which;
	switch (exit)
	{
		case 1:
			SD_PlaySound (SHOOTSND);
			getIndex(which)->activate();
			VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
			PrintX = getX() + getIndent();
			PrintY = getY() + which * 13;
			getIndex(which)->draw();
			VW_UpdateScreen();
			return which;

		case 2:
			SD_PlaySound(ESCPRESSEDSND);
			return -1;
	}

	return 0;                   // JUST TO SHUT UP THE ERROR MESSAGES!
}

void Menu::show()
{
	draw();
	MenuFadeIn();
	WaitKeyUp();

	int item = 0;
	while((item = handle()) != -1)
	{
		if(handler != NULL)
			handler(item);
	}

	MenuFadeOut ();
}
