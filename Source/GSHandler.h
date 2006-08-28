#ifndef _GS_H_
#define _GS_H_

#include "Types.h"
#include "Stream.h"

#pragma pack(push, 1)

enum GS_REGS
{
	GS_REG_PRIM			= 0x00,
	GS_REG_RGBAQ		= 0x01,
	GS_REG_ST			= 0x02,
	GS_REG_UV			= 0x03,
	GS_REG_XYZF2		= 0x04,
	GS_REG_XYZ2			= 0x05,
	GS_REG_TEX0_1		= 0x06,
	GS_REG_TEX0_2		= 0x07,
	GS_REG_CLAMP_1		= 0x08,
	GS_REG_CLAMP_2		= 0x09,
	GS_REG_FOG			= 0x0A,
	GS_REG_XYZF3		= 0x0C,
	GS_REG_XYZ3			= 0x0D,
	GS_REG_TEX1_1		= 0x14,
	GS_REG_TEX1_2		= 0x15,
	GS_REG_TEX2_1		= 0x16,
	GS_REG_TEX2_2		= 0x17,
	GS_REG_XYOFFSET_1	= 0x18,
	GS_REG_XYOFFSET_2	= 0x19,
	GS_REG_PRMODECONT	= 0x1A,
	GS_REG_PRMODE		= 0x1B,
	GS_REG_TEXCLUT		= 0x1C,
	GS_REG_TEXA			= 0x3B,
	GS_REG_FOGCOL		= 0x3D,
	GS_REG_TEXFLUSH		= 0x3F,
	GS_REG_SCISSOR_1	= 0x40,
	GS_REG_ALPHA_1		= 0x42,
	GS_REG_ALPHA_2		= 0x43,
	GS_REG_TEST_1		= 0x47,
	GS_REG_TEST_2		= 0x48,
	GS_REG_FRAME_1		= 0x4C,
	GS_REG_FRAME_2		= 0x4D,
	GS_REG_ZBUF_1		= 0x4E,
	GS_REG_ZBUF_2		= 0x4F,
	GS_REG_BITBLTBUF	= 0x50,
	GS_REG_TRXPOS		= 0x51,
	GS_REG_TRXREG		= 0x52,
	GS_REG_TRXDIR		= 0x53,
};

struct GSTEX0
{
	unsigned int	nBufPtr			: 14;
	unsigned int	nBufWidth		: 6;
	unsigned int	nPsm			: 6;
	unsigned int	nWidth			: 4;
	unsigned int	nPad0			: 2;
	unsigned int	nPad1			: 2;
	unsigned int	nColorComp		: 1;
	unsigned int	nFunction		: 2;
	unsigned int	nCBP			: 14;
	unsigned int	nCPSM			: 4;
	unsigned int	nCSM			: 1;
	unsigned int	nCSA			: 5;
	unsigned int	nCLD			: 3;
	uint32			GetBufPtr()		{ return nBufPtr * 256; }
	uint32			GetBufWidth()	{ return nBufWidth * 64; }
	uint32			GetWidth()		{ return (1 << nWidth); }
	uint32			GetHeight()		{ return (1 << (nPad0 | (nPad1 << 2))); }
	uint32			GetCLUTPtr()	{ return nCBP * 256; }
};

struct GSTEX1
{
	unsigned int	nLODMethod		: 1;
	unsigned int	nReserved0		: 1;
	unsigned int	nMaxMip			: 3;
	unsigned int	nMagFilter		: 1;
	unsigned int	nMinFilter		: 3;
	unsigned int	nMipBaseAddr	: 1;
	unsigned int	nReserved1		: 9;
	unsigned int	nLODL			: 2;
	unsigned int	nReserved2		: 11;
	unsigned int	nLODK			: 7;
	unsigned int	nReserved3		: 25;
};

struct GSRGBAQ
{
	uint8			nR;
	uint8			nG;
	uint8			nB;
	uint8			nA;
	float			nQ;
};

struct GSDISPLAY
{
	unsigned int	nX			: 12;
	unsigned int	nY			: 11;
	unsigned int	nMagX		: 4;
	unsigned int	nMagY		: 5;
	unsigned int	nW			: 12;
	unsigned int	nH			: 12;
};

struct GSPRIM
{
	unsigned int	nType			: 3;
	unsigned int	nShading		: 1;
	unsigned int	nTexture		: 1;
	unsigned int	nFog			: 1;
	unsigned int	nAlpha			: 1;
	unsigned int 	nAntiAliasing	: 1;
	unsigned int	nUseUV			: 1;
	unsigned int	nContext		: 1;
	unsigned int	nUseFloat		: 1;
};

struct GSPRMODE
{
	unsigned int	nReserved0		: 3;
	unsigned int	nShading		: 1;
	unsigned int	nTexture		: 1;
	unsigned int	nFog			: 1;
	unsigned int	nAlpha			: 1;
	unsigned int 	nAntiAliasing	: 1;
	unsigned int	nUseUV			: 1;
	unsigned int	nContext		: 1;
	unsigned int	nUseFloat		: 1;
};

struct GSTEXA
{
	unsigned int	nTA0			: 8;
	unsigned int	nReserved0		: 7;
	unsigned int	nAEM			: 1;
	unsigned int	nReserved1		: 16;
	unsigned int	nTA1			: 8;
	unsigned int	nReserved2		: 24;
};

struct GSALPHA
{
	unsigned int	nA			: 2;
	unsigned int	nB			: 2;
	unsigned int	nC			: 2;
	unsigned int	nD			: 2;
	unsigned int	nReserved0	: 24;
	unsigned int	nFix		: 8;
	unsigned int	nReserved1	: 24;
};

struct GSTEST
{
	unsigned int	nAlphaEnabled		: 1;
	unsigned int	nAlphaMethod		: 3;
	unsigned int	nAlphaRef			: 8;
	unsigned int	nAlphaFail			: 2;
	unsigned int	nDestAlphaEnabled	: 1;
	unsigned int	nDestAlphaMode		: 1;
	unsigned int	nDepthEnabled		: 1;
	unsigned int	nDepthMethod		: 2;
	unsigned int	nReserved0			: 13;
	uint32			nReserved1;
};

#pragma pack(pop)

#define DECODE_DISPLAY(v, d)					\
	(d) = *(GSDISPLAY*)&(v);

#define DECODE_PRIM(v, pr)						\
	(pr) = *(GSPRIM*)&(v);

#define DECODE_PRMODE(v, prm)					\
	(prm) = *(GSPRMODE*)&(v);

#define DECODE_TEXA(v, texa)					\
	(texa) = *(GSTEXA*)&(v);

#define DECODE_TEX0(v, tex)						\
	(tex) = *(GSTEX0*)&(v);

#define DECODE_TEX1(v, tex)						\
	(tex) = *(GSTEX1*)&(v);

#define DECODE_RGBAQ(v, rgbaq)					\
	(rgbaq) = *(GSRGBAQ*)&(v);

#define DECODE_UV(p, u, v)						\
	(u)  = (double)((p >>  4) & 0xFFF);			\
	(u) += (double)((p >>  0) & 0xF) / 16.0f;	\
	(v)  = (double)((p >> 20) & 0xFFF);			\
	(v) += (double)((p >> 16) & 0xF) / 16.0f;

#define DECODE_ST(p, s, t)						\
	(s)  = *(float*)((uint32*)&(p) + 0);		\
	(t)  = *(float*)((uint32*)&(p) + 1);

#define DECODE_XYZ2(v, x, y, z)						\
	(x)  = (double)(((v >>  0) & 0xFFFF)) / 16.0;	\
	(y)  = (double)(((v >> 16) & 0xFFFF)) / 16.0;	\
	(z)  = (double)((v >> 32) & 0xFFFFFFFF);

#define DECODE_XYOFFSET(v, x, y)				\
	(x)  = (double)((v >>  4) & 0xFFF);			\
	(x) += (double)((v >>  0) & 0xF) / 16.0f;	\
	(y)  = (double)((v >> 36) & 0xFFF);			\
	(y) += (double)((v >> 32) & 0xF) / 16.0f;	

#define DECODE_ALPHA(v, alpha)					\
	(alpha) = *(GSALPHA*)&(v);

#define DECODE_TEST(v, tst)						\
	(tst) = *(GSTEST*)&(v);

class CGSHandler
{
public:
											CGSHandler();
	virtual									~CGSHandler();

	void									Reset();

	virtual void							SaveState(Framework::CStream*);
	virtual void							LoadState(Framework::CStream*);

	virtual void							WritePrivRegister(uint32, uint32);
	virtual uint32							ReadPrivRegister(uint32);
	void									DisassembleWrite(uint8, uint64);
	void									SetVBlank();
	void									ResetVBlank();

	virtual void							WriteRegister(uint8, uint64);
	void									FeedImageData(void*, uint32);

	void									FetchImagePSCMT16(uint16*, uint32, uint32, uint32, uint32);

	virtual void							SetCrt(bool, unsigned int, bool);
	virtual void							UpdateViewport()						= 0;
	virtual void							ProcessImageTransfer(uint32, uint32)	= 0;
	virtual void							Flip()									= 0;

	enum PRIVATE_REGISTER
	{
		GS_CSR = 0x12001000,
		GS_IMR = 0x12001010,
	};

protected:
	enum RAMSIZE
	{
		RAMSIZE = 0x00400000,
	};

	enum PSM
	{
		PSMCT32		= 0x00,
		PSMCT24		= 0x01,
		PSMCT16		= 0x02,
		PSMCT16S	= 0x0A,
		PSMT8		= 0x13,
		PSMT4		= 0x14,
		PSMT8H		= 0x1B,
		PSMT4HL		= 0x24,
		PSMT4HH		= 0x2C,
		PSMZ32		= 0x30,
		PSMZ24		= 0x31,
		PSMZ16		= 0x32,
		PSMZ16S		= 0x3A,
		PSM_MAX,
	};

	struct DISPFB
	{
		unsigned int	nBufPtr			: 9;
		unsigned int	nBufWidth		: 6;
		unsigned int	nPSM			: 5;
		unsigned int	nReserved0		: 12;
		unsigned int	nX				: 11;
		unsigned int	nY				: 11;
		unsigned int	nReserved1		: 10;
		uint32			GetBufPtr()		{ return nBufPtr * 8192; };
		uint32			GetBufWidth()	{ return nBufWidth * 64; };
	};

	//Reg 0x04/0x0C
	struct XYZF
	{
		unsigned int	nX				: 16;
		unsigned int	nY				: 16;
		unsigned int	nZ				: 24;
		unsigned int	nF				: 8;

		double			GetX()			{ return (double)nX / 16.0; }
		double			GetY()			{ return (double)nY / 16.0; }
	};

	//Reg 0x08/0x09
	struct CLAMP
	{
		unsigned int	nWMS			: 2;
		unsigned int	nWMT			: 2;
		unsigned int	nMINU			: 10;
		unsigned int	nMAXU			: 10;
		unsigned int	nReserved0		: 8;
		unsigned int	nReserved1		: 2;
		unsigned int	nMAXV			: 10;
		unsigned int	nReserved2		: 22;
		unsigned int	GetMinU()		{ return nMINU;	}
		unsigned int	GetMaxU()		{ return nMAXU;	}
		unsigned int	GetMinV()		{ return (nReserved0) | (nReserved1 << 8); }
		unsigned int	GetMaxV()		{ return nMAXV; }
	};

	//Reg 0x16/0x17
	struct TEX2
	{
		unsigned int	nReserved0		: 20;
		unsigned int	nPsm			: 6;
		unsigned int	nReserved1		: 6;
		unsigned int	nReserved2		: 5;
		unsigned int	nCBP			: 14;
		unsigned int	nCPSM			: 4;
		unsigned int	nCSM			: 1;
		unsigned int	nCSA			: 5;
		unsigned int	nCLD			: 3;
		uint32			GetCLUTPtr()	{ return nCBP * 256; }
	};

	//Reg 0x3D
	struct FOGCOL
	{
		unsigned int	nFCR			: 8;
		unsigned int	nFCG			: 8;
		unsigned int	nFCB			: 8;
		unsigned int	nReserved0		: 8;
		unsigned int	nReserved1		: 32;
	};

	//Reg 0x3F
	struct TEXCLUT
	{
		unsigned int	nCBW			: 6;
		unsigned int	nCOU			: 6;
		unsigned int	nCOV			: 10;
		unsigned int	nReserved1		: 10;
		unsigned int	nReserved2		: 32;
		uint32			GetBufWidth()	{ return nCBW * 64; }
		uint32			GetOffsetU()	{ return nCOU * 16; }
		uint32			GetOffsetV()	{ return nCOV; }
	};

	//Reg 0x4C/0x4D
	struct FRAME
	{
		unsigned int	nPtr			: 16;
		unsigned int	nWidth			: 8;
		unsigned int	nPsm			: 8;
		unsigned int	nMask			: 32;
		uint32			GetBasePtr()	{ return nPtr * 8192; }
		uint32			GetWidth()		{ return nWidth * 64; }
	};

	//Reg 0x4E/0x4F
	struct ZBUF
	{
		unsigned int	nPtr			: 9;
		unsigned int	nReserved0		: 15;
		unsigned int	nPsm			: 4;
		unsigned int	nReserved1		: 4;
		unsigned int	nMask			: 1;
		uint32			GetBasePtr()	{ return nPtr * 2048; }
	};

	//Reg 0x50
	struct BITBLTBUF
	{
		unsigned int	nSrcPtr			: 14;
		unsigned int	nReserved0		: 2;
		unsigned int	nSrcWidth		: 6;
		unsigned int	nReserved1		: 2;
		unsigned int	nSrcPsm			: 6;
		unsigned int	nReserved2		: 2;
		unsigned int	nDstPtr			: 14;
		unsigned int	nReserved3		: 2;
		unsigned int	nDstWidth		: 6;
		unsigned int	nReserved4		: 2;
		unsigned int	nDstPsm			: 6;
		unsigned int	nReserved5		: 2;
		uint32			GetSrcPtr()		{ return nSrcPtr * 256; }
		uint32			GetSrcWidth()	{ return nSrcWidth * 64; }
		uint32			GetDstPtr()		{ return nDstPtr * 256; }
		uint32			GetDstWidth()	{ return nDstWidth * 64; }
	};

	//Reg 0x51
	struct TRXPOS
	{
		unsigned int	nSSAX			: 11;
		unsigned int	nReserved0		: 5;
		unsigned int	nSSAY			: 11;
		unsigned int	nReserved1		: 5;
		unsigned int	nDSAX			: 11;
		unsigned int	nReserved2		: 5;
		unsigned int	nDSAY			: 11;
		unsigned int	nDIR			: 2;
		unsigned int	nReserved3		: 3;
	};

	//Reg 0x52
	struct TRXREG
	{
		unsigned int	nRRW			: 12;
		unsigned int	nReserved0		: 20;
		unsigned int	nRRH			: 12;
		unsigned int	nReserved1		: 20;
	};

	struct TRXCONTEXT
	{
		uint32			nSize;
		uint32			nRRX;
		uint32			nRRY;
		bool			nDirty;
	};

	struct STORAGEPSMCT32
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 64 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 32 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 8 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 8 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 8 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 2 };

		static int m_nBlockSwizzleTable[4][8];
		static int m_nColumnSwizzleTable[2][8];

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

		static int m_nBlockSwizzleTable[8][4];
		static int m_nColumnSwizzleTable[2][16];

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

		static int m_nBlockSwizzleTable[8][4];
		static int m_nColumnSwizzleTable[2][16];

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

		static int m_nBlockSwizzleTable[4][8];
		static int m_nColumnWordTable[2][2][8];

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

		static int m_nBlockSwizzleTable[8][4];
		static int m_nColumnWordTable[2][2][8];

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
			uint32 nAddress;
			nAddress = GetColumnAddress(nX, nY);
			return &((typename Storage::Unit*)&m_pMemory[nAddress])[Storage::m_nColumnSwizzleTable[nY][nX]];
		}

	private:
		uint32 GetColumnAddress(unsigned int& nX, unsigned int& nY)
		{
			uint32 nPageNum, nBlockNum, nColumnNum;

			nPageNum = (nX / Storage::PAGEWIDTH) + (nY / Storage::PAGEHEIGHT) * (m_nWidth * 64) / Storage::PAGEWIDTH;

			nX %= Storage::PAGEWIDTH;
			nY %= Storage::PAGEHEIGHT;

			nBlockNum = Storage::m_nBlockSwizzleTable[nY / Storage::BLOCKHEIGHT][nX / Storage::BLOCKWIDTH];

			nX %= Storage::BLOCKWIDTH;
			nY %= Storage::BLOCKHEIGHT;

			nColumnNum = (nY / Storage::COLUMNHEIGHT);

			nY %= Storage.COLUMNHEIGHT;

			return (m_nPointer + (nPageNum * PAGESIZE) + (nBlockNum * BLOCKSIZE) + (nColumnNum * COLUMNSIZE)) & (RAMSIZE - 1);
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

	typedef bool (CGSHandler::*TRANSFERHANDLER)(void*, uint32);

	//General Purpose Registers
	TEXCLUT*								GetTexClut();
	FRAME*									GetFrame(unsigned int);
	FOGCOL*									GetFogCol();
	TRXREG*									GetTrxReg();
	TRXPOS*									GetTrxPos();
	BITBLTBUF*								GetBitBltBuf();

	//Privileged Regsiters
	DISPFB*									GetDispFb(unsigned int);

	unsigned int							GetCrtWidth();
	unsigned int							GetCrtHeight();
	bool									GetCrtIsInterlaced();
	bool									GetCrtIsFrameMode();

	TRANSFERHANDLER							m_pTransferHandler[PSM_MAX];

	bool									TrxHandlerInvalid(void*, uint32);
	template <typename Storage> bool		TrxHandlerCopy(void*, uint32);
	bool									TrxHandlerPSMT4(void*, uint32);
	bool									TrxHandlerPSMCT24(void*, uint32);
	bool									TrxHandlerPSMT8H(void*, uint32);
	template <uint32, uint32> bool			TrxHandlerPSMT4H(void*, uint32);

	unsigned int							GetPsmPixelSize(unsigned int);

	uint64									m_nPMODE;			//0x12000000
	uint64									m_nDISPFB1;			//0x12000070
	uint64									m_nDISPLAY1;		//0x12000080
	uint64									m_nDISPFB2;			//0x12000090
	uint64									m_nDISPLAY2;		//0x120000A0
	uint64									m_nCSR;				//0x12001000
	uint64									m_nIMR;				//0x12001010

	TRXCONTEXT								m_TrxCtx;

	uint64									m_nReg[0x80];

	uint8*									m_pRAM;

	unsigned int							m_nCrtMode;
	bool									m_nCrtIsInterlaced;
	bool									m_nCrtIsFrameMode;
};

//////////////////////////////////////////////
//Some storage methods templates specializations

template <> uint8 CGSHandler::CPixelIndexor<CGSHandler::STORAGEPSMT4>::GetPixel(unsigned int nX, unsigned int nY)
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

/*
template <> void CGSHandler::CPixelIndexor<CGSHandler::STORAGEPSMT4>::SetPixel(unsigned int nX, unsigned int nY, uint8 nPixel)
{
	typedef STORAGEPSMT4 Storage;

	uint32 nAddress;
	uint8* pPixel;
	unsigned int nColumnNum, nSubTable, nShiftAmount;

	nColumnNum = (nY / Storage::COLUMNHEIGHT) & 0x01;
	nAddress = GetColumnAddress(nX, nY);

	nShiftAmount	=	(nX & 0x18);
	nShiftAmount	+=	(nY & 0x02) << 1;
	nSubTable		=	(nY & 0x02) >> 1;
	nSubTable		^=	(nColumnNum);

	nX &= 0x07;
	nY &= 0x01;

	pPixel = reinterpret_cast<uint8*>(&(((uint32*)&m_pMemory[nAddress])[Storage::m_nColumnWordTable[nSubTable][nY][nX]]));

	(*pPixel) &= (0xF		<< nShiftAmount);
	(*pPixel) |= (nPixel	<< nShiftAmount);
}
*/

template <> uint8* CGSHandler::CPixelIndexor<CGSHandler::STORAGEPSMT8>::GetPixelAddress(unsigned int nX, unsigned int nY)
{
	typedef CGSHandler::STORAGEPSMT8 Storage;

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

#endif
