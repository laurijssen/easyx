#include "window.h"

// device context
DC::DC(EASYDRAW *DDraw)
{
	this->DDraw = DDraw;
	DDraw->BackBuffer.surface->GetDC(&hdc);	
}

DC::~DC()
{
	DDraw->BackBuffer.surface->ReleaseDC(hdc);
}

void DC::Ellipse(int x1, int y1, int x2, int y2)
{
	HBRUSH hBrush = CreateSolidBrush(DDraw->color);
	HGDIOBJ old = SelectObject(hdc, hBrush);
	::Ellipse(hdc, x1, y1, x2, y2);
	
	DeleteObject(SelectObject(hdc, old));
}

void DC::Arc(int left, int top, int right, int bottom, int startx, int starty, int endx, int endy)
{
	HPEN hPen = CreatePen(PS_SOLID, 0, DDraw->color);
	HGDIOBJ old = SelectObject(hdc, hPen);
	::Arc(hdc, left, top, right, bottom, startx, starty, endx, endy);	

	DeleteObject(SelectObject(hdc, old));
}

void DC::FillRect(int left, int top, int right, int bottom)
{
	HBRUSH hBrush = CreateSolidBrush(DDraw->color);
	HGDIOBJ old = SelectObject(hdc, hBrush);
	RECT rect = {left, top, right, bottom};
	::FillRect(hdc, &rect, hBrush);
	
	DeleteObject(SelectObject(hdc, old));
}

void DC::Rectangle(int left, int top, int right, int bottom)
{
	HBRUSH hBrush = CreateSolidBrush(DDraw->color);
	HGDIOBJ old = SelectObject(hdc, hBrush);
	::Rectangle(hdc, left, top, right, bottom);

	DeleteObject(SelectObject(hdc, old));
}

void DC::TextOut(int x, int y, char *text)
{
	::TextOut(hdc, x, y, text, strlen(text));
}

void DC::GradientFill(TRIVERTEX *vertex, DWORD numvertex, void *mesh, DWORD nummesh, DWORD mode)
{
	//::GradientFill(hdc, vertex, numvertex, mesh, nummesh, mode);
}

void DC::SetTextColor(COLORREF color)
{
	::SetTextColor(hdc, color);
}

void DC::SetBkMode(COLORREF color)
{
	::SetBkMode(hdc, color);
}

HDC DC::GetDC() { return hdc; }

MyWindow::MyWindow(int w, int h, void *MessageHandler, char *WindowName, int IconID)
{
	WNDCLASSEX wndclass;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	wndclass.cbSize      = sizeof wndclass;
	wndclass.style       = CS_HREDRAW | CS_VREDRAW;
  	wndclass.lpfnWndProc = (WNDPROC)MessageHandler;
	wndclass.cbClsExtra  =  0;
 	wndclass.cbWndExtra  =  0;
 	wndclass.hInstance   =  hInstance;
	wndclass.hCursor     =  LoadCursor(NULL, IDC_ARROW);
 	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
 	wndclass.lpszMenuName  = NULL;
 	wndclass.lpszClassName = WindowName;

	if( IconID == 0 )
	{
 		wndclass.hIcon =  LoadIcon (hInstance, IDI_APPLICATION);
		wndclass.hIconSm =  LoadIcon (hInstance, IDI_APPLICATION);
	}
	else
	{
		wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IconID));
		wndclass.hIconSm =  LoadIcon (hInstance, IDI_APPLICATION);
	}

 	RegisterClassEx(&wndclass);

	WindowHandle = CreateWindow(WindowName, WindowName, WS_POPUP, 0, 0, w, h,
								NULL, NULL, hInstance, NULL);

	::ShowWindow(WindowHandle, SW_SHOWNORMAL);
	UpdateWindow(WindowHandle);
}

MyWindow::~MyWindow()
{
	DestroyWindow(WindowHandle);
}

void MyWindow::LockWindowUpdate()
{
	::LockWindowUpdate(WindowHandle);
}

void MyWindow::UnlockWindowUpdate()
{
	::LockWindowUpdate(NULL);
}

void MyWindow::ShowWindow(int how)
{
	::ShowWindow(WindowHandle, how);
}