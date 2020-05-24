/***************************************************************
program converts bitmaps and later maybe targa's to easydrawformat
author: Servé Laurijssen
****************************************************************/

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <ddraw.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <easydraw.h>

using namespace std;

LPDIRECTDRAW7 lpdd;
LPDIRECTDRAWSURFACE7 lpddPrimary;
LPDIRECTDRAWSURFACE7 lpddOffScreen;

void InitDDraw();
void ReleaseAll();
void ConvertBitmap(char *file);
bool LoadBitmap(LPDIRECTDRAWSURFACE7 surface, char *file);
void SaveEasyFormat(LPDIRECTDRAWSURFACE7 surface, char *file);

int main(int argc, char **argv)
{
	int retcode = 0;

	if( argc <= 1 )
	{
		cerr << "Too few arguments given...\n\n";
		cerr << "This program converts bitmaps to\n"
			    "16-bit EasyDraw Format (EDF) files\n";
		cerr << "Give the complete bitmap files as argument\n";
		cerr << "Press a key to continue\n";
		cin >> retcode;
		retcode = 1;
	}
	else
	{		
		InitDDraw();
		argv++; // put the command line ptr at the first 'real' parameter
		while( *argv )
		{
			ConvertBitmap(*argv);
			argv++;
		}
	}

	ReleaseAll();

	return retcode;
}

void ConvertBitmap(char *file)
{
	BITMAPFILEHEADER header;
	BITMAPINFOHEADER info;
	FILE *fp = fopen(file, "rb");

	if( !fp )
	{
		cout << "File does not exist: " << file << endl;
		return;
	}

	fread(&header, sizeof header, 1, fp);
	fread(&info, sizeof info, 1, fp);

	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwHeight = info.biHeight;
	ddsd.dwWidth = info.biWidth;

	if( FAILED(lpdd->CreateSurface(&ddsd, &lpddOffScreen, NULL)) )
		MessageBox(NULL, "Error during createsurface", "Error", MB_ICONHAND|MB_OK|MB_DEFBUTTON1);

	if( LoadBitmap(lpddOffScreen, file) )
		SaveEasyFormat(lpddOffScreen, file);

	lpddOffScreen->Release();
	lpddOffScreen = NULL;

	fclose(fp);
}

void SaveEasyFormat(LPDIRECTDRAWSURFACE7 surface, char *file)
{
	char newfile[MAX_PATH];
	unsigned short *mem;

	int ret;
	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof ddsd;

	do
	{
		ret = surface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_SURFACEMEMORYPTR, NULL);
	}
	while( ret != DD_OK );

	strcpy(newfile, file);
	strcpy(strchr(newfile, '.'), ".edf");	

	mem = (unsigned short *)ddsd.lpSurface;
	
	FILE *fp = fopen(newfile, "wb");
	EASYDRAW_HEADER header(ddsd.lPitch >> 1, ddsd.dwHeight);
	fwrite(&header, sizeof header, 1, fp);
	fwrite(mem, ddsd.dwWidth * ddsd.dwHeight, sizeof(unsigned short), fp);

	cout << newfile << " is converted to easyformat" << endl;

	surface->Unlock(NULL);
	fclose(fp);
}

bool LoadBitmap(LPDIRECTDRAWSURFACE7 surface, char *file)
{
	bool bReturn = false;
	HGDIOBJ OldHandle;
    HBITMAP hbm;
    HDC hdcSurf  = NULL;
    HDC hdcImage = NULL;    
	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof ddsd;
	surface->GetSurfaceDesc(&ddsd);

	try
	{
		hbm = (HBITMAP)LoadImage(NULL, file, IMAGE_BITMAP, ddsd.dwWidth, ddsd.dwHeight,
								 LR_LOADFROMFILE | LR_CREATEDIBSECTION);

		if( hbm == NULL )
			throw exception("error in LoadImage(...)\n");

		hdcImage = CreateCompatibleDC(NULL);
		OldHandle = SelectObject(hdcImage, hbm);

		if( FAILED(surface->GetDC(&hdcSurf)) )
			throw exception("GetDC failed\n");

		if( BitBlt(hdcSurf, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcImage, 0, 0, SRCCOPY) == false )
			throw exception("Error in BitBlt\n");

		bReturn = true;

	} catch( exception &e)
	{
		cout << e.what() << endl;
	}

    if( hdcSurf )
        surface->ReleaseDC(hdcSurf);
    if( hdcImage )
        DeleteDC(hdcImage);
    if( hbm )
        DeleteObject(hbm);
	if( OldHandle )
		DeleteObject(OldHandle);

	return bReturn;
}

void InitDDraw()
{
	if( FAILED(DirectDrawCreateEx(NULL, (void **)&lpdd, IID_IDirectDraw7, NULL)) )
		MessageBox(NULL, "Error during create", "Error", MB_ICONHAND|MB_OK|MB_DEFBUTTON1);
	lpdd->SetCooperativeLevel(GetDesktopWindow(), DDSCL_NORMAL);
	lpdd->SetDisplayMode(1024, 768, 16, 0, 0);

	DDSURFACEDESC2 ddsd = {0};
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if( FAILED(lpdd->CreateSurface(&ddsd, &lpddPrimary, NULL)) )
		MessageBox(NULL, "Error during createsurface", "Error", MB_ICONHAND|MB_OK|MB_DEFBUTTON1);	
}

void ReleaseAll()
{
	if( lpddOffScreen )
		lpddOffScreen->Release();
	if( lpddPrimary )
		lpddPrimary->Release();
	if( lpdd )
		lpdd->Release();
}
