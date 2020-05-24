#include "utilities.h"
#include <stdexcept>

bool easyx::LoadBitmap(LPDIRECTDRAW7 pdd, LPDIRECTDRAWSURFACE7 *dest, char *file)
{
	BITMAP bm;
	HBITMAP hbm;
	HDC hdcSurf = 0, hdcImg = 0;
	HGDIOBJ old;
	DDSURFACEDESC2 ddsd = {0};
	bool result = false;

	try
	{
		hbm = (HBITMAP)LoadImage(0, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

		if( !hbm )
			throw exception("error loading bitmap\n");

		GetObject(hbm, sizeof bm, &bm);

		ddsd.dwSize = sizeof ddsd;
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth = bm.bmWidth;
		ddsd.dwHeight = bm.bmHeight;

		if( FAILED(pdd->CreateSurface(&ddsd, dest, 0)) )
			throw exception("error in CreateSurface\n");

		hdcImg = CreateCompatibleDC(0);
		old = SelectObject(hdcImg, hbm);

		if( FAILED((*dest)->GetDC(&hdcSurf)) )
			throw exception("error in GetDC\n");

		if( BitBlt(hdcSurf, 0, 0, bm.bmWidth, bm.bmHeight, hdcImg, 0, 0, SRCCOPY) == false )
			throw exception("error in BitBlt\n");

		result = true;
	} catch( exception &e )
	{
		OutputDebugString(e.what());
	}

	if( hdcSurf )
		(*dest)->ReleaseDC(hdcSurf);

	if( hdcImg )
	{
		if( hbm )
			DeleteObject(SelectObject(hdcImg, old));
		DeleteDC(hdcImg);
	}

	return result;
}


U16 easyx::Convert16Bit(U8 r, U8 g, U8 b)
{
	return (((U16)b) | ((U16)g << 5) | ((U16)b << 10));
}

POINT easyx::SlideMapTilePlotter(int x, int y, int tilewidth, int tileheight)
{
	POINT pt;

	pt.x = x * tilewidth + y * tilewidth / 2;
	pt.y = y * tileheight / 2;

	return pt;
}