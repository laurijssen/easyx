#include <easyinput.h>
#include <process.h>

//EINPUT is  a namespace. This thread gets started by EASYINPUT::SetKeyListener
void EINPUT::KeyListener(void *KeyParameter)
{	
	KeyEventParameter *param = (KeyEventParameter *)KeyParameter;
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	
	DIDEVICEOBJECTDATA od[EASYINPUT::BUFFERSIZE];
	DWORD dwItems;

	if( hEvent )
	{
		param->Keyboard->Unacquire();
		param->Keyboard->SetEventNotification(hEvent);		
		param->Keyboard->Acquire();
		while( true )
		{
			WaitForSingleObject(hEvent, INFINITE);
			dwItems = EASYINPUT::BUFFERSIZE;
			param->Keyboard->Poll();
			param->Keyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), od, &dwItems, 0);
			param->KeyInterface->KeyPressed(od, dwItems); // call this function every time a key is pressed
		}
	}

	CloseHandle(hEvent);	
}

void EINPUT::MouseListener(void *MouseParameter)
{
	MouseEventParameter *param = (MouseEventParameter *)MouseParameter;
	DIDEVICEOBJECTDATA od[EASYINPUT::MOUSEBUFFER];
	DWORD dwItems;

	while( true )
	{
		WaitForSingleObject(param->hEvent, INFINITE);
		dwItems = EASYINPUT::MOUSEBUFFER;
		param->Mouse->Poll();
		param->Mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), od, &dwItems, 0);
		param->MouseInterface->MouseEvent(od, dwItems);
	}

	CloseHandle(param->hEvent);
}

EASYINPUT::EASYINPUT(HWND hwnd)
{	
	WindowHandle = hwnd;
	Mouse = NULL;

	if( FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&InputObject, NULL)) )
	{
		ErrorMessage("error initializing directinput");		
		return;
	}

	if( FAILED(InputObject->CreateDevice(GUID_SysKeyboard, &Keyboard, NULL)) )
	{
		ErrorMessage("couldn't create temporary object");
		return;
	}

	if( FAILED(Keyboard->SetCooperativeLevel(WindowHandle, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) )
	{
		ErrorMessage("couldn't set cooperativelevel for this device");
		return;
	}

	if( FAILED(Keyboard->SetDataFormat(&c_dfDIKeyboard)) )
	{
		ErrorMessage("couldn't set dataformat");
		return;
	}

	DIPROPDWORD dipdw; // set the bufferproperties for the keyboard
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData     = BUFFERSIZE;
	if( FAILED(Keyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)) )
		OutputDebugString("couldn't set buffer property");
	
	CreateMouseSupport();
	Acquire();
}

EASYINPUT::~EASYINPUT()
{
	if( Keyboard )
		Keyboard->Release();
	if( Mouse )
		Mouse->Release();
}

void EASYINPUT::CreateMouseSupport()
{	
	if( FAILED(InputObject->CreateDevice(GUID_SysMouse, &Mouse, NULL)) )
	{
		ErrorMessage("couldn't create temporary object");
		return;
	}

	if( FAILED(Mouse->SetCooperativeLevel(WindowHandle, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) )
	{
		ErrorMessage("couldn't set cooperativelevel");
		return;
	}

	if( FAILED(Mouse->SetDataFormat(&c_dfDIMouse)) )
	{
		ErrorMessage("SetDataFormat failed");
		return;
	}

	MouseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	Mouse->SetEventNotification(MouseEvent);
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = MOUSEBUFFER;

	Mouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);	
}

void EASYINPUT::Acquire()
{
	Keyboard->Acquire();

	if( Mouse )
		Mouse->Acquire();
}

void EASYINPUT::Unacquire()
{
	Keyboard->Unacquire();
	if( Mouse )
		Mouse->Unacquire();
}

long EASYINPUT::GetAxis(short which)
{
	DIMOUSESTATE2 state;
	Mouse->GetDeviceState(sizeof(state), &state);

	if( which == EI_X )
		return state.lX;
	else if( which == EI_Y )
		return state.lY;
	else
		return state.lZ;
}

bool EASYINPUT::MouseDown(int which)
{
	DIMOUSESTATE2 state;

	Mouse->GetDeviceState(sizeof(state), &state);

	return state.rgbButtons[which] & 0x80 ? true : false;
}

bool EASYINPUT::KeyDown(int which)
{
	Keyboard->GetDeviceState(sizeof(keys), keys);

	 // the msb of this char determines whether the key is down
	return keys[which] & 0x80 ? true : false;
}

bool EASYINPUT::KeyDownBuffered(int which)
{
	for( DWORD d = 0; d < dwItems; d++ )
	{
		if( data[d].dwOfs == which )
			return data[d].dwData & 0x80 ? true : false;
	}

	return false;
}

bool EASYINPUT::KeyUpBuffered(int which)
{
	for( DWORD d = 0; d < dwItems; d++ )
	{
		if( data[d].dwOfs == which )
			return !(data[d].dwData & 0x80) ? true : false;
	}

	return false;
}

void EASYINPUT::UpdateBuffer()
{
	Keyboard->Poll();
	dwItems = BUFFERSIZE; // items in buffer
	while( FAILED(Keyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data, &dwItems, 0)) ) ;
}

LPDIRECTINPUTDEVICE8 EASYINPUT::GetKeyboard()
{
	return Keyboard;
}

LPDIRECTINPUTDEVICE8 EASYINPUT::GetMouse()
{
	return Mouse;
}

void EASYINPUT::SetKeyListener(IKeyInterface *Handler)
{
	using namespace EINPUT;
	KeyEventParameter *param = new KeyEventParameter(); // has to be on heap
	param->Keyboard = this->Keyboard;
	param->KeyInterface = Handler;

	_beginthread(KeyListener, 0, param);
}

void EASYINPUT::SetMouseListener(IMouseInterface *Handler)
{
	using namespace EINPUT;
	MouseEventParameter *param = new MouseEventParameter();
	param->Mouse = this->Mouse;
	param->MouseInterface = Handler;
	param->hEvent = MouseEvent;

	_beginthread(MouseListener, 0, param);
}

void EASYINPUT::ErrorMessage(char *s)
{
	MessageBox(WindowHandle, s, "Error", MB_OK);
}
