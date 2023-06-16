#pragma once

#include <stdint.h>

#include <array>
#include <vector>

namespace FlatWarriors
{
	typedef uint8_t u8;
	using BBuffer = std::vector<u8>;
	
	typedef struct tagU8Vector3
	{
		union {
			struct {
				u8 x, y, z;
			};
			struct {
				u8 r, g, b;
			};
			u8 v[3];
		};
	} U8Vector3;
	
	static constexpr int kMapSizeX = 512;
	static constexpr int kMapSizeY = 512;
	static constexpr int kMapSizeZ = 64;

	static constexpr int kMapColorPaletteSize = 0x100;
	using MapColorPalette = std::array<U8Vector3, kMapColorPaletteSize>;
	using MapColorIndices = u8[kMapSizeX][kMapSizeY][kMapSizeZ];
	struct MapData
	{
		MapColorPalette palette;
		MapColorIndices colors;
	};
	
	using palette_index_t = u8;
	enum class kMapColorPaletteIndex : palette_index_t
	{
		Solid = 7,
		NonColoredBlock = 0xFD,
		Air = 0xFF,
	};

	void Read(const BBuffer &mapBuffer, MapData &mapData);
	void Write(const MapData &mapData, BBuffer &v);
	
	void GenerateFlatTerrain(MapData &mapData);
	void SaveTo(const char *path, const MapData &mapData);
}