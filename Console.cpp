#include <console.h>

Console::Console(char *image, EASYDRAW *DDraw)
{
	this->DDraw = DDraw;
	this->image = DDraw->CreateImage(DDraw->GetScreenWidth(), DDraw->GetScreenHeight() / 2, 0, image);

	active = false;

	for( int i = 0; i < 10; i++ )
	{
		for( int j = 0; j < 50; j++ )
			line[i][j] = '\0';
	}

	prev = 'x';
	position = 0;
	time = timeGetTime();
}

Console::~Console()
{
	DDraw->ReleaseSurface(image);
}

void Console::Draw()
{
	if( active )
	{
		DDraw->DrawImageFast(0, 0, image, DDBLTFAST_WAIT);
		
		char temp[55];
		sprintf(temp, "-- %s", buffer);

		if( SUCCEEDED(DDraw->GetImage(image)->GetSurface()->GetDC(&hdc)) )
		{
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 0, 0));
			for( int i = 0; i < 10; i++ )
				TextOut(hdc, 10, i * 20 + 10, line[i], strlen(line[i]));
		}

		DDraw->GetImage(image)->GetSurface()->ReleaseDC(hdc);
	}
}

void Console::Input(EASYINPUT *DInput)
{
	if( !active )
		return;

	DWORD currentTime = timeGetTime();

	char *keys = DInput->GetKeys();
	char in = 0;

	if( keys[DIK_1] & 0x80 )
		in = '1';
	if( keys[DIK_2] & 0x80 )
		in = '2';
	if( keys[DIK_3] & 0x80 )
		in = '3';
	if( keys[DIK_4] & 0x80 )               
		in = '4';
	if( keys[DIK_5] & 0x80 )               
		in = '5';
	if( keys[DIK_6] & 0x80 )               
		in = '6';
	if( keys[DIK_7] & 0x80 )               
		in = '7';
	if( keys[DIK_8] & 0x80 )
		in = '8';
	if( keys[DIK_9] & 0x80 )               
		in = '9';
	if( keys[DIK_0] & 0x80 )               
		in = '0';
	if( keys[DIK_MINUS] & 0x80 )               /* - on main keyboard */
		in = '-';
	if( keys[DIK_EQUALS] & 0x80 )          
		in = '=';
	if( keys[DIK_Q] & 0x80 )
		in = 'q';
	if( keys[DIK_W] & 0x80 )               
		in = 'w';
	if( keys[DIK_E] & 0x80 )               
		in = 'e';
	if( keys[DIK_R] & 0x80 )               
		in = 'r';
	if( keys[DIK_T] & 0x80 )               
		in = 't';
	if( keys[DIK_Y] & 0x80 )               
		in = 'y';
	if( keys[DIK_U] & 0x80 )               
		in = 'u';
	if( keys[DIK_I] & 0x80 )               
		in = 'i';
	if( keys[DIK_O] & 0x80 )               
		in = 'o';
	if( keys[DIK_P] & 0x80 )               
		in = 'p';
	if( keys[DIK_LBRACKET] & 0x80 )        
		in = '(';
	if( keys[DIK_RBRACKET] & 0x80 )        
		in = ')';
	if( keys[DIK_A] & 0x80 )               
		in = 'a';
	if( keys[DIK_S] & 0x80 )               
		in = 's';
	if( keys[DIK_D] & 0x80 )               
		in = 'd';
	if( keys[DIK_F] & 0x80 )               
		in = 'f';
	if( keys[DIK_G] & 0x80 )               
		in = 'g';
	if( keys[DIK_H] & 0x80 )               
		in = 'h';
	if( keys[DIK_J] & 0x80 )               
		in = 'j';
	if( keys[DIK_K] & 0x80 )               
		in = 'k';
	if( keys[DIK_L] & 0x80 )
		in = 'l';
	if( keys[DIK_SEMICOLON] & 0x80 )
		in = ';';
	if( keys[DIK_APOSTROPHE] & 0x80 )      
		in = '\'';
	if( keys[DIK_GRAVE] & 0x80 )               /* accent grave */
		in = '^';
	if( keys[DIK_BACKSLASH] & 0x80 )       
		in = '\\';
	if( keys[DIK_Z] & 0x80 )               
		in = 'z';
	if( keys[DIK_X] & 0x80 )               
		in = 'x';
	if( keys[DIK_C] & 0x80 )               
		in = 'c';
	if( keys[DIK_V] & 0x80 )               
		in = 'v';
	if( keys[DIK_B] & 0x80 )               
		in = 'b';
	if( keys[DIK_N] & 0x80 )               
		in = 'n';
	if( keys[DIK_M] & 0x80 )               
		in = 'm';
	if( keys[DIK_COMMA] & 0x80 )           
		in = ',';
	if( keys[DIK_PERIOD] & 0x80 )              /* . on main keyboard */
		in = '.';
	if( keys[DIK_SLASH] & 0x80 )               /* / on main keyboard */
		in = '/';
	if( keys[DIK_MULTIPLY] & 0x80 )            /* * on numeric keypad */
		in = '*';
	if( keys[DIK_SPACE] & 0x80 )           
		in = ' ';
	if( keys[DIK_NUMPAD7] & 0x80 )         
		in = '7';
	if( keys[DIK_NUMPAD8] & 0x80 )         
		in = '8';
	if( keys[DIK_NUMPAD9] & 0x80 )         
		in = '9';
	if( keys[DIK_SUBTRACT] & 0x80 )            /* - on numeric keypad */
		in = '-';
	if( keys[DIK_NUMPAD4] & 0x80 )         
		in = '4';
	if( keys[DIK_NUMPAD5] & 0x80 )         
		in = '5';
	if( keys[DIK_NUMPAD6] & 0x80 )         
		in = '6';
	if( keys[DIK_ADD] & 0x80 )                 /* + on numeric keypad */
		in = '+';
	if( keys[DIK_NUMPAD1] & 0x80 )         
		in = '1';
	if( keys[DIK_NUMPAD2] & 0x80 )         
		in = '2';
	if( keys[DIK_NUMPAD3] & 0x80 )         
		in = '3';
	if( keys[DIK_NUMPAD0] & 0x80 )         
		in = '0';
	if( keys[DIK_DECIMAL] & 0x80 )             /* . on numeric keypad */
		in = '.';
	if( keys[DIK_OEM_102] & 0x80 ) /* < > | on UK/Germany keyboards */
		in = '|';

	if( in != 0 )
	{
		if( position < 50 )
			buffer[position++] = in;

		prev = in;
	}	
	time = timeGetTime();
}

bool Console::IsActive()
{
	return active;
}