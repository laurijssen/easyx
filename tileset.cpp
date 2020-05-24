#include <tileset.h>
#include <easydraw.h>
#include <comdef.h>
#include <fstream>
#include <globals.h>
#include "utilities.h"

static void LogError(const char *file, const char *errormessage)
{
	using namespace std;
	fstream os(file, ios::out | ios::app);

	os << errormessage << endl;
}

CTileSet::CTileSet()
{
	m_TileCount = 0;
	TileList = 0;
	TileSet = 0;
	reload = 0;
}

CTileSet::~CTileSet()
{
	Unload();
}

void CTileSet::DrawTile(LPDIRECTDRAWSURFACE7 where, int x, int y, DWORD which)
{
	OffsetRect(&framerect, x, y);
	
	where->Blt(&framerect, TileSet->surface, &TileList[which].src, DDBLT_WAIT | DDBLT_KEYSRC, 0);

	OffsetRect(&framerect, -x, -y);
}

void CTileSet::Reload()
{
	LPDIRECTDRAW7 pdd;
	TileSet->surface->GetDDInterface((void **)&pdd);
	pdd->RestoreAllSurfaces();

	Load(pdd, reload);
}

void CTileSet::Unload()
{
	if( TileSet )
	{
		delete TileSet;
		TileSet = 0;
	}
	if( reload )
	{
		free(reload);
		reload = 0;
	}
	if( TileList )
	{
		delete[] TileList;
		TileList = 0;
		m_TileCount = 0;
	}
}

bool CTileSet::Load(LPDIRECTDRAW7 dd, char *file)
{
	DDSURFACEDESC2 ddsd = {0};
	U16ptr ptr;
	int w, h, rowcount, colcount;

	Unload(); // if one was previously loaded, unload it

	TileSet = new Image;
	reload = strdup(file);

	if( !easyx::LoadBitmap(dd, &TileSet->surface, file) )
		return false;
	
	ddsd.dwSize = sizeof ddsd;

	if( FAILED(TileSet->surface->Lock(0, &ddsd, DDLOCK_SURFACEMEMORYPTR, 0)) )
		return false;

	ptr = (U16ptr)ddsd.lpSurface;
	w = ddsd.dwWidth; //ddsd.lPitch >> 1;
	h = ddsd.dwHeight;

	U16 transparent = ptr[w-1];
	
	colcount = 0; // count the columns (the horizontal part) by counting the transparency pixels
	for( int x = 1; x < w; x++ ) {
		if( ptr[x] == transparent )
			colcount++;
	}

	rowcount = 0;
	for( int y = 1; y < h; y++ ) {
		if( ptr[y*(ddsd.lPitch>>1)] == transparent )
			rowcount++;
	}

	m_TileCount = rowcount * colcount;
	TileList = new TILEINFO[m_TileCount];

	for( y = 0; y < rowcount; y++ ) {
		for( x = 0; x < colcount; x++ ) 
		{
			SetRect(&TileList[(y*colcount)+x].src, x*(w/colcount)+1, y*(h/rowcount)+1,
					x*(w/colcount)+(w/colcount), y*(h/rowcount)+(h/rowcount));			
		}
	}

	SetRect(&framerect, 0, 0, w/colcount, h/rowcount);

	TileSet->surface->Unlock(0);

	DDCOLORKEY ddck;
	ddck.dwColorSpaceHighValue = transparent;
	ddck.dwColorSpaceLowValue = transparent;
	TileSet->surface->SetColorKey(DDCKEY_SRCBLT, &ddck);

	return true;
}
