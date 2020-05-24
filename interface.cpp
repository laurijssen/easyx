#include <interface.h>

EASYDRAW *Component::DDraw = NULL;

Component::Component()
{
	SetVisible(true);
	needrepaint = true;
}

Component::Component(short x, short y, short w, short h)
{
	SetVisible(true);
	SetBounds(x, y, w, h);
	needrepaint = true;
}

void Component::Move(U16 x, U16 y)
{
	this->x = x, this->y = y;
}

void Component::SetBounds(short x, short y, short w, short h)
{
	this->x = x, this->y = y, width = w, height = h;
}

void Component::SetVisible(bool b)
{
	visible = b;
}

void Component::SetOutlineColor(U16 col)
{
	outline = col;
}

void Component::SetBackground(U16 col)
{
	background = col;
}

// COMPONENT ************************************************ COMPONENT

Control::Control()
{
}

Control::Control(short x, short y, short w, short h) : Component(x, y, w, h) {}

void Control::SetParent(Window *window)
{
	parent = window;
}

Window *Control::GetParent()
{
	return parent;
}

void Control::SetTextColor(U16 col)
{
	textcolor = col;
}

//CONTROL *************************************************** CONTROL

Window::Window() {}

Window::Window(short x, short y, short w, short h, std::string &title) : Component(x, y, w, h) 
{
	this->title = title;
	clientX = x;
	clientY = y + 16;

	// set its colors, in future the colors may come from a GUI editor
	outline = 0;
	background = DDraw->Convert16Bit(10, 10, 10);
	nonclient = DDraw->Convert16Bit(0, 0, 25);
	titlecolor = 65535;
	closeboxcolor = DDraw->Convert16Bit(25, 0, 0);
}

Window::~Window()
{
	ControlIterator it = list.begin();

	while (it != list.end())
	{
		delete *it;
		++it;
	}
}

void Window::Draw()
{
	if (visible && needrepaint)
	{
		DrawDecorations();
		ControlIterator it = list.begin();
		ControlIterator end = list.end();

		while (it != end)
		{
			(*it)->Draw();
			++it;
		}
	}
}

Control *Window::GetComponent(POINT &p) // p contains the mouse coords
{
	ControlIterator it = list.begin();
	ControlIterator end = list.end();

	Control *c;

	while (it != end)
	{
		c = *it;
		if (clientX + c->x < p.x && clientX + c->x + c->width > p.x && 
			clientY + c->y < p.y && clientY + c->y + c->height > p.y)
		  return c;

		++it;
	}

	return 0;
}

void Window::Move(U16 x, U16 y)
{
	this->x = x, this->y = y;
	clientX = x, clientY = y + 16;
}

void Window::AddControl(Control *control)
{
	control->SetParent(this);
	list.push_back(control);
}

void Window::DrawDecorations()
{
	DDraw->SetColor(background);
	DDraw->FillRect(x+1, y+1, x+width-1, y+height-1); // draw the window

	DDraw->SetColor(nonclient);
	DDraw->FillRect(x, y, x+width, y+16); // draw non-client area on top

	DDraw->SetColor(titlecolor);
	DDraw->DrawString(x+14, y+3, title.c_str()); // draw title

	DDraw->SetColor(closeboxcolor);
	DDraw->FillRect(x+3, y+4, x+11, y+11); // draw close box at the top

	DDraw->SetColor(outline);
	DDraw->FrameRect(x, y, x+width, y+height); // draw outline
}

void Window::SetNonClient(U16 col)
{
	nonclient = col;
}

void Window::SetTitleColor(U16 col)
{
	titlecolor = col;	
}

void Window::SetCloseBoxColor(U16 col)
{
	closeboxcolor = col;
}


//WINDOW ************************************************* WINDOW

TextField::TextField()
{
}

TextField::TextField(short x, short y, short w, short h) : Control(x, y, w, h) 
{
	outline = 0;
	caretpos = 0;
	caretcount = 0;
	background = 65535;
	drawcaret = false;
}

void TextField::Draw()
{	
	if (visible && needrepaint)
	{		
		int xpos = parent->clientX + x;
		int ypos = parent->clientY + y; 
		int xpos2 = xpos + width;
		int ypos2 = ypos + height;
		DDraw->SetColor(background);
		DDraw->FillRect(xpos+1, ypos+1, xpos2-1, ypos2-1);
		DDraw->SetColor(outline);
		DDraw->FrameRect(xpos, ypos, xpos2, ypos2);
		DDraw->SetColor(textcolor);

		caretcount++; // waiting for overflow to get back at 0
		if (caretcount == 80)
		{
			drawcaret = !drawcaret;
			caretcount = 0;
		}
		if (drawcaret)
			DDraw->VerticalLine(ypos, ypos2, caretpos * 8 + xpos + 3);
	}
}

// TEXTFIELD ************************************************** TEXTFIELD

Button::Button()
{
}

Button::Button(short x, short y, short w, short h, std::string &text) : Control(x, y, w, h) 
{
	this->text = text;
	textcolor = 65535;
	background = DDraw->Convert16Bit(31, 0, 0);
	outline = 0;
}

void Button::Draw()
{	
	if (visible && needrepaint)
	{		
		int xpos = parent->clientX + x;
		int ypos = parent->clientY + y;
		int xpos2 = xpos + width;
		int ypos2 = ypos + height;
		DDraw->SetColor(background);
		DDraw->FillRect(xpos, ypos, xpos2, ypos2);
		DDraw->SetColor(textcolor);
		DDraw->DrawString(xpos+((xpos2-xpos)/4), ypos+((ypos2-ypos)/4)+3, text.c_str());
		DDraw->SetColor(outline);
		DDraw->FrameRect(xpos, ypos, xpos2, ypos2);
	}
}

Picture::Picture() {}

Picture::Picture(short x, short y, short w, short h, std::string *file) : Control(x, y, w, h) 
{
	if (file)
		image = DDraw->CreateImage(w, h, 0, (char *)file->c_str());
	else
		image = DDraw->CreateImage(w, h, 0);
}

Picture::~Picture()
{
	DDraw->ReleaseSurface(image);
}

void Picture::Draw()
{	
	if (visible && needrepaint)
		DDraw->DrawImage(parent->clientX + x, parent->clientY + y, image);
}

bool Picture::LoadImage(char *file)
{
	char *extension = file + strlen(file) - 4; // extension points to the files extension
	if (strcmp(extension, ".bmp") == 0)
		return DDraw->GetImage(image)->LoadBitmap(file);			
	else if (strcmp(extension, ".tga") == 0)
		return DDraw->GetImage(image)->LoadTarga(file, DDraw);
	else
		return DDraw->GetImage(image)->LoadEasyFormat(file, DDraw);
}

Label::Label()
{
}

Label::Label(short x, short y, short w, short h) : Control(x, y, w, h) {}

Label::Label(short x, short y, short w, short h, std::string &text) : Control(x, y, w, h)
{
	this->text = text;
	textcolor = 0;
}

void Label::Draw()
{
	if (visible && needrepaint)
	{
		int xpos = parent->clientX + x;
		int ypos = parent->clientY + y;
		DDraw->DrawString(xpos, ypos, text.c_str());
	}
}

// LABEL *********************************************************************** LABEL

EASYINPUT *WindowManager::DInput = NULL; // static, set by GameManager

WindowManager::WindowManager()
{
	for (int i = 0; i < MAXWINDOWS; i++)
	{
		WindowList.push_back(i);
		windows[i] = NULL;
	}	
	DInput->SetMouseListener(this);
	DInput->SetKeyListener(this);
	selected = -1;
}

WindowManager::~WindowManager()
{
	for (int i = 0; i < MAXWINDOWS; i++)
	{
		delete windows[i];
		windows[i] = 0;
	}
}

void WindowManager::Update()
{
	for (int i = 0; i < MAXWINDOWS; i++)
	{
		if (windows[i])
			windows[i]->Draw();
	}
}

WindowID WindowManager::InsertWindow(Window *window)
{
	if (WindowList.empty())
		return FAIL;

	int index = WindowList.front();
	WindowList.pop_front();

	windows[index] = window;

	return index;
}

void WindowManager::RemoveWindow(int which)
{
	delete windows[which];
	windows[which] = NULL;
	WindowList.push_front(which);
}

void WindowManager::MouseEvent(DIDEVICEOBJECTDATA *od, DWORD dwItems)
{
	DWORD i;
	long mousestate = 0;
	POINT pos;
	GetCursorPos(&pos); // get cursor in screen coords

	for (i = 0; i < dwItems; i++)
	{
		if (od[i].dwOfs == DIMOFS_BUTTON0)
			mousestate |= IMouseInterface::LBUTTON;
		else if (od[i].dwOfs == DIMOFS_BUTTON1)
			mousestate |= IMouseInterface::RBUTTON;
		else if (od[i].dwOfs == DIMOFS_BUTTON2)
			mousestate |= IMouseInterface::MBUTTON;
	}

	if (mousestate)
	{
		for (i = 0; i < MAXWINDOWS; i++)
		{
			if (windows[i] && windows[i]->visible)
			{
				if (windows[i]->x < pos.x && windows[i]->x + windows[i]->width > pos.x &&
					windows[i]->y < pos.y && windows[i]->y + windows[i]->height > pos.y)
				{
					selected = i;					
					Control *c = windows[i]->GetComponent(pos);
					if (c)
						c->MouseClicked(mousestate);
					else
						windows[i]->MouseClicked(mousestate, pos);
				}
			}
		}
	}
}

void WindowManager::KeyPressed(DIDEVICEOBJECTDATA *KeyData, DWORD dwItems)
{
}