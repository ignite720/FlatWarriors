#include "GameMapUtils.h"

#define __IS_SOLID(x, y, z)		(mapData.colors[ (x) ][ (y) ][ (z) ] != V(kMapColorPaletteIndex::Air))

#define __WRAP_EDGE() \
do \
{ \
	if (wrapEdge) \
	{ \
		x &= (kMapSizeX - 1); \
		y &= (kMapSizeY - 1); \
	} \
} while(false)

namespace FlatWarriors
{
	bool IsValidPos(int x, int y, int z)
	{
		return ((x >= 0 && x < kMapSizeX) && (y >= 0 && y < kMapSizeY) && (z >= 0 && z < kMapSizeZ));
	}
		
	bool IsSolid(const MapData &mapData, int x, int y, int z, bool wrapEdge)
	{
		__WRAP_EDGE();

		if (!IsValidPos(x, y, z)) { return false; }
		return __IS_SOLID(x, y, z);
	}

	bool HasColour(const MapData &mapData, int x, int y, int z)
	{
		if (!__IS_SOLID(x, y, z)) { return false; }

		if ((x > 0) && !__IS_SOLID(x - 1, y, z)) { return true; }
		if (((x + 1) < kMapSizeX) && !__IS_SOLID(x + 1, y, z)) { return true; }

		if ((y > 0) && !__IS_SOLID(x, y - 1, z)) { return true; }
		if (((y + 1) < kMapSizeY) && !__IS_SOLID(x, y + 1, z)) { return true; }

		if ((z > 0) && !__IS_SOLID(x, y, z - 1)) { return true; }
		if (((z + 1) < kMapSizeZ) && !__IS_SOLID(x, y, z + 1)) { return true; }
		return false;
	}
	
	void SetBlockGeometry(MapData &mapData, int x, int y, int z, bool solid)
	{
		mapData.colors[x][y][z] = V(solid ? kMapColorPaletteIndex::Solid : kMapColorPaletteIndex::Air);
	}
		
	void SetBlockColor(MapData &mapData, int x, int y, int z, palette_index_t value)
	{
		mapData.colors[x][y][z] = value;
	}
	
	void ReadBlockColorIndex(const BBuffer &mapBuffer, MapData &mapData, int x, int y, int z, int pos)
	{
		SetBlockColor(mapData, x, y, z, mapBuffer[pos + 1]);
	}
		
	void WriteBlockColor(BBuffer &dst, const U8Vector3 &color)
	{
		dst.push_back(color.r);
		dst.push_back(color.g);
		dst.push_back(color.b);
	}
	
	void Read(const BBuffer &mapBuffer, MapData &mapData)
	{
		if (mapBuffer.empty()) { return; }

		int pos = 0;

		for (int i = 0; i < kMapColorPaletteSize; ++i)
		{
			ReadBlockColor(mapBuffer, mapData.palette[i], pos);
			pos += 3;
		}

		memset(mapData.colors, static_cast<int>(kMapColorPaletteIndex::Solid), sizeof(mapData.colors));

		for (int y = 0; y < kMapSizeY; ++y)
		{
			for (int x = 0; x < kMapSizeX; ++x)
			{
				for (int z = 0; z < kMapSizeZ; ++z)
				{
					SetBlockColor(mapData, x, y, z, V(kMapColorPaletteIndex::NonColoredBlock));
				}

				const int numAirBlockSpans = mapBuffer[pos + 0];
				const int numColoredBlocks = mapBuffer[pos + 1];
				pos += 2;

				for (int i = 0; i < numAirBlockSpans; ++i)
				{
					const int airStart = mapBuffer[pos + i * 2 + 0];
					const int airEnd = mapBuffer[pos + i * 2 + 1];
					for (int j = airStart; j < airEnd; ++j)
					{
						SetBlockGeometry(mapData, x, y, j, false);
					}
				}
				pos += (numAirBlockSpans * 2);

				for (int i = 0; i < numColoredBlocks; ++i)
				{
					int z = mapBuffer[pos + 0];
					ReadBlockColorIndex(mapBuffer, mapData, x, y, z, pos);
					pos += 2;
				}
			}
		}
	}

	void Write(const MapData &mapData, BBuffer &v)
	{
		v.reserve(kMapSizeX * kMapSizeZ * kMapSizeZ);

		for (int i = 0; i < kMapColorPaletteSize; ++i)
		{
			WriteBlockColor(v, mapData.palette[i]);
		}

		std::vector<std::pair<u8, u8>> airBlockSpans;
		std::vector<u8> coloredBlocks;

		for (int y = 0; y < kMapSizeY; ++y)
		{
			for (int x = 0; x < kMapSizeX; ++x)
			{
				airBlockSpans.clear();
				coloredBlocks.clear();

				for (int z = 0; z < kMapSizeZ; ++z)
				{
					if (HasColour(mapData, x, y, z)) { coloredBlocks.push_back(z); }
				}

				{
					int z = 0;
					while (z < kMapSizeZ)
					{
						int airBegin = z;
						while (!IsSolid(mapData, x, y, z) && (z < kMapSizeZ)) { ++z; }
						airBlockSpans.emplace_back(airBegin, z);

						while (IsSolid(mapData, x, y, z)) { ++z; }
					}
				}

				v.push_back(static_cast<u8>(airBlockSpans.size()));
				v.push_back(static_cast<u8>(coloredBlocks.size()));

				for (const auto z : airBlockSpans)
				{
					v.push_back(z.first);
					v.push_back(z.second);
				}
				for (const auto z : coloredBlocks)
				{
					v.push_back(z);
					v.push_back(mapData.colors[x][y][z]);
				}
			}
		}
	}
	
	void GenerateFlatTerrain(MapData &mapData)
	{
		memset(mapData.colors, static_cast<int>(kMapColorPaletteIndex::Air), sizeof(mapData.colors));
		for (int y = 0; y < kMapSizeY; ++y)
		{
			for (int x = 0; x < kMapSizeX; ++x)
			{
				int z = 0;

				int solidsBegin = (kMapBedRockZ - 1);
				for (z = solidsBegin; z < kMapSizeZ; ++z)
				{
					SetBlockColor(mapData, x, y, z, 0);
				}
			}
		}
	}
	
	void SaveTo(const char *path, const MapData &mapData)
	{
		MapData tmp;
		GenerateFlatTerrain(tmp);
		
		BBuffer buf;
		Write(tmp, buf);
		
		{
			FILE *fp = fopen(path, "wb");
			if (fp)
			{
				fwrite(&buf[0], 1, buf.size(), fp);
				fclose(fp);
			}
		}
	}
}