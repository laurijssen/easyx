#include <WorldMap.h>

U8 Tile::width = 0;
U8 Tile::height = 0;

Tile::Tile() { LoadTileData(); }

void Tile::SetWidth(U8 w) { if( !Tile::width ) Tile::width = w; }

void Tile::SetHeight(U8 h) { if( !Tile::height ) Tile::height = h; }

WorldMap::WorldMap(U16 row, U16 col) : rows(row), cols(col) 
{
	map = new Tile**[row];
	for( int i = 0; i < row; i++ )
		map[i] = new Tile *[col];
}

WorldMap::~WorldMap()
{
	delete[] map;
}

void WorldMap::LoadMap(char *file) throw(FileError)
{
	FILE *fp;

	if( !(fp = fopen(file, "r+b")) )
		throw FileError(file);

	fclose(fp);
}