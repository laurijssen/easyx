/**************************************************************
easydraw lib 
author: Servé Laurijssen 2000 ©
**************************************************************/

#include "alphabet.h"
#include <Easydraw.h>

using namespace std;

/********************************************************
* the guid of the device that you can choose from a list
* or keep it 0 to get the primary device 
*********************************************************/
GUID *EASYDRAW::DeviceGUID = 0;

/**********************************************
The constructor takes care of creating the mainwindow
and calls the initddraw function .
If you don't have an icon for the application, set the last
parameter (IconID) to 0
**********************************************/

EASYDRAW::EASYDRAW(int w, int h, void *MessageHandler, char *WindowName, int IconID)
: m_window(new MyWindow(w, h, MessageHandler, WindowName, IconID))
{
	ScreenBuffer.surface = 0;
	BackBuffer.surface   = 0;
	alphabet             = letters; // letters is an array that contains all the pixels to draw
	OffSurfaceCount      = 0;	
	color								 = 65535; // default is white, because in 16-bit mode
	
	ScreenWidth = w, ScreenHeight = h;	
	
	if (SUCCEEDED(InitDDraw()))
	{
		for (int i = 0; i < MAXIMAGES; i++)
			ImageList.push(i); // list gets 0 to MAXIMAGES because everything is free to use

		ClearScreen(); // clear the two buffers
		Swap();
		ClearScreen();
	}
}

//***********************************************************************

// the Image array is automatically destroyed
EASYDRAW::~EASYDRAW()
{		
	clipper->Release();
	DrawObject->Release();
}

/*****************************************************
Create the directdraw objects and calls Createsurfaces()
*****************************************************/
int EASYDRAW::InitDDraw()
{
	if (FAILED(DirectDrawCreateEx(DeviceGUID, (void **)&DrawObject, IID_IDirectDraw7, 0)))
		return FAIL;

	if (FAILED(DrawObject->SetCooperativeLevel(m_window->GetWindowHandle(), 
							   DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT)))
		return FAIL;

	if (FAILED(DrawObject->SetDisplayMode(ScreenWidth, ScreenHeight, 16, 0, 0)))
		return FAIL;

	if (FAILED(CreateSurfaces()))
		return FAIL;

	if (FAILED(DrawObject->CreateClipper(0, &clipper, 0)))
		return FAIL;

	clipper->SetHWnd(0, GetWindowHandle());
	BackBuffer.surface->SetClipper(clipper);
	m_RenderTarget = &BackBuffer;
	
	return true;
}

/*********************************************************************
this function creates the primary surface with one backbuffer
*********************************************************************/
int EASYDRAW::CreateSurfaces()
{
	DDSCAPS2 ddsCaps = {0};

	// create the primary surface
	memset(&Descriptor, 0, sizeof Descriptor);
	Descriptor.dwSize = sizeof Descriptor;	
	memset(&Descriptor.ddpfPixelFormat, 0, sizeof Descriptor.ddpfPixelFormat);
	Descriptor.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	Descriptor.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	Descriptor.dwBackBufferCount = 1;

	if (FAILED(DrawObject->CreateSurface(&Descriptor, &ScreenBuffer.surface, 0)))
		ErrorMessage("couldn't create surface");

	ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
	if (FAILED(ScreenBuffer.surface->GetAttachedSurface(&ddsCaps, &BackBuffer.surface)))
		ErrorMessage("couldn't attach backbuffer");

	memset(&Descriptor, 0, sizeof Descriptor);
	Descriptor.dwSize = sizeof Descriptor;
 	Descriptor.dwFlags = DDSD_PIXELFORMAT; // give the pixelformat back
	if( FAILED(ScreenBuffer.surface->GetSurfaceDesc(&Descriptor)) )
		return FAIL;

	//************* get the color type, retrieved with GetSurfaceDesc()
	int g = Descriptor.ddpfPixelFormat.dwGBitMask>>5;// this assumes 5 for red

	if (g == 0x1F) // == 31 decimal and ...0011111 (5 bits for green)
		ColorType = _555_;
	else if (g == 0x3F) // == 63 decimal and ...0111111 (6 bits for green)
		ColorType = _565_;
	else
	{ 
		ErrorMessage("Wrong Colormode"); 
		return 0;
	}

	return 0;
}

/********************************************************************
Creates a new offscreen surface and stores it in the off surfaces array
********************************************************************/
ImageID EASYDRAW::CreateImage(int width, int height, int parm, char *file)
{
	ImageID index; // the index of the first empty surface
	
	if( OffSurfaceCount >= MAXIMAGES )
		return FAIL;

	DescribeOffScreenSurface(width, height, parm); //fill DDSURFASEDESC (Descriptor)

	if( FAILED(index = FirstEmptySurface()) )
	{
		ErrorMessage("Too many images");
		return FAIL;
	}

	if (FAILED(DrawObject->CreateSurface(&Descriptor, &OffSurface[index].surface, 0)))
		return FAIL;

	OffSurface[index].Height = height; // set the image height and width
	OffSurface[index].Width  = width;

	LockImage(&OffSurface[index]); // obtain address of image data
	OffSurface[index].surface->Unlock(0);

	OffSurfaceCount++;

	if( file != 0 ) // if the user wants to load imagedata from file rightaway
	{
		char *extension = file + strlen(file) - 4; // extension points to the files extension
		if( strcmp(extension, ".bmp") == 0 )
			OffSurface[index].LoadBitmap(file);			
		else if( strcmp(extension, ".tga") == 0 )
			OffSurface[index].LoadTarga(file, this);
		else if( strcmp(extension, ".jpg") == 0 )
			OffSurface[index].LoadJPG(file, this);
		else
			OffSurface[index].LoadEasyFormat(file, this);
	}
	else // make it black
		Memset16Bit(Screen.Screen, 0, height * width); //Screen.Screen is filled by LockImage()

	return index; // returns the index in the array where this surface is stored
}

/**************************************************************
* This function creates a surface that is partly transparent
* It sort of floats above the underlying surface (when it is drawn)
**************************************************************/
ImageID EASYDRAW::CreateTransparentImage(int width, int height, U16 color)
{
	ImageID index;

	if( OffSurfaceCount >= MAXIMAGES )
		return FAIL;

	DescribeOffScreenSurface(width, height, 0); //fill DDSURFASEDESC with standard options

	if( FAILED(index = FirstEmptySurface()) )
		return FAIL;

	if (FAILED(DrawObject->CreateSurface(&Descriptor, &OffSurface[index].surface, 0)))
		return FAIL;

	OffSurface[index].Height = height;
	OffSurface[index].Width  = width;

	LockImage(&OffSurface[index]); // get its address

	OffSurface[index].surface->Unlock(0);
	SetColorKey(Convert16Bit(31, 0, 31), &OffSurface[index]);

	int n = height * width;
	U16ptr vidmem = (U16ptr)Screen.Screen;

	Memset16Bit(Screen.Screen, Convert16Bit(31, 0, 31), n);

	bool b = true;

	while( n-- )
	{
		if( b ) // fill the surface with the specified color skipping one pixel
			*vidmem = color;

		vidmem++;
		b = !b;
	}

	OffSurfaceCount++;

	return index;
}

/***************************************************************************
* fills the DDSURFACEDESC struct to represent an offscreen surface
***************************************************************************/
void EASYDRAW::DescribeOffScreenSurface(int width, int height, int parm)
{
	memset(&Descriptor, 0, sizeof Descriptor);
	Descriptor.dwSize = sizeof Descriptor;
	Descriptor.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	Descriptor.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;	
	Descriptor.dwHeight = height;
	Descriptor.dwWidth = width;

	Descriptor.ddsCaps.dwCaps |= parm;
}

void EASYDRAW::LockImage(Image *image)
{
	memset(&Descriptor, 0, sizeof Descriptor);
	Descriptor.dwSize = sizeof Descriptor;

	while (image->surface->Lock(0, &Descriptor, DDLOCK_SURFACEMEMORYPTR, 0) != DD_OK)
		;
	
	Screen.Screen    = (U16ptr)Descriptor.lpSurface; // the address of the surface in memory
	Screen.Height    = ScreenHeight; // eh, bug? too lame to fix?
	Screen.Width     = ScreenWidth;
	Screen.RealWidth = Descriptor.lPitch >> 1; // lpitch is in bytes,so div by 2 because in 16 bit
}

void EASYDRAW::ErrorMessage(char *s)
{
	MessageBox(m_window->GetWindowHandle(), s, "Error", MB_OK);
}

/*******************************************************************************
Call this one before every drawing because it gets the address of the backbuffer
Stopdrawing() unlocks the surface again. not needed for Drawing images
********************************************************************************/
void EASYDRAW::StartDrawing()
{	
	LockImage(m_RenderTarget); //&BackBuffer); //get its address

	m_RenderTarget->surface->Unlock(0);	
}

void EASYDRAW::StopDrawing()
{
	ScreenBuffer.surface->Unlock(0);
	Screen.Screen = 0;
}

// flip the front and backbuffer, but not when they are still flipping
int EASYDRAW::Swap()
{
	int ret = BackBuffer.surface->GetFlipStatus(DDGFS_ISFLIPDONE);

	if( ret == DDERR_WASSTILLDRAWING )
		return 1;

	ret = ScreenBuffer.surface->Flip(0, DDFLIP_WAIT);	

	if( ret == DDERR_SURFACELOST )
		return RestoreImages();

	return 0;
}

// blts the backbuffer to the frontbuffer, is slower than Flipping but dialogs can be drawn now
int EASYDRAW::SwapWithBlt()
{
	int ret = ScreenBuffer.surface->Blt(0, BackBuffer.surface, 0, DDBLT_WAIT, 0);

	if( ret == DDERR_SURFACELOST )
		return RestoreImages();

	return 0;
}

/************************************************************************
* puts bitmap date inside the already created image. The image should be
* created with CreateBlankImage()
************************************************************************/
bool EASYDRAW::LoadBitmap(int which, char * ImageName)
{
	Image *image = GetImage(which);

	if( image == 0 )
		return false;
	else
		return image->LoadBitmap(ImageName);
} /* LoadBitmap */

bool EASYDRAW::LoadTarga(int which, char *ImageName)
{
	Image *img = GetImage(which);

	if( img == 0 )
		return false;
	else
		return img->LoadTarga(ImageName, this);
} /* LoadTarga */

bool EASYDRAW::LoadEasyFormat(int which, char *ImageName)
{
	Image *img = GetImage(which);

	if( img == 0 )
		return false;
	else
		return img->LoadEasyFormat(ImageName, this);
} /* LoadEasyFormat */

bool EASYDRAW::LoadJPG(int which, char *ImageName)
{
	Image *img = GetImage(which);

	if( img == 0 )
		return false;
	else
		return img->LoadJPG(ImageName, this);
} /* LoadJPG */

void EASYDRAW::DrawMirrorImage(int x, int y, int which, int parms)
{
	DDBLTFX fx = {0};
	fx.dwSize = sizeof fx;
	fx.dwDDFX = DDBLTFX_MIRRORUPDOWN;

	DrawImage(x, y, which, parms | DDBLT_DDFX, &fx);
}

/***************************************************************************
* If you are sure that you don't need clipping for this drawing, call this 
* function. It is faster, since it doesn't access the windows cliplist
****************************************************************************/
bool EASYDRAW::DrawImageFast(int x, int y, int which, int parm)
{	
	parm |= DDBLTFAST_WAIT;
	
	if( FAILED(m_RenderTarget->surface->BltFast(x, y, OffSurface[which].surface, 0, parm)) )
		return false;

	return true;
}

bool EASYDRAW::DrawImage(int x, int y, int which, int parm, LPDDBLTFX ddbltfx)
{
	RECT destrect, srcrect;	

	if( x >= ScreenWidth || y >= ScreenHeight )
		return false;

	int width = OffSurface[which].Width, height = OffSurface[which].Height;

	SetRect(&destrect, x, y, x + width, y + height);
	SetRect(&srcrect, 0, 0, width, height);

	// draw to the backbuffer
	if( FAILED(m_RenderTarget->surface->Blt(&destrect, OffSurface[which].surface, &srcrect, parm, ddbltfx)) )
		return false;

	return true;
}

bool EASYDRAW::DrawImage(RECT *destrect, RECT *srcrect, int which, int parm, LPDDBLTFX ddbltfx)
{
	if( OffSurface[which].surface == 0 ) // no bitmap to draw
		return false;

	if( FAILED(m_RenderTarget->surface->Blt(destrect, OffSurface[which].surface, srcrect, parm, ddbltfx)) )
		return false;

	return true;
}

bool EASYDRAW::DrawOnPrimary(int x, int y, int which, int parm, LPDDBLTFX ddbltfx)
{
	int width = OffSurface[which].Width, height = OffSurface[which].Height;

	if( OffSurface[which].surface == 0 ) // no bitmap to draw
		return false;

	// clipping
	if( x + width >= ScreenWidth )
		width = ScreenWidth - x;

	if( y + height >= ScreenHeight )
		height = ScreenHeight - y;

	RECT destrect = { x, y, x + width, y + height };
	RECT srcrect  = { 0, 0, width, height };

	/* everything was fine, so we continue drawing to the primary surface*/
	if( FAILED(ScreenBuffer.surface->Blt(&destrect, OffSurface[which].surface, &srcrect, parm, ddbltfx)) )
		return false;

	return true;
}

void EASYDRAW::DrawOverlay(int x, int y, Overlay *overlay)
{
	if( !overlay ) return;

	overlay->Draw(x, y, this);
}

void EASYDRAW::HideOverlay(Overlay *overlay)
{
	if( !overlay ) return;

	overlay->Hide(this);
}

void EASYDRAW::CopyBitmap(ImageID dest, ImageID src, int x, int y, int width, int height)
{
	if( OffSurface[dest].surface == 0 || OffSurface[src].surface == 0 )
	{
		ErrorMessage("Error in CopyBitmap() : Empty bitmap");
		return;
	}

	RECT rect = {x, y, x + width, y + height};
	OffSurface[dest].surface->Blt(0, src == BACK ? BackBuffer.surface : OffSurface[src].surface, 
								  &rect, 0, 0);
}

// The code that's commented out seems to be slower than the standard dx routine
void EASYDRAW::ClearScreen(LPRECT rect, U16 color)
{
	DDBLTFX fx = {0};
	fx.dwSize = sizeof fx;
	fx.dwFillColor = color;
	BackBuffer.GetSurface()->Blt(rect, 0, 0, DDBLT_COLORFILL | DDBLT_DDFX, &fx);
}

// overloaded, for ease of use, just calls other clearscreen
void EASYDRAW::ClearScreen(U32 left, U32 top, U32 right, U32 bottom, U16 color)
{
	RECT rect = {left, top, right, bottom};
	ClearScreen(&rect, color);
}

/*******************************************************************
asm routine to set the specified memory in dwords
*******************************************************************/
void EASYDRAW::Memset16Bit(U16ptr dst, U16 fill, int num)
{
    if (num <= 0) return;
    if (num&1) *dst++ = fill; // one too many for stosd
    if (num <2) return;

 _asm
 {
    mov    ax, fill   // store value
    mov   ecx, num    // how many copies
    shl   eax, 16     // shift it into high order
    mov   edi, dst    // to where
    add    ax, fill   // do two pixels at a time
    shr   ecx, 1      // divide by 2

    rep   stosd		  // pentium memset routine
 }
}

/*******************************************************************
my c-version of memset. much slower but more readable
*******************************************************************/
void EASYDRAW::Memset(U16ptr dest, U16 fill, int num)
{
	if( num <= 0 ) return;

	while( num-- )
		*dest++ = fill;
}

HWND EASYDRAW::GetWindowHandle()
{
	return m_window->GetWindowHandle();
} 

Image *EASYDRAW::GetScreenBuffer()
{
	return &ScreenBuffer;
}

Image *EASYDRAW::GetBackBuffer()
{
	return &BackBuffer;
}

Image *EASYDRAW::GetImage(ImageID which)
{
	return &OffSurface[which];
}

// each letter is 8 high and 12 wide and the color is white
void EASYDRAW::DrawString(int x, int y, const char *s)
{
	int len = strlen(s);

	// a little clipping
//	if( x + 9 * len >= ScreenWidth || y > ScreenHeight - 12 )
//		return;
	
	StartDrawing(); // get the address of the backbuffer in Screen.Screen
	
	for( int i = 0; i < len; i++, x+=8 )
	{
		if( *(s+i) == ' ' ) // skip spaces, just move 8 pixels further
			continue;
		DrawLetter(x, y, *(s+i));
	}

	StopDrawing();
}

/*void EASYDRAW::DrawFormattedString(int x, int y, const char *fmt, ...)
{
	va_list ap;
	char buff[128];

	va_start(ap, fmt);
	vsprintf(buff, "%d", ap);

	DrawString(x, y, buff);
}*/

void EASYDRAW::DrawLetter(int x, int y, char c)
{
	// Screen.Screen is filled with the address of where the backbuffer is
	int h = 12;
	unsigned char *arrayoffset = alphabet + (c - '!') * 96 + 8; // '!' is index 0 in the alfabet array
	U16ptr pos = (Screen.Screen) + (y * Screen.RealWidth) + (x);

	while( h-- )
	{
		for( register i = 0; i < 8; i++, arrayoffset++ )
		{
			if( *arrayoffset == 0 )
				*(pos + i) = color;
		}
		pos+=Screen.RealWidth;
	}
} 

void EASYDRAW::SetColorKey(U16 col, Image *image)
{
	DDCOLORKEY ddck;
	ddck.dwColorSpaceLowValue  = col; // these are the only two mebers of DDCOLORKEY
	ddck.dwColorSpaceHighValue = col;

	image->surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
}

void EASYDRAW::SetColorRange(U16 low, U16 high, Image *image)
{
	DDCOLORKEY ddck;
	ddck.dwColorSpaceLowValue  = low;
	ddck.dwColorSpaceHighValue = high;

	image->surface->SetColorKey(DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ddck);
}

void EASYDRAW::SetColorKey(U16 col, int which) // overloaded function
{
	DDCOLORKEY ddck;

	ddck.dwColorSpaceLowValue = col;
	ddck.dwColorSpaceHighValue = col;

	OffSurface[which].surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
}

void EASYDRAW::SetRenderTarget(ImageID which) // easydraw will draw on which from now on
{
	m_RenderTarget = &OffSurface[which];
}

void EASYDRAW::PlotPixel(int x, int y)
{
	if( Screen.Screen == 0 )
		return;

	U16ptr ptr = Screen.Screen + (y * ScreenWidth) + x;
	*ptr = color;
}

/***********************************************************
Bresenham's line algorithm
***********************************************************/
void EASYDRAW::Line(U32 x1, U32 y1, U32 x2, U32 y2)
{
	StartDrawing();

	int SC_WIDTH = Screen.RealWidth, SC_HEIGHT = ScreenHeight;

	int y_unit, x_unit;
	int ydiff = y2 - y1;

	if( ydiff < 0 )
	{
		ydiff = -ydiff;
		y_unit = -1;
	}
	else
		y_unit = 1;

	int xdiff = x2 - x1;

	if( xdiff < 0 )
	{
		xdiff = -xdiff;
		x_unit = -1;
	}
	else
		x_unit = 1;

	int error_term = 0;

	if( xdiff > ydiff )
	{
		int length = xdiff + 1;
		for( int i = 0; i < length; i++ )
		{
			*(Screen.Screen + (y1 * ScreenWidth) + x1) = color; // plotpixel without function overhead
			x1 += x_unit;
			error_term += ydiff;
			if( error_term > xdiff )
			{
				error_term -= xdiff;
				y1 += y_unit;
			}
		}
	}
	else
	{
		int length = ydiff + 1;
		for( int i = 0; i < length; i++ )
		{
			*(Screen.Screen + (y1 * ScreenWidth) + x1) = color; // plotpixel without function overhead
			y1 += y_unit;
			error_term += xdiff;
			if( error_term > 0 )
			{
				error_term -= ydiff;
				x1 += x_unit;
			}
		}
	}

	StopDrawing();
}

void EASYDRAW::HorizontalLine(int x1, int x2, int y)
{
	StartDrawing();

	if( x2 > ScreenWidth ) x2 = ScreenWidth;
	if( y > ScreenHeight || y < 0 ) return;
	if( x1 < 0 ) x1 = 0;

	U16ptr pos = Screen.Screen + (y * Screen.RealWidth) + x1;

	Memset16Bit(pos, color, x2 - x1);

	StopDrawing();
}

void EASYDRAW::VerticalLine(int y1, int y2, int x)
{
	StartDrawing();

	int num = y2 - y1;
	U16ptr pos = Screen.Screen + (y1 * Screen.RealWidth) + x;

	while( num-- )
	{
		*pos = color;
		pos+=Screen.RealWidth;
	}

	StopDrawing();
}

void EASYDRAW::FrameRect(RECT &rect)
{
	FrameRect(rect.top, rect.left, rect.right, rect.bottom);
}

void EASYDRAW::FrameRect(int x1, int y1, int x2, int y2)
{
	HorizontalLine(x1, x2, y1);
	HorizontalLine(x1, x2, y2);
	VerticalLine(y1, y2, x1);
	VerticalLine(y1, y2, x2);
}

void EASYDRAW::FillRect(RECT &rect)
{
	FillRect(rect.left, rect.top, rect.right, rect.bottom);
}

void EASYDRAW::FillRect(int x1, int y1, int x2, int y2)
{
	int num = y2 - y1;

	while( num-- )
		HorizontalLine(x1, x2, y1++);
}

void EASYDRAW::SetClipList(RGNDATA *data)
{
	clipper->SetClipList(data, 0);
}

void EASYDRAW::SetColor(U16 col)
{
	color = col;
}

void EASYDRAW::ReplaceColor(ImageID which, U16 replacecolor, const U16 searchcolor)
{	
	LockImage(&OffSurface[which]);

	OffSurface[which].surface->Unlock(0);

	int n = OffSurface[which].Width * OffSurface[which].Height;
	U16ptr vidmem = (U16ptr)Screen.Screen;

	while( n-- )
	{
		if( *vidmem == searchcolor )
			*vidmem = replacecolor;

		vidmem++;
	}
}

void EASYDRAW::InvertColor(int which)
{
	LockImage(&OffSurface[which]);
	OffSurface[which].surface->Unlock(0);

	int n = OffSurface[which].Width * OffSurface[which].Height;
	U16ptr vidmem = (U16ptr)Screen.Screen;	

	while( n-- )
	{
		*vidmem = ~*vidmem; // invert colors pixel by pixel
		vidmem++;
	}
}

/* caller has to make sure values range from 0..31 for r and b, and 63 for g */
U16 EASYDRAW::Convert16Bit(U8 r, U8 g, U8 b)
{
	if( ColorType == _555_ )
		return (U16) ((U16)b) | ((U16)g << 5) | ((U16)r << 10);
	else // colortype == _565_
		return (U16) ((U16)b) | ((U16)g << 5) | ((U16)r << 11);
}

void EASYDRAW::ReleaseSurface(ImageID which)
{
	if( which < 0 || which >= MAXIMAGES || OffSurface[which].surface == 0 )
		return;

	OffSurface[which].surface->Release();
	OffSurface[which].surface = 0;
	OffSurface[which].Width  = 0;
	OffSurface[which].Height = 0;
	OffSurfaceCount--;
	ImageList.push(which);
}

int EASYDRAW::FirstEmptySurface()
{
	if( ImageList.empty() )
		return FAIL;
	else
	{
		int i = ImageList.top();
		ImageList.pop();
		return i;
	}
}

/* restores ONLY images and NOT animations */
int EASYDRAW::RestoreImages()
{
	if (FAILED(DrawObject->RestoreAllSurfaces()))
		return FAIL;

	for (int i = 0; i < MAXIMAGES; i++)
	{
		if (OffSurface[i].surface != 0)
		{
			DescribeOffScreenSurface(OffSurface[i].Width, OffSurface[i].Height, 0);

			switch (GetImage(i)->type)
			{
				case ED_BITMAP :
					OffSurface[i].LoadBitmap(OffSurface[i].filepath);
					break;

				case ED_TARGA :
					OffSurface[i].LoadTarga(OffSurface[i].filepath, this);
					break;

				case ED_EASYFORMAT :
					OffSurface[i].LoadEasyFormat(OffSurface[i].filepath, this);
					break;

				case ED_JPG :
					OffSurface[i].LoadJPG(OffSurface[i].filepath, this);
					break;
			}
		}
	}

	return 0;
}

int EASYDRAW::GetScreenWidth()
{
	return ScreenWidth;
}

int EASYDRAW::GetScreenHeight()
{
	return ScreenHeight;
}

int EASYDRAW::GetOffsurfaceCount()
{
	return OffSurfaceCount;  
}
