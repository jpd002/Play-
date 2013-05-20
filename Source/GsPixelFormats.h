#pragma once

#include "Types.h"
#include "GSHandler.h"

class CGsPixelFormats
{
public:
	struct STORAGEPSMCT32
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 64 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 32 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 8 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 8 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 8 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 2 };

		static const int m_nBlockSwizzleTable[4][8];
		static const int m_nColumnSwizzleTable[2][8];

		typedef uint32 Unit;
	};

	struct STORAGEPSMCT16
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 64 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 64 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 16 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 8 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 16 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 2 };

		static const int m_nBlockSwizzleTable[8][4];
		static const int m_nColumnSwizzleTable[2][16];

		typedef uint16 Unit;
	};

	struct STORAGEPSMCT16S
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 64 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 64 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 16 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 8 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 16 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 2 };

		static const int m_nBlockSwizzleTable[8][4];
		static const int m_nColumnSwizzleTable[2][16];

		typedef uint16 Unit;
	};

	struct STORAGEPSMT8
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 128 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 64 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 16 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 16 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 16 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 4 };

		static const int m_nBlockSwizzleTable[4][8];
		static const int m_nColumnWordTable[2][2][8];

		typedef uint8 Unit;
	};

	struct STORAGEPSMT4
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 128 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 128 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 32 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 16 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 32 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 4 };

		static const int m_nBlockSwizzleTable[8][4];
		static const int m_nColumnWordTable[2][2][8];

		typedef uint8 Unit;
	};

	template <typename Storage> class CPixelIndexor
	{
	public:
		CPixelIndexor(uint8* pMemory, uint32 nPointer, uint32 nWidth)
		{
			m_nPointer		= nPointer;
			m_nWidth		= nWidth;
			m_pMemory		= pMemory;
		}

		typename Storage::Unit GetPixel(unsigned int nX, unsigned int nY)
		{
			return *GetPixelAddress(nX, nY);
		}

		void SetPixel(unsigned int nX, unsigned int nY, typename Storage::Unit nPixel)
		{
			*GetPixelAddress(nX, nY) = nPixel;
		}

		typename Storage::Unit* GetPixelAddress(unsigned int nX, unsigned int nY)
		{
			uint32 nAddress = GetColumnAddress(nX, nY);
			return &((typename Storage::Unit*)&m_pMemory[nAddress])[Storage::m_nColumnSwizzleTable[nY][nX]];
		}

	private:
		uint32 GetColumnAddress(unsigned int& nX, unsigned int& nY)
		{
			uint32 nPageNum = (nX / Storage::PAGEWIDTH) + (nY / Storage::PAGEHEIGHT) * (m_nWidth * 64) / Storage::PAGEWIDTH;

			nX %= Storage::PAGEWIDTH;
			nY %= Storage::PAGEHEIGHT;

			uint32 nBlockNum = Storage::m_nBlockSwizzleTable[nY / Storage::BLOCKHEIGHT][nX / Storage::BLOCKWIDTH];

			nX %= Storage::BLOCKWIDTH;
			nY %= Storage::BLOCKHEIGHT;

			uint32 nColumnNum = (nY / Storage::COLUMNHEIGHT);

			nY %= Storage::COLUMNHEIGHT;

			return (m_nPointer + (nPageNum * PAGESIZE) + (nBlockNum * BLOCKSIZE) + (nColumnNum * COLUMNSIZE)) & (CGSHandler::RAMSIZE - 1);
		}

		enum PAGESIZE
		{
			PAGESIZE = 8192,
		};

		enum BLOCKSIZE
		{
			BLOCKSIZE = 256,
		};

		enum COLUMNSIZE
		{
			COLUMNSIZE = 64,
		};

		uint32		m_nPointer;
		uint32		m_nWidth;
		uint8*		m_pMemory;
	};

	typedef CPixelIndexor<STORAGEPSMCT32>	CPixelIndexorPSMCT32;
	typedef CPixelIndexor<STORAGEPSMCT16>	CPixelIndexorPSMCT16;
	typedef CPixelIndexor<STORAGEPSMCT16S>	CPixelIndexorPSMCT16S;
	typedef CPixelIndexor<STORAGEPSMT8>		CPixelIndexorPSMT8;
	typedef CPixelIndexor<STORAGEPSMT4>		CPixelIndexorPSMT4;
};

//////////////////////////////////////////////
//Some storage methods templates specializations

template <> 
inline uint8 CGsPixelFormats::CPixelIndexor<CGsPixelFormats::STORAGEPSMT4>::GetPixel(unsigned int nX, unsigned int nY)
{
	typedef STORAGEPSMT4 Storage;

	uint32 nAddress;
	unsigned int nColumnNum, nSubTable, nShiftAmount;

	nColumnNum = (nY / Storage::COLUMNHEIGHT) & 0x01;
	nAddress = GetColumnAddress(nX, nY);

	nShiftAmount	=	(nX & 0x18);
	nShiftAmount	+=	(nY & 0x02) << 1;
	nSubTable		=	(nY & 0x02) >> 1;
	nSubTable		^=	(nColumnNum);

	nX &= 0x07;
	nY &= 0x01;

	return (uint8)(((uint32*)&m_pMemory[nAddress])[Storage::m_nColumnWordTable[nSubTable][nY][nX]] >> nShiftAmount) & 0x0F;
}

template <> 
inline void CGsPixelFormats::CPixelIndexor<CGsPixelFormats::STORAGEPSMT4>::SetPixel(unsigned int nX, unsigned int nY, uint8 nPixel)
{
	typedef STORAGEPSMT4 Storage;

	uint32 nAddress;
	unsigned int nColumnNum, nSubTable, nShiftAmount;

	nColumnNum = (nY / Storage::COLUMNHEIGHT) & 0x01;
	nAddress = GetColumnAddress(nX, nY);

	nShiftAmount	=	(nX & 0x18);
	nShiftAmount	+=	(nY & 0x02) << 1;
	nSubTable		=	(nY & 0x02) >> 1;
	nSubTable		^=	(nColumnNum);

	nX &= 0x07;
	nY &= 0x01;

	uint32* pPixel = &(((uint32*)&m_pMemory[nAddress])[Storage::m_nColumnWordTable[nSubTable][nY][nX]]);

	(*pPixel) &= ~(0xF		<< nShiftAmount);
	(*pPixel) |=  (nPixel	<< nShiftAmount);
}

template <> 
inline uint8* CGsPixelFormats::CPixelIndexor<CGsPixelFormats::STORAGEPSMT8>::GetPixelAddress(unsigned int nX, unsigned int nY)
{
	typedef CGsPixelFormats::STORAGEPSMT8 Storage;

	unsigned int nByte, nTable;
	uint32 nColumnNum, nOffset;

	nColumnNum = (nY / Storage::COLUMNHEIGHT) & 0x01;
	nOffset = GetColumnAddress(nX, nY);

	nTable			=	(nY & 0x02) >> 1;
	nByte			=	(nX & 0x08) >> 2;
	nByte			+=	(nY & 0x02) >> 1;
	nTable			^=	(nColumnNum);

	nX &= 0x7;
	nY &= 0x1;

	return reinterpret_cast<uint8*>(&((uint32*)&m_pMemory[nOffset])[Storage::m_nColumnWordTable[nTable][nY][nX]]) + nByte;
}
