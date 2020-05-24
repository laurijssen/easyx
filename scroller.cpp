#include <scroller.h>
#include <utilities.h>

void Scroller::SetupSpace(CTileSet *tileset, int tilewidth, int tileheight, int mapwidth, int mapheight)
{
	using easyx::SlideMapTilePlotter;

	SetRect(&rcScreen, 0, 0, 1024, 768);

	RECT rcTile1, rcTile2;
	POINT ptPlot, ptMap;

	tsIso = tileset;
	CopyRect(&rcTile1, &tileset->GetTileList()[0].src);
	CopyRect(&rcTile2, &tileset->GetTileList()[0].src);

	ptMap.x = 0;
	ptMap.y = 0;
	ptPlot = SlideMapTilePlotter(ptMap.x, ptMap.y, tilewidth, tileheight);
	OffsetRect(&rcTile1, ptPlot.x, ptPlot.y);

	ptMap.x = mapwidth-1;
	ptMap.y = mapheight-1;
	ptPlot = SlideMapTilePlotter(ptMap.x, ptMap.y, tilewidth, tileheight);
	OffsetRect(&rcTile2, ptPlot.x, ptPlot.y);

	UnionRect(&rcWorld, &rcTile1, &rcTile2);
	CopyRect(&rcAnchor, &rcWorld);

	rcAnchor.right -= (rcScreen.right - rcScreen.left);
	if( rcAnchor.right < rcAnchor.left )
		rcAnchor.right = rcAnchor.left;

	rcAnchor.bottom -= (rcScreen.bottom - rcScreen.top);
	if( rcAnchor.bottom < rcAnchor.top )
		rcAnchor.bottom = rcAnchor.top;

	ptScreenAnchor.x = 0;
	ptScreenAnchor.y = 0;
}

void Scroller::DrawMap()
{
}