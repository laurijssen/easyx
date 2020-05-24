#include <easydraw.h>
#include <ijl.h> // intel jpeg lib

/********************************************************************************
here the Image class members are defined
********************************************************************************/
Image::Image()
{
	surface = NULL;
	Width = 0;
	Height = 0;
}

Image::~Image()
{
	if (surface)
	{
		surface->Release();
		surface = 0;
	}
}

/***********************************************************************
EASYDRAW's own file format. It uses stretchhing to create any kind of surface
you want
************************************************************************/
bool Image::LoadEasyFormat(char *path, EASYDRAW *DDraw)
{
	HDC Dest = NULL, Source = NULL;
	LPDIRECTDRAWSURFACE7 TempSurface;
	EASYDRAW_HEADER header;
	U16ptr ptr;
	int ret;
	FILE *fp = NULL;
	bool bReturn = false;
	
	try
	{
		fp = fopen(path, "r+b"); // open the file

		if( fp == NULL )
			throw exception("Error opening file\n");

		filepath = path;
		type = ED_EASYFORMAT;

		fread(&header, sizeof header, 1, fp); // read the header from file

		DDraw->DescribeOffScreenSurface(header.Width, header.Height, DDSCAPS_SYSTEMMEMORY);
		DDraw->DrawObject->CreateSurface(&DDraw->Descriptor, &TempSurface, NULL); // data will be stretched from this surface

		ret = TempSurface->Lock(NULL, &DDraw->Descriptor, DDLOCK_SURFACEMEMORYPTR, NULL); // get its address
		TempSurface->Unlock(NULL);

		ptr = (U16ptr)DDraw->Descriptor.lpSurface; // here's the address

		fread(ptr, sizeof U16, header.Height * header.Width, fp); // read in the data from file

		TempSurface->GetDC(&Source); // the device context so we can use GDI's StretchBlt
		surface->GetDC(&Dest);

		if( StretchBlt(Dest, 0, 0,  Width, Height, Source, 0, 0, 
						header.Width, header.Height, SRCCOPY) == false )
			throw exception("Error in StretchBlt(...)\n");

		bReturn = true;
	} catch( exception &e )
	{
		OutputDebugString(e.what());
	}

	if( Dest )
		surface->ReleaseDC(Dest);
	if( TempSurface )
	{
		TempSurface->ReleaseDC(Source);
		TempSurface->Release();
	}
	if( fp )
		fclose(fp);

	return bReturn;
}

/* writes the simple easyformat to file */
void Image::SaveEasyFormat(char *path)
{
	FILE *fp = fopen(path, "w+b");

	U16ptr ptr = GetAddress();

	EASYDRAW_HEADER header(this->Width, this->Height);
	
	fwrite(&header, sizeof header, 1, fp); // write the header to file
	fwrite(ptr, sizeof U16, header.Width * header.Height, fp); // write the pixel data to file

	fclose(fp);
}

/******************************************************
load a bitmap using windows GDI
******************************************************/
bool Image::LoadBitmap(char * ImageName)
{
	HGDIOBJ OldHandle;
    HBITMAP hbm;
    HDC hdcSurf  = NULL;
    HDC hdcImage = NULL;
    bool bReturn = false;    
	
	try
	{		
		filepath = ImageName;
		type = ED_BITMAP;

		hbm = (HBITMAP)LoadImage(NULL, ImageName, IMAGE_BITMAP, Width, Height,
								 LR_LOADFROMFILE | LR_CREATEDIBSECTION);

		if( hbm == NULL )
			throw exception("error in LoadImage(...)\n");

		hdcImage = CreateCompatibleDC(0);
		OldHandle = SelectObject(hdcImage, hbm);
				
		if( FAILED(surface->GetDC(&hdcSurf)) )
			throw exception("GetDC failed\n");
		
		if( BitBlt(hdcSurf, 0, 0, Width, Height, hdcImage, 0, 0, SRCCOPY) == false )
			throw exception("Error in BitBlt\n");

		bReturn = true;
	}catch( exception &e)
	{
		OutputDebugString(e.what());
	}	
    if( hdcSurf )
        surface->ReleaseDC(hdcSurf);
    if( hdcImage )
        DeleteDC(hdcImage);
    if( hbm )
        DeleteObject(SelectObject(hdcImage, OldHandle));

    return bReturn;
} /* LoadImage */

void Image::SaveBitmap(char *path)
{
	FILE *fp;
	HDC hdc, hdcMem;
    HBITMAP  hbm;
    BITMAPFILEHEADER  header;
	BITMAPINFO bmInfo;
	BYTE * pBits;
	DWORD dwCount;
	
	if( (fp = fopen(path, "w+b")) == NULL )
		return;

	surface->GetDC(&hdc);

	hdcMem = CreateCompatibleDC(hdc);
	hbm = CreateCompatibleBitmap(hdc, Width, Height);

	SelectObject(hdcMem, hbm);
	if( BitBlt(hdcMem, 0, 0, Width, Height, hdc, 0, 0, SRCCOPY) == FALSE )
		return;

	dwCount = Width * Height * 3;

	header.bfType = MAKEWORD('B', 'M');
	header.bfSize = sizeof(header) + sizeof(BITMAPINFOHEADER) + dwCount;
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);

    bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth = Width;
    bmInfo.bmiHeader.biHeight = Height;
    bmInfo.bmiHeader.biPlanes = 1;
    bmInfo.bmiHeader.biBitCount = 24;
    bmInfo.bmiHeader.biCompression = 0;
    bmInfo.bmiHeader.biSizeImage = 0;
    bmInfo.bmiHeader.biXPelsPerMeter = 0;
    bmInfo.bmiHeader.biYPelsPerMeter = 0;
    bmInfo.bmiHeader.biClrUsed = 0;
    bmInfo.bmiHeader.biClrImportant = 0;

	pBits = (BYTE *)malloc(dwCount);

	GetDIBits(hdcMem, hbm, 0, Height, pBits, &bmInfo, DIB_RGB_COLORS);

	fwrite(&header, sizeof(header), 1, fp);
    fwrite(&bmInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1,  fp);
   	fwrite(pBits, dwCount, 1, fp);

	free(pBits);
	surface->ReleaseDC(hdc);
	DeleteObject(hbm);
	fclose(fp);
} /* SaveBitmap */

bool Image::LoadTarga(char *ImageName, EASYDRAW *DDraw)
{
	FILE *fp = NULL;
	LPDIRECTDRAWSURFACE7 temp = NULL;
	TARGA_HEADER header; // defined in image.h	
	U16ptr ptr;
	bool bResult = false;
	HDC Dest = NULL, Source = NULL;

	try {
		fp = fopen(ImageName, "r+b");
		if( fp == NULL ) // something went wrong with the file
			throw exception("File not found\n");

		fread(&header, sizeof(TARGA_HEADER), 1, fp);

		DDraw->DescribeOffScreenSurface(header.Width, header.Height, DDSCAPS_VIDEOMEMORY);
		DDraw->DrawObject->CreateSurface(&DDraw->Descriptor, &temp, NULL);

		// retrieve the surface pointer	
		memset(&DDraw->Descriptor, 0, sizeof DDraw->Descriptor);
		DDraw->Descriptor.dwSize = sizeof DDraw->Descriptor;
		if( FAILED(temp->Lock(NULL, &DDraw->Descriptor, DDLOCK_SURFACEMEMORYPTR, NULL)) )
			throw exception("error during lock\n");

		temp->Unlock(NULL);

		if( header.PixelDepth == 16 )
		{	
			for( int i = header.Height - 1; i >= 0; i-- )
			{
				ptr = (U16ptr)DDraw->Descriptor.lpSurface + i * header.Width; 
				fread(ptr, sizeof U16, header.Width, fp); // read in the colordata line by line				
			}			
			if( DDraw->ColorType == EASYDRAW::_565_ ) // if the videocard uses 6 bits for green, convert the bits
			{
				ptr = (U16ptr)DDraw->Descriptor.lpSurface;
				ConvertTo_5x6x5(ptr, Width * Height);
			}
		}
		else if( header.PixelDepth == 24 ) 
		{		
			RGBVal *read = new RGBVal[Width+1];

			for( int i = header.Height - 1; i >= 0; i-- )
			{
				ptr = (U16ptr)DDraw->Descriptor.lpSurface + i * header.Width; 
				fread(read, sizeof RGBVal, Width, fp); // read in the colordata line by line
				ConvertTo16Bit(ptr, read, Width, DDraw->ColorType); // and convert it
			}

			delete[] read;
		} 
		else
			throw exception("Unsupported pixeldepth\n");

		filepath = ImageName;
		type = ED_TARGA;

		temp->GetDC(&Source); // the device context so we can use GDI's StretchBlt
		surface->GetDC(&Dest);

		if( StretchBlt(Dest, 0, 0,  Width, Height, Source, 0, 0, 
						header.Width, header.Height, SRCCOPY) == false )
			throw exception("StretchBlt error\n");

		bResult = true;
	}
	catch( exception &e )
	{
		OutputDebugString(e.what());
	}

// finally {}
	if( Dest )
		surface->ReleaseDC(Dest);

	if( temp )
	{
		temp->ReleaseDC(Source);
		temp->Release();
	}
	if( fp != NULL )
		fclose(fp);

	return bResult;
}

void Image::SaveTarga(char *path)
{
	FILE *fp;
	TARGA_HEADER header;
	if( (fp = fopen(path, "w+b")) == NULL )
		return;

	U16ptr ptr = GetAddress();	
	header.Width = Width;
	header.Height = Height;
	header.PixelDepth = 16;	

	fclose(fp);
}

bool Image::LoadJPG(char *ImageName, EASYDRAW *DDraw)
{
	JPEG_CORE_PROPERTIES jcprops;
	bool bResult = false;
	BYTE *buffer24 = 0;
	WORD *buffer16 = 0;

	try
	{
		if( ijlInit(&jcprops) != IJL_OK )
			throw exception("error during ijlInit\n");

		type = ED_JPG;
		filepath = ImageName;
		jcprops.JPGFile = ImageName;

		if( ijlRead(&jcprops, IJL_JFILE_READPARAMS) != IJL_OK )
			throw exception("error during jpeg parameter reading\n");

		long lbuff24 = (jcprops.JPGWidth * 24 + 7) / 8 * jcprops.JPGHeight;
		buffer24 = new BYTE[lbuff24];

		jcprops.DIBWidth = jcprops.JPGWidth;
		jcprops.DIBHeight = jcprops.JPGHeight;
		jcprops.DIBColor = IJL_BGR;
		jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES(jcprops.JPGWidth, 3);
		jcprops.DIBBytes = reinterpret_cast<BYTE *>(buffer24);

		switch( jcprops.JPGChannels )
		{
			case 1 :
				jcprops.JPGColor = IJL_G; break;
			case 3 :
				jcprops.JPGColor = IJL_YCBCR; break;
			default :
				jcprops.JPGColor = (IJL_COLOR)IJL_OTHER;
				jcprops.DIBColor = (IJL_COLOR)IJL_OTHER;
				break;
		}

		if( ijlRead(&jcprops, IJL_JFILE_READWHOLEIMAGE ) != IJL_OK )
			throw exception("error during image reading\n");

		// now we have 24-bit color data, but we need 16-bit, so conversion is needed
		long lbuff16 = ((jcprops.JPGWidth * 16 + 7) / 8) * jcprops.JPGHeight;
		buffer16 = new WORD[lbuff16];
		long j = 0;
		for( long i = 0; i < lbuff24; i += 3 ) // every 3 bytes will be one word
		{
			if( DDraw->ColorType == EASYDRAW::_555_ )
				buffer16[j++] = RGB555(buffer24[i], buffer24[i+1], buffer24[i+2]);
			else
				buffer16[j++] = RGB565(buffer24[i], buffer24[i+1], buffer24[i+2]);
		}

		HBITMAP hbm = CreateBitmap(jcprops.DIBWidth, jcprops.DIBHeight, 1, 16, buffer16);
		if( !hbm )
			throw exception("couldn't create bitmap from memory\n");

		HDC hdc, hdcMem;
		hdcMem = CreateCompatibleDC(0);
		HGDIOBJ old = SelectObject(hdcMem, hbm);

		surface->GetDC(&hdc);

		if( !StretchBlt(hdc, 0, 0, Width, Height, 
						hdcMem, 0, 0, jcprops.DIBWidth, jcprops.DIBHeight, SRCCOPY) )
		{
			surface->ReleaseDC(hdc);
			DeleteObject(SelectObject(hdcMem, old));
			DeleteDC(hdcMem);
			throw exception("error during stretchblt\n");
		}

		surface->ReleaseDC(hdc);
		DeleteObject(SelectObject(hdcMem, old));
		DeleteDC(hdcMem);

		bResult = true;
	}
	catch( exception &e )
	{
		OutputDebugString(e.what());
	}

	if( buffer16 )
		delete[] buffer16;
	if( buffer24 )
		delete[] buffer24;
	ijlFree(&jcprops);

	return bResult;
}

void FC Image::Draw(int x, int y, EASYDRAW *DDraw)
{
	LPDIRECTDRAWSURFACE7 backbuffer = DDraw->BackBuffer.GetSurface();
	RECT dest = {x, y, Width, Height};
	backbuffer->Blt(&dest, surface, NULL, 0, NULL);
}

void Image::SetRelativeBrightness(int ColorType, U8 r, U8 g, U8 b)
{
	U16 col;
	U16ptr ptr = GetAddress();
	int num = Width * Height;	

	r &= 0x1F, b &= 0x1F; // always mask to 5 bits

	if( ColorType == EASYDRAW::_555_ )
	{
		g &= 0x1F; // mask the 5 lsb's

		while( num-- )
		{
			col = ((U16)b) | ((U16)g) << 5 | ((U16)r) << 10;
			*ptr += col;
			ptr++;
		}
	}
	else
	{
		g &= 0x3F; // mask the 6 lsb's

		while( num-- )
		{
			col = ((U16)b) | ((U16)g) << 5 | ((U16)r) << 11;
			*ptr += col;
			ptr++;
		}
	}
}

void Image::operator~()
{
	U16ptr ptr = GetAddress();

	int num = Width * Height;
	while( num-- )
	{
		*ptr = ~*ptr;
		ptr++;
	}
}

void Image::ConvertTo_5x6x5(U16ptr ptr, int num)
{
	U16 RMask = 31<<10; // 1111100000000000
	U16 GMask = 31<<5;  // 0000001111100000
	U16 BMask = 31;     // 0000000000011111

	U32 r,g,b;

	while( num-- )
	{
		r = *ptr & RMask;
		g = *ptr & GMask;
		b = *ptr & BMask;

		*ptr++ = (U16)((r<<1) + (g<<1) + b); // shift red and green by 1
	}
}

/********************************************************************
Some videocards have 15bit color and some have 16 bit color.
Change the colors acoordingly
********************************************************************/

void Image::ConvertTo16Bit(U16ptr buff, RGBVal *read, int num, int ColorType)
{
	if( ColorType == EASYDRAW::_555_ )
	{
		while( num-- )
		{
			*buff = read->Convert15();
			buff++, read++;
		}
	}
	else
	{
		while( num-- )
		{
			*buff = read->Convert16();
			buff++, read++;
		}
	}
}

U16ptr Image::GetAddress()
{
	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof ddsd;

	while (surface->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
		;
	surface->Unlock(NULL);

	return (U16ptr)ddsd.lpSurface;
}

Overlay::Overlay(int w, int h, char *file, EASYDRAW *DDraw)
{
	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
	ddsd.dwWidth = w, ddsd.dwHeight = h;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFourCC = 0;
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 16; // we are in 16-bit for certain
	ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
	ddsd.ddpfPixelFormat.dwGBitMask = 0x03E0;
	ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;
	ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

	// first try _555_ bit layout
	if( FAILED(DDraw->GetDrawObject()->CreateSurface(&ddsd, &surface, NULL)) )
	{
		ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x07E0;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;
		if( FAILED(DDraw->GetDrawObject()->CreateSurface(&ddsd, &surface, NULL)) )
		{
			DDraw->ErrorMessage("Couldn't create overlay surface");
			return;
		}
	}

	Width = w, Height = h, type = ED_OVERLAY;
	if( !LoadBitmap(file) )
		DDraw->ErrorMessage("Error in LoadBitmap()");
}

Overlay::~Overlay() {}

void Overlay::Draw(int x, int y, EASYDRAW *DDraw)
{
	if( !surface ) return;

	DDSURFACEDESC2 ddsd = {0};	
	DDCAPS         caps = {0};

	ddsd.dwSize = sizeof ddsd;
	caps.dwSize = sizeof caps;
	surface->GetSurfaceDesc(&ddsd); // we want to get the width of the src surface
	DDraw->GetDrawObject()->GetCaps(&caps, NULL);

	RECT src, dest;
	src.left   = 0, src.top = 0;
	src.bottom = ddsd.dwHeight;
	src.right  = ddsd.dwWidth;
	dest.left  = x, dest.top = y;	

	if( caps.dwCaps & DDCAPS_ALIGNSIZESRC ) // adjust the source rectangle
		src.right = AlignDown(src.right, caps.dwAlignSizeSrc);

	if( (caps.dwMinOverlayStretch <= 4000 && // stretching 4000 <--> * 4
		 caps.dwMaxOverlayStretch >= 4000) )
	{ 
		dest.right = ((src.right * 4000 + 999) / 1000);
	}
	else
	{
		dest.right = ((src.right * caps.dwMinOverlayStretch + 999) / 1000);
		if( dest.right == 0 ) dest.right = src.right;
	}

	if( caps.dwCaps & DDCAPS_ALIGNSIZEDEST ) // adjust the dest rectangle
	{
		dest.right = AlignUp(dest.right, caps.dwAlignSizeDest);
	}

	dest.bottom = dest.right;

	DWORD dwFlags = DDOVER_SHOW;
	surface->UpdateOverlay(&src, DDraw->GetScreenBuffer()->GetSurface(), &dest, dwFlags, NULL);
}

void Overlay::Hide(EASYDRAW *DDraw)
{
	surface->UpdateOverlay(NULL, DDraw->GetScreenBuffer()->GetSurface(), NULL, DDOVER_HIDE, NULL);
}

void Overlay::SetPosition(int x, int y)
{
	surface->SetOverlayPosition(x, y);
}

DWORD Overlay::AlignDown(DWORD dwNumber, DWORD dwAlignment)
{
	if( dwNumber == 1 ) return dwNumber;
	return dwNumber - (dwNumber % dwAlignment);
}

DWORD Overlay::AlignUp(DWORD dwNumber, DWORD dwAlignment)
{
	DWORD dwResult;

	if( dwNumber == 1 ) return dwNumber;
	dwResult = dwNumber + (dwAlignment - 1);
	dwResult -= dwNumber % dwAlignment;
	return dwResult;
}


AnimFrame::AnimFrame() {}

AnimFrame::AnimFrame(int w, int h, EASYDRAW *DDraw)
{
	Count = 0;
	Time = 16;
	Index = DDraw->CreateImage(w, h, 0);
}

void AnimFrame::SetTime(U8 time)
{
	this->Time = min(time, 255); // there are 8-bits reserved for time, so 255 is max
}

//********************************************************************

void AnimFrame::DrawCurrent(int x, int y, EASYDRAW *DDraw, U32 param)
{
	DDraw->DrawImage(x, y, Index, param);
}

//********************************************************************/

int AnimFrame::Draw(int x, int y, EASYDRAW *DDraw, U32 param)
{
	DDraw->DrawImage(x, y, Index, param);

	Count++;

	if( Count >= Time ) // is it time?
	{
		Count = 0;
		return TIME_EXCEEDED; // indicates that the next frame should be drawn
	}
	else
		return OK;
}

/********************************************************************

Release this frame from memory

********************************************************************/

void AnimFrame::Release(EASYDRAW *DDraw)
{
	DDraw->ReleaseSurface(Index);
}

int FC AnimFrame::NextCount()
{
	Count++;

	if( Count >= Time )
	{
		Count = 0;
		return TIME_EXCEEDED;
	}
	else
		return OK;
}

/********************************************************************


********************************************************************/

Animation::Animation(int startx, int starty, int w, int h, int framecount, std::string &file, EASYDRAW *DDraw)
{
	frames = new AnimFrame *[framecount]; // initialize the array

	this->DDraw = DDraw;
	this->framecount = framecount;
	currentframe = 0;
	LoadAnimation(startx, starty, w, h, file);
}

Animation::~Animation()
{
	for( int i = 0; i < framecount; i++ ) 
	{
		frames[i]->Release(DDraw); // delete the frames
		delete frames[i];
		frames[i] = 0;
	}

	delete[] frames; // delete the array
	frames = 0;
}

void Animation::LoadAnimation(int startx, int starty, int w, int h, std::string &file)
{
	int i;
		
	//create the prepared picture
	ImageID totalpic = DDraw->CreateImage(framecount * w, h, 0, (char *)file.c_str());

	for( i = 0; i < framecount; i++ ) // create framecount surfaces of the same size
		frames[i] = new AnimFrame(w, h, DDraw);

	for( i = 0; i < framecount; i++ )
		DDraw->CopyBitmap(frames[i]->GetIndex(), totalpic, startx + i * w, starty, w, h);
	
	DDraw->ReleaseSurface(totalpic);	
}

void FC Animation::DrawFrame(int x, int y, U32 param)
{
	if( frames[currentframe]->Draw(x, y, DDraw, param) == AnimFrame::TIME_EXCEEDED )
	{
		currentframe++;
		if( currentframe == framecount )
			currentframe = 0;
	}
}

void FC Animation::DrawCurrentFrame(int x, int y, U32 param) const
{
	frames[currentframe]->DrawCurrent(x, y, DDraw, param);
}

void FC Animation::SkipFrame()
{
	currentframe+=2;
	if( currentframe >= framecount )
		currentframe = 0;
}

void FC Animation::NextFrame()
{
	if( frames[currentframe]->NextCount() == AnimFrame::TIME_EXCEEDED )
	{
		currentframe++;
		if( currentframe >= framecount )
			currentframe = 0;
	}
}

void FC Animation::SetTimeFrame(U8 time, int which)
{
	int i;

	if( which == -1 ) // set all frames
	{
		for( i = 0; i < framecount; i++ )
			frames[i]->SetTime(time); 
	}
	else
		frames[which]->SetTime(time);
}

void FC Animation::SetColorKey(U16 color) const
{
	for( int i = 0; i < framecount; i++ )
		DDraw->SetColorKey(color, frames[i]->GetIndex());
}
