#ifndef _GS_H_
#define _GS_H_

#include "Types.h"
#include "Stream.h"
#include "SettingsDialogProvider.h"

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

	virtual CSettingsDialogProvider*		GetSettingsDialogProvider()				= 0;

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

	struct CLAMP
	{
		unsigned int	nWMS			: 2;
		unsigned int	nWMT			: 2;
		unsigned int	nMINU			: 10;
		unsigned int	nMAXU			: 10;
		unsigned int	nMINV			: 10;
		unsigned int	nMAXV			: 10;
		unsigned int	nReserved0		: 22;
	};

	struct FOGCOL
	{
		unsigned int	nFCR			: 8;
		unsigned int	nFCG			: 8;
		unsigned int	nFCB			: 8;
		unsigned int	nReserved0		: 8;
		unsigned int	nReserved1		: 32;
	};

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

	struct FRAME
	{
		unsigned int	nPtr			: 16;
		unsigned int	nWidth			: 8;
		unsigned int	nPsm			: 8;
		unsigned int	nMask			: 32;
		uint32			GetBasePtr()	{ return nPtr * 8192; }
		uint32			GetWidth()		{ return nWidth * 64; }
	};

	struct ZBUF
	{
		unsigned int	nPtr			: 9;
		unsigned int	nReserved0		: 15;
		unsigned int	nPsm			: 4;
		unsigned int	nReserved1		: 4;
		unsigned int	nMask			: 1;
		uint32			GetBasePtr()	{ return nPtr * 2048; }
	};

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

	struct STORAGEPSMT8
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 128 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 64 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 16 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 16 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 16 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 4 };

		static int m_nBlockSwizzleTable[4][8];
		//The swizzling actually differs depending you're on columns 0-2 or 1-3
		static int m_nColumnSwizzleTable[4][16];

		typedef uint8 Unit;
	};

	//We use a little "hack" here since we can't address stuff on 4-bits boundaries
	struct STORAGEPSMT4
	{
		enum PAGEWIDTH		{ PAGEWIDTH = 64 };
		enum PAGEHEIGHT		{ PAGEHEIGHT = 128 };
		enum BLOCKWIDTH		{ BLOCKWIDTH = 16 };
		enum BLOCKHEIGHT	{ BLOCKHEIGHT = 16 };
		enum COLUMNWIDTH	{ COLUMNWIDTH = 16 };
		enum COLUMNHEIGHT	{ COLUMNHEIGHT = 4 };

		static int m_nBlockSwizzleTable[8][4];
		static int m_nColumnSwizzleTable[4][16];

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

		typename Storage::Unit* GetPixel(unsigned int nX, unsigned int nY)
		{
			uint32 nPageNum, nBlockNum, nColumnNum, nOffset;

			nPageNum = (nX / Storage::PAGEWIDTH) + (nY / Storage::PAGEHEIGHT) * (m_nWidth * 64) / Storage::PAGEWIDTH;

			nX %= Storage::PAGEWIDTH;
			nY %= Storage::PAGEHEIGHT;

			nBlockNum = Storage::m_nBlockSwizzleTable[nY / Storage::BLOCKHEIGHT][nX / Storage::BLOCKWIDTH];

			nX %= Storage::BLOCKWIDTH;
			nY %= Storage::BLOCKHEIGHT;

			nColumnNum = (nY / Storage::COLUMNHEIGHT);

			nY %= Storage.COLUMNHEIGHT;

			nOffset = m_nPointer + (nPageNum * PAGESIZE) + (nBlockNum * BLOCKSIZE) + (nColumnNum * COLUMNSIZE);
			nOffset &= (RAMSIZE - 1);

			return &((typename Storage::Unit*)&m_pMemory[nOffset])[Storage::m_nColumnSwizzleTable[nY][nX]];
		}

	private:
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
	typedef CPixelIndexor<STORAGEPSMT8>		CPixelIndexorPSMT8;
	typedef CPixelIndexor<STORAGEPSMT4>		CPixelIndexorPSMT4;

	typedef void (CGSHandler::*TRANSFERHANDLER)(void*, uint32);

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

	void									TrxHandlerInvalid(void*, uint32);
	template <typename Storage> void		TrxHandlerCopy(void*, uint32);
	void									TrxHandlerPSMT4(void*, uint32);
	void									TrxHandlerPSMCT24(void*, uint32);
	void									TrxHandlerPSMT8H(void*, uint32);
	template <uint32, uint32> void			TrxHandlerPSMT4H(void*, uint32);

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

#endif
