#include "wl_def.h"
#include "wl_menu.h"

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

bool Menu::close = false;

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

	US_Print("\n");
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

FunctionMenuItem::FunctionMenuItem(const char string[36], int (*function)(int), bool fadeEnabled) : MenuItem(string), function(function)
{
	setEnableFade(fadeEnabled);
}

void FunctionMenuItem::activate()
{
	if(fadeEnabled)
		MenuFadeOut();
	if(function != 0)
		function(menu->getCurrentPosition());
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

TextInputMenuItem::TextInputMenuItem(std::string text, unsigned int max, MENU_LISTENER_PROTOTYPE(preeditListener), MENU_LISTENER_PROTOTYPE(posteditListener)) : MenuItem("", posteditListener), max(max), preeditListener(preeditListener)
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
		US_LineInput(menu->getX() + menu->getIndent() + 2, PrintY, buffer, getValue(), true, max, menu->getWidth() - menu->getIndent() - menu->getX());
		setValue(buffer);
		delete[] buffer;
		fontnumber = 1;
		MenuItem::activate();
	}
}

void TextInputMenuItem::draw()
{
	setTextColor();
	DrawOutline(menu->getX() + menu->getIndent(), PrintY, menu->getWidth() - menu->getIndent() - menu->getX(), 11, fontcolor, fontcolor);
	PrintX = menu->getX() + menu->getIndent() + 2;
	PrintY += 1;
	fontnumber = 0;
	US_Print(getValue());
	fontnumber = 1;
}

void Menu::drawGun(int x, int &y, int which, int basey)
{
	VWB_Bar (x - 1, y, 25, 16, BKGDCOLOR);
	y = basey + getHeight(which);
	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);

	PrintX = getX() + getIndent();
	PrintY = getY() + getHeight(which);
	getIndex(which)->setSelected(true);
	getIndex(which)->draw();

	VW_UpdateScreen();
	SD_PlaySound(MOVEGUN2SND);
}

void Menu::eraseGun(int x, int y, int which)
{
	VWB_Bar(x - 1, y, 25, 16, BKGDCOLOR);

	PrintX = getX() + getIndent();
	PrintY = getY() + getHeight(which);
	getIndex(which)->setSelected(false);
	getIndex(which)->draw();
	VW_UpdateScreen();
}

Menu::Menu(int x, int y, int w, int indent, MENU_LISTENER_PROTOTYPE(entryListener)) : x(x), y(y), w(w), entryListener(entryListener), indent(indent), headPicture(-1), curPos(0), height(0)
{
	for(unsigned int i = 0;i < 36;i++)
		headText[i] = '\0';
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
	height += item->getHeight();
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
	unsigned int num = 0;
	for(unsigned int i = 0;i < items.size();i++)
	{
		if(position == 0)
			break;

		if(items[i]->isVisible())
		{
			num += items[i]->getHeight();
			position--;
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
	int which = curPos;

	WindowX = PrintX = getX() + getIndent();
	WindowY = PrintY = getY();
	WindowW = 320;
	WindowH = 200;

	PrintY = getY();
	int y = getY();
	int selectedY = getY(); // See later

	for (unsigned int i = 0; i < countItems(); i++)
	{
		if(i == curPos)
			selectedY = y;
		else
		{
			PrintY = y;
			getIndex(i)->draw();
		}
		y += getIndex(i)->getHeight();
	}

	// In order to draw the skill menu correctly we need to draw the selected option now
	PrintY = selectedY;
	getIndex(curPos)->setSelected(true);
	getIndex(curPos)->draw();
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

	if(close)
		return -1;

	which = curPos;
	x = getX() & -8;
	basey = getY() - 2;
	y = basey + getHeight(which);

	if(getIndent() != 0)
		VWB_DrawPic (x, y, C_CURSOR1PIC);
	if (redrawitem)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(which);
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
				{
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
				PrintY = getY() + getHeight(which);
				getIndex(which)->draw();
				VW_UpdateScreen();
				TicDelay(20);
				break;
			case dir_East:
				getIndex(which)->right();
				PrintX = getX() + getIndent();
				PrintY = getY() + getHeight(which);
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
		PrintY = getY() + getHeight(which);
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
			PrintY = getY() + getHeight(which);
			getIndex(which)->draw();
			VW_UpdateScreen();
			return which;

		case 2:
			SD_PlaySound(ESCPRESSEDSND);
			return -1;
	}

	return 0;                   // JUST TO SHUT UP THE ERROR MESSAGES!
}

void Menu::setHeadText(const char text[36])
{
	strcpy(headText, text);
}

void Menu::show()
{
	if(entryListener != NULL)
		entryListener(0);

	draw();
	MenuFadeIn();
	WaitKeyUp();

	int item = 0;
	while((item = handle()) != -1);

	MenuFadeOut ();
}
