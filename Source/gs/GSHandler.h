#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <atomic>
#include <array>
#include "signal/Signal.h"

#include "bitmap/Bitmap.h"
#include "Types.h"
#include "Convertible.h"
#include "../MailBox.h"
#include "../Integer64.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CFrameDump;
class CGsPacketMetadata;
class CINTC;
struct MASSIVEWRITE_INFO;

#define PREF_CGSHANDLER_PRESENTATION_MODE "renderer.presentationmode"

enum GS_REGS
{
	GS_REG_PRIM = 0x00,
	GS_REG_RGBAQ = 0x01,
	GS_REG_ST = 0x02,
	GS_REG_UV = 0x03,
	GS_REG_XYZF2 = 0x04,
	GS_REG_XYZ2 = 0x05,
	GS_REG_TEX0_1 = 0x06,
	GS_REG_TEX0_2 = 0x07,
	GS_REG_CLAMP_1 = 0x08,
	GS_REG_CLAMP_2 = 0x09,
	GS_REG_FOG = 0x0A,
	GS_REG_XYZF3 = 0x0C,
	GS_REG_XYZ3 = 0x0D,
	GS_REG_TEX1_1 = 0x14,
	GS_REG_TEX1_2 = 0x15,
	GS_REG_TEX2_1 = 0x16,
	GS_REG_TEX2_2 = 0x17,
	GS_REG_XYOFFSET_1 = 0x18,
	GS_REG_XYOFFSET_2 = 0x19,
	GS_REG_PRMODECONT = 0x1A,
	GS_REG_PRMODE = 0x1B,
	GS_REG_TEXCLUT = 0x1C,
	GS_REG_MIPTBP1_1 = 0x34,
	GS_REG_MIPTBP1_2 = 0x35,
	GS_REG_MIPTBP2_1 = 0x36,
	GS_REG_MIPTBP2_2 = 0x37,
	GS_REG_TEXA = 0x3B,
	GS_REG_FOGCOL = 0x3D,
	GS_REG_TEXFLUSH = 0x3F,
	GS_REG_SCISSOR_1 = 0x40,
	GS_REG_SCISSOR_2 = 0x41,
	GS_REG_ALPHA_1 = 0x42,
	GS_REG_ALPHA_2 = 0x43,
	GS_REG_COLCLAMP = 0x46,
	GS_REG_TEST_1 = 0x47,
	GS_REG_TEST_2 = 0x48,
	GS_REG_PABE = 0x49,
	GS_REG_FBA_1 = 0x4A,
	GS_REG_FBA_2 = 0x4B,
	GS_REG_FRAME_1 = 0x4C,
	GS_REG_FRAME_2 = 0x4D,
	GS_REG_ZBUF_1 = 0x4E,
	GS_REG_ZBUF_2 = 0x4F,
	GS_REG_BITBLTBUF = 0x50,
	GS_REG_TRXPOS = 0x51,
	GS_REG_TRXREG = 0x52,
	GS_REG_TRXDIR = 0x53,
	GS_REG_HWREG = 0x54,
	GS_REG_SIGNAL = 0x60,
	GS_REG_FINISH = 0x61,
	GS_REG_LABEL = 0x62,
};

class CGSHandler
{
public:
	enum RAMSIZE
	{
		RAMSIZE = 0x00400000,
	};

	enum PRESENTATION_MODE
	{
		PRESENTATION_MODE_FILL,
		PRESENTATION_MODE_FIT,
		PRESENTATION_MODE_ORIGINAL
	};

	enum PRIVATE_REGISTER
	{
		GS_PMODE = 0x12000000,
		GS_SMODE2 = 0x12000020,
		GS_DISPFB1 = 0x12000070,
		GS_DISPLAY1 = 0x12000080,
		GS_DISPFB2 = 0x12000090,
		GS_DISPLAY2 = 0x120000A0,
		GS_CSR_ALT = 0x12000400, // Used by funslower demo
		GS_CSR = 0x12001000,
		GS_IMR = 0x12001010,
		GS_SIGLBLID = 0x12001080,
	};

	enum
	{
		CSR_SIGNAL_EVENT = 0x0001,
		CSR_FINISH_EVENT = 0x0002,
		CSR_HSYNC_INT = 0x0004,
		CSR_VSYNC_INT = 0x0008,
		CSR_RESET = 0x0200,
		CSR_FIELD = 0x2000,
		CSR_FIFO_STATUS = 0xC000,
		CSR_FIFO_NEITHER = 0x0000,
		CSR_FIFO_EMPTY = 0x4000,
		CSR_FIFO_FULL = 0x8000
	};

	struct PRESENTATION_PARAMS
	{
		uint32 windowWidth;
		uint32 windowHeight;
		PRESENTATION_MODE mode;
	};

	struct PRESENTATION_VIEWPORT
	{
		int32 offsetX = 0;
		int32 offsetY = 0;
		int32 width = 0;
		int32 height = 0;
	};

	enum PSM
	{
		PSMCT32 = 0x00,
		PSMCT24 = 0x01,
		PSMCT16 = 0x02,
		PSMCT24_UNK = 0x09,
		PSMCT16S = 0x0A,
		PSMT8 = 0x13,
		PSMT4 = 0x14,
		PSMT8H = 0x1B,
		PSMCT32_UNK = 0x20, //Used by movies in LotR: RotK
		PSMT4HL = 0x24,
		PSMT4HH = 0x2C,
		PSMZ32 = 0x30,
		PSMZ24 = 0x31,
		PSMZ16 = 0x32,
		PSMZ16S = 0x3A,
		PSM_MAX = 0x40,
	};

	enum PRIM_TYPE
	{
		PRIM_POINT,
		PRIM_LINE,
		PRIM_LINESTRIP,
		PRIM_TRIANGLE,
		PRIM_TRIANGLESTRIP,
		PRIM_TRIANGLEFAN,
		PRIM_SPRITE,
		PRIM_INVALID,
	};

	enum ALPHABLEND_ABD
	{
		ALPHABLEND_ABD_CS,
		ALPHABLEND_ABD_CD,
		ALPHABLEND_ABD_ZERO,
		ALPHABLEND_ABD_INVALID
	};

	enum ALPHABLEND_C
	{
		ALPHABLEND_C_AS,
		ALPHABLEND_C_AD,
		ALPHABLEND_C_FIX,
		ALPHABLEND_C_INVALID
	};

	enum DEPTH_TEST_METHOD
	{
		DEPTH_TEST_NEVER,
		DEPTH_TEST_ALWAYS,
		DEPTH_TEST_GEQUAL,
		DEPTH_TEST_GREATER
	};

	enum ALPHA_TEST_METHOD
	{
		ALPHA_TEST_NEVER,
		ALPHA_TEST_ALWAYS,
		ALPHA_TEST_LESS,
		ALPHA_TEST_LEQUAL,
		ALPHA_TEST_EQUAL,
		ALPHA_TEST_GEQUAL,
		ALPHA_TEST_GREATER,
		ALPHA_TEST_NOTEQUAL,
		ALPHA_TEST_MAX
	};

	enum ALPHA_TEST_FAIL_METHOD
	{
		ALPHA_TEST_FAIL_KEEP,
		ALPHA_TEST_FAIL_FBONLY,
		ALPHA_TEST_FAIL_ZBONLY,
		ALPHA_TEST_FAIL_RGBONLY
	};

	enum TEX0_FUNCTION
	{
		TEX0_FUNCTION_MODULATE,
		TEX0_FUNCTION_DECAL,
		TEX0_FUNCTION_HIGHLIGHT,
		TEX0_FUNCTION_HIGHLIGHT2,
		TEX0_FUNCTION_MAX
	};

	enum CLAMP_MODE
	{
		CLAMP_MODE_REPEAT,
		CLAMP_MODE_CLAMP,
		CLAMP_MODE_REGION_CLAMP,
		CLAMP_MODE_REGION_REPEAT,
		CLAMP_MODE_MAX
	};

	enum REGISTER_MAX
	{
		REGISTER_MAX = 0x80
	};

	//Reg 0x00
	struct PRIM : public convertible<uint64>
	{
		unsigned int nType : 3;
		unsigned int nShading : 1;
		unsigned int nTexture : 1;
		unsigned int nFog : 1;
		unsigned int nAlpha : 1;
		unsigned int nAntiAliasing : 1;
		unsigned int nUseUV : 1;
		unsigned int nContext : 1;
		unsigned int nUseFloat : 1;
		unsigned int nReserved0 : 21;
		uint32 nReserved1;
	};
	static_assert(sizeof(PRIM) == sizeof(uint64), "Size of PRIM struct must be 8 bytes.");

	//Reg 0x01
	struct RGBAQ : public convertible<uint64>
	{
		uint8 nR;
		uint8 nG;
		uint8 nB;
		uint8 nA;
		float nQ;
	};
	static_assert(sizeof(RGBAQ) == sizeof(uint64), "Size of RGBAQ struct must be 8 bytes.");

	//Reg 0x02
	struct ST : public convertible<uint64>
	{
		float nS;
		float nT;
	};
	static_assert(sizeof(ST) == sizeof(uint64), "Size of ST struct must be 8 bytes.");

	//Reg 0x03
	struct UV : public convertible<uint64>
	{
		uint16 nU;
		uint16 nV;
		uint32 nReserved;

		float GetU() const
		{
			return static_cast<float>(nU & 0x3FFF) / 16.0f;
		}
		float GetV() const
		{
			return static_cast<float>(nV & 0x3FFF) / 16.0f;
		}
	};
	static_assert(sizeof(UV) == sizeof(uint64), "Size of UV struct must be 8 bytes.");

	//Reg 0x04/0x0C
	struct XYZF : public convertible<uint64>
	{
		unsigned int nX : 16;
		unsigned int nY : 16;
		unsigned int nZ : 24;
		unsigned int nF : 8;

		float GetX()
		{
			return static_cast<float>(nX) / 16.0f;
		}
		float GetY()
		{
			return static_cast<float>(nY) / 16.0f;
		}
	};
	static_assert(sizeof(XYZF) == sizeof(uint64), "Size of XYZF struct must be 8 bytes.");

	//Reg 0x05/0x0D
	struct XYZ : public convertible<uint64>
	{
		unsigned int nX : 16;
		unsigned int nY : 16;
		uint32 nZ;

		float GetX()
		{
			return static_cast<float>(nX) / 16.0f;
		}
		float GetY()
		{
			return static_cast<float>(nY) / 16.0f;
		}
		float GetZ()
		{
			return static_cast<float>(nZ);
		}
	};
	static_assert(sizeof(XYZ) == sizeof(uint64), "Size of XYZ struct must be 8 bytes.");

	//Reg 0x06/0x07
	struct TEX0 : public convertible<uint64>
	{
		unsigned int nBufPtr : 14;
		unsigned int nBufWidth : 6;
		unsigned int nPsm : 6;
		unsigned int nWidth : 4;
		unsigned int nPad0 : 2;
		unsigned int nPad1 : 2;
		unsigned int nColorComp : 1;
		unsigned int nFunction : 2;
		unsigned int nCBP : 14;
		unsigned int nCPSM : 4;
		unsigned int nCSM : 1;
		unsigned int nCSA : 5;
		unsigned int nCLD : 3;
		uint32 GetBufPtr() const
		{
			return nBufPtr * 256;
		}
		uint32 GetBufWidth() const
		{
			return nBufWidth * 64;
		}
		uint32 GetWidth() const
		{
			return (1 << nWidth);
		}
		uint32 GetHeight() const
		{
			return (1 << (nPad0 | (nPad1 << 2)));
		}
		uint32 GetCLUTPtr() const
		{
			return nCBP * 256;
		}
	};
	static_assert(sizeof(TEX0) == sizeof(uint64), "Size of TEX0 struct must be 8 bytes.");

	//Reg 0x08/0x09
	struct CLAMP : public convertible<uint64>
	{
		unsigned int nWMS : 2;
		unsigned int nWMT : 2;
		unsigned int nMINU : 10;
		unsigned int nMAXU : 10;
		unsigned int nReserved0 : 8;
		unsigned int nReserved1 : 2;
		unsigned int nMAXV : 10;
		unsigned int nReserved2 : 20;
		unsigned int GetMinU()
		{
			return nMINU;
		}
		unsigned int GetMaxU()
		{
			return nMAXU;
		}
		unsigned int GetMinV()
		{
			return (nReserved0) | (nReserved1 << 8);
		}
		unsigned int GetMaxV()
		{
			return nMAXV;
		}
	};
	static_assert(sizeof(CLAMP) == sizeof(uint64), "Size of CLAMP struct must be 8 bytes.");

	//Reg 0x14/0x15
	struct TEX1 : public convertible<uint64>
	{
		unsigned int nLODMethod : 1;
		unsigned int nReserved0 : 1;
		unsigned int nMaxMip : 3;
		unsigned int nMagFilter : 1;
		unsigned int nMinFilter : 3;
		unsigned int nMipBaseAddr : 1;
		unsigned int nReserved1 : 9;
		unsigned int nLODL : 2;
		unsigned int nReserved2 : 11;
		unsigned int nLODK : 12;
		unsigned int nReserved3 : 20;

		float GetK() const
		{
			int16 temp = nLODK | ((nLODK & 0x800) ? 0xF000 : 0x0000);
			return static_cast<float>(temp) / 16.0f;
		}
	};
	static_assert(sizeof(TEX1) == sizeof(uint64), "Size of TEX1 struct must be 8 bytes.");

	//Reg 0x16/0x17
	struct TEX2 : public convertible<uint64>
	{
		unsigned int nReserved0 : 20;
		unsigned int nPsm : 6;
		unsigned int nReserved1 : 6;
		unsigned int nReserved2 : 5;
		unsigned int nCBP : 14;
		unsigned int nCPSM : 4;
		unsigned int nCSM : 1;
		unsigned int nCSA : 5;
		unsigned int nCLD : 3;
		uint32 GetCLUTPtr()
		{
			return nCBP * 256;
		}
	};
	static_assert(sizeof(TEX2) == sizeof(uint64), "Size of TEX2 struct must be 8 bytes.");

	//Reg 0x18/0x19
	struct XYOFFSET : public convertible<uint64>
	{
		uint16 nOffsetX;
		uint16 nReserved0;
		uint16 nOffsetY;
		uint16 nReserved1;
		float GetX()
		{
			return static_cast<float>(nOffsetX) / 16.0f;
		}
		float GetY()
		{
			return static_cast<float>(nOffsetY) / 16.0f;
		}
	};
	static_assert(sizeof(XYOFFSET) == sizeof(uint64), "Size of XYOFFSET struct must be 8 bytes.");

	//Reg 0x1B
	struct PRMODE : public convertible<uint64>
	{
		unsigned int nReserved0 : 3;
		unsigned int nShading : 1;
		unsigned int nTexture : 1;
		unsigned int nFog : 1;
		unsigned int nAlpha : 1;
		unsigned int nAntiAliasing : 1;
		unsigned int nUseUV : 1;
		unsigned int nContext : 1;
		unsigned int nUseFloat : 1;
		unsigned int nReserved1 : 21;
		uint32 nReserved2;
	};
	static_assert(sizeof(PRMODE) == sizeof(uint64), "Size of PRMODE struct must be 8 bytes.");

	//Reg 0x34/0x35
	struct MIPTBP1 : public convertible<uint64>
	{
		unsigned int tbp1 : 14;
		unsigned int tbw1 : 6;
		unsigned int pad0 : 12;
		unsigned int pad1 : 2;
		unsigned int tbw2 : 6;
		unsigned int tbp3 : 14;
		unsigned int tbw3 : 6;
		unsigned int reserved : 4;
		uint32 GetTbp1() const
		{
			return tbp1 * 256;
		}
		uint32 GetTbp2() const
		{
			return (pad0 | (pad1 << 12)) * 256;
		}
		uint32 GetTbp3() const
		{
			return tbp3 * 256;
		}
		uint32 GetTbw1() const
		{
			return tbw1 * 64;
		}
		uint32 GetTbw2() const
		{
			return tbw2 * 64;
		}
		uint32 GetTbw3() const
		{
			return tbw3 * 64;
		}
	};
	static_assert(sizeof(MIPTBP1) == sizeof(uint64), "Size of MIPTBP1 struct must be 8 bytes.");

	//Reg 0x36/0x37
	struct MIPTBP2 : public convertible<uint64>
	{
		unsigned int tbp4 : 14;
		unsigned int tbw4 : 6;
		unsigned int pad0 : 12;
		unsigned int pad1 : 2;
		unsigned int tbw5 : 6;
		unsigned int tbp6 : 14;
		unsigned int tbw6 : 6;
		unsigned int reserved : 4;
		uint32 GetTbp4() const
		{
			return tbp4 * 256;
		}
		uint32 GetTbp5() const
		{
			return (pad0 | (pad1 << 12)) * 256;
		}
		uint32 GetTbp6() const
		{
			return tbp6 * 256;
		}
		uint32 GetTbw4() const
		{
			return tbw4 * 64;
		}
		uint32 GetTbw5() const
		{
			return tbw5 * 64;
		}
		uint32 GetTbw6() const
		{
			return tbw6 * 64;
		}
	};
	static_assert(sizeof(MIPTBP2) == sizeof(uint64), "Size of MIPTBP2 struct must be 8 bytes.");

	//Reg 0x3B
	struct TEXA : public convertible<uint64>
	{
		unsigned int nTA0 : 8;
		unsigned int nReserved0 : 7;
		unsigned int nAEM : 1;
		unsigned int nReserved1 : 16;
		unsigned int nTA1 : 8;
		unsigned int nReserved2 : 24;
	};
	static_assert(sizeof(TEXA) == sizeof(uint64), "Size of TEXA struct must be 8 bytes.");

	//Reg 0x3D
	struct FOGCOL : public convertible<uint64>
	{
		unsigned int nFCR : 8;
		unsigned int nFCG : 8;
		unsigned int nFCB : 8;
		unsigned int nReserved0 : 8;
		unsigned int nReserved1 : 32;
	};
	static_assert(sizeof(FOGCOL) == sizeof(uint64), "Size of FOGCOL struct must be 8 bytes.");

	//Reg 0x3F
	struct TEXCLUT : public convertible<uint64>
	{
		unsigned int nCBW : 6;
		unsigned int nCOU : 6;
		unsigned int nCOV : 10;
		unsigned int nReserved1 : 10;
		unsigned int nReserved2 : 32;
		uint32 GetBufWidth() const
		{
			return nCBW * 64;
		}
		uint32 GetOffsetU() const
		{
			return nCOU * 16;
		}
		uint32 GetOffsetV() const
		{
			return nCOV;
		}
	};
	static_assert(sizeof(TEXCLUT) == sizeof(uint64), "Size of TEXCLUT struct must be 8 bytes.");

	//Reg 0x40/0x41
	struct SCISSOR : public convertible<uint64>
	{
		unsigned int scax0 : 11;
		unsigned int reserved0 : 5;
		unsigned int scax1 : 11;
		unsigned int reserved1 : 5;
		unsigned int scay0 : 11;
		unsigned int reserved2 : 5;
		unsigned int scay1 : 11;
		unsigned int reserved3 : 5;
	};
	static_assert(sizeof(SCISSOR) == sizeof(uint64), "Size of SCISSOR struct must be 8 bytes.");

	//Reg 0x42/0x43
	struct ALPHA : public convertible<uint64>
	{
		unsigned int nA : 2;
		unsigned int nB : 2;
		unsigned int nC : 2;
		unsigned int nD : 2;
		unsigned int nReserved0 : 24;
		unsigned int nFix : 8;
		unsigned int nReserved1 : 24;
	};
	static_assert(sizeof(ALPHA) == sizeof(uint64), "Size of ALPHA struct must be 8 bytes.");

	//Reg 0x47/0x48
	struct TEST : public convertible<uint64>
	{
		unsigned int nAlphaEnabled : 1;
		unsigned int nAlphaMethod : 3;
		unsigned int nAlphaRef : 8;
		unsigned int nAlphaFail : 2;
		unsigned int nDestAlphaEnabled : 1;
		unsigned int nDestAlphaMode : 1;
		unsigned int nDepthEnabled : 1;
		unsigned int nDepthMethod : 2;
		unsigned int nReserved0 : 13;
		uint32 nReserved1;
	};
	static_assert(sizeof(TEST) == sizeof(uint64), "Size of TEST struct must be 8 bytes.");

	//Reg 0x4C/0x4D
	struct FRAME : public convertible<uint64>
	{
		unsigned int nPtr : 9;
		unsigned int nReserved0 : 7;
		unsigned int nWidth : 6;
		unsigned int nReserved1 : 2;
		unsigned int nPsm : 6;
		unsigned int nReserved2 : 2;
		unsigned int nMask : 32;
		uint32 GetBasePtr() const
		{
			return nPtr * 8192;
		}
		uint32 GetWidth() const
		{
			return nWidth * 64;
		}
	};
	static_assert(sizeof(FRAME) == sizeof(uint64), "Size of FRAME struct must be 8 bytes.");

	//Reg 0x4E/0x4F
	struct ZBUF : public convertible<uint64>
	{
		unsigned int nPtr : 9;
		unsigned int nReserved0 : 15;
		unsigned int nPsm : 4;
		unsigned int nReserved1 : 4;
		unsigned int nMask : 1;
		unsigned int nReserved2 : 31;
		uint32 GetBasePtr() const
		{
			return nPtr * 8192;
		}
	};
	static_assert(sizeof(ZBUF) == sizeof(uint64), "Size of ZBUF struct must be 8 bytes.");

	//Reg 0x50
	struct BITBLTBUF : public convertible<uint64>
	{
		unsigned int nSrcPtr : 14;
		unsigned int nReserved0 : 2;
		unsigned int nSrcWidth : 6;
		unsigned int nReserved1 : 2;
		unsigned int nSrcPsm : 6;
		unsigned int nReserved2 : 2;
		unsigned int nDstPtr : 14;
		unsigned int nReserved3 : 2;
		unsigned int nDstWidth : 6;
		unsigned int nReserved4 : 2;
		unsigned int nDstPsm : 6;
		unsigned int nReserved5 : 2;
		uint32 GetSrcPtr() const
		{
			return nSrcPtr * 256;
		}
		uint32 GetSrcWidth() const
		{
			return nSrcWidth * 64;
		}
		uint32 GetDstPtr() const
		{
			return nDstPtr * 256;
		}
		uint32 GetDstWidth() const
		{
			return nDstWidth * 64;
		}
	};
	static_assert(sizeof(BITBLTBUF) == sizeof(uint64), "Size of BITBLTBUF struct must be 8 bytes.");

	//Reg 0x51
	struct TRXPOS : public convertible<uint64>
	{
		unsigned int nSSAX : 11;
		unsigned int nReserved0 : 5;
		unsigned int nSSAY : 11;
		unsigned int nReserved1 : 5;
		unsigned int nDSAX : 11;
		unsigned int nReserved2 : 5;
		unsigned int nDSAY : 11;
		unsigned int nDIR : 2;
		unsigned int nReserved3 : 3;
	};
	static_assert(sizeof(TRXPOS) == sizeof(uint64), "Size of TRXPOS struct must be 8 bytes.");

	//Reg 0x52
	struct TRXREG : public convertible<uint64>
	{
		unsigned int nRRW : 12;
		unsigned int nReserved0 : 20;
		unsigned int nRRH : 12;
		unsigned int nReserved1 : 20;
	};
	static_assert(sizeof(TRXREG) == sizeof(uint64), "Size of TRXREG struct must be 8 bytes.");

	//Reg 0x60
	struct SIGNAL : public convertible<uint64>
	{
		unsigned int id : 32;
		unsigned int idmsk : 32;
	};
	static_assert(sizeof(SIGNAL) == sizeof(uint64), "Size of SIGNAL struct must be 8 bytes.");

	//Reg 0x62
	struct LABEL : public convertible<uint64>
	{
		unsigned int id : 32;
		unsigned int idmsk : 32;
	};
	static_assert(sizeof(LABEL) == sizeof(uint64), "Size of LABEL struct must be 8 bytes.");

	typedef std::pair<uint8, uint64> RegisterWrite;
	typedef std::vector<RegisterWrite> RegisterWriteList;
	typedef std::function<CGSHandler*(void)> FactoryFunction;

	typedef Framework::CSignal<void()> FlipCompleteEvent;
	typedef Framework::CSignal<void(uint32)> NewFrameEvent;

	CGSHandler(bool = true);
	virtual ~CGSHandler();

	static void RegisterPreferences();
	void NotifyPreferencesChanged();

	void SetIntc(CINTC*);
	void Reset();
	virtual void SetPresentationParams(const PRESENTATION_PARAMS&);

	virtual void SaveState(Framework::CZipArchiveWriter&);
	virtual void LoadState(Framework::CZipArchiveReader&);
	void Copy(const CGSHandler*);

	void SetFrameDump(CFrameDump*);

	bool GetDrawEnabled() const;
	void SetDrawEnabled(bool);

	void WritePrivRegister(uint32, uint32);
	uint32 ReadPrivRegister(uint32);

	void SetLoggingEnabled(bool);
	static std::string DisassembleWrite(uint8, uint64);

	void SetVBlank();
	void ResetVBlank();

	void WriteRegister(uint8, uint64);
	void FeedImageData(const void*, uint32);
	void ReadImageData(void*, uint32);
	void WriteRegisterMassively(uint32, uint32, const CGsPacketMetadata*);

	inline void AddWriteToBuffer(const RegisterWrite& write)
	{
		assert(m_writeBufferSize < REGISTERWRITEBUFFER_SIZE);
		if(m_writeBufferSize == REGISTERWRITEBUFFER_SIZE) return;
		m_writeBuffer[m_writeBufferSize++] = write;
	}

	void ProcessWriteBuffer();
	void SubmitWriteBuffer();
	void FlushWriteBuffer();
	
	virtual void SetCrt(bool, unsigned int, bool);
	void Initialize();
	void Release();
	virtual void ProcessHostToLocalTransfer() = 0;
	virtual void ProcessLocalToHostTransfer() = 0;
	virtual void ProcessLocalToLocalTransfer() = 0;
	virtual void ProcessClutTransfer(uint32, uint32) = 0;
	void Flip(bool = false);
	void Finish();
	virtual void ReadFramebuffer(uint32, uint32, void*) = 0;

	void MakeLinearCLUT(const TEX0&, std::array<uint32, 256>&) const;

	virtual uint8* GetRam() const;
	uint64* GetRegisters();

	uint64 GetSMODE2() const;
	void SetSMODE2(uint64);

	int GetPendingTransferCount() const;
	void NotifyEvent(uint32);

	unsigned int GetCrtWidth() const;
	unsigned int GetCrtHeight() const;
	bool GetCrtIsInterlaced() const;
	bool GetCrtIsFrameMode() const;
	std::pair<uint64, uint64> GetCurrentDisplayInfo();
	unsigned int GetCurrentReadCircuit();

	static std::pair<uint32, uint32> GetTransferInvalidationRange(const BITBLTBUF&, const TRXREG&, const TRXPOS&);

	virtual Framework::CBitmap GetScreenshot();
	void ProcessSingleFrame();

	FlipCompleteEvent OnFlipComplete;
	NewFrameEvent OnNewFrame;

protected:
	struct DELAYED_REGISTER
	{
		uint32 heldValue;
		INTEGER64 value;
	};

	enum CLUTSIZE
	{
		CLUTSIZE = 0x400,
		CLUTENTRYCOUNT = (CLUTSIZE / 2)
	};

	enum
	{
		REGISTERWRITEBUFFER_SIZE = 0x100000,
		REGISTERWRITEBUFFER_SUBMIT_THRESHOLD = 0x100
	};

	enum MAG_FILTER
	{
		MAG_FILTER_NEAREST = 0,
		MAG_FILTER_LINEAR = 1
	};

	enum MIN_FILTER
	{
		MIN_FILTER_NEAREST = 0,
		MIN_FILTER_LINEAR = 1,
		MIN_FILTER_NEAREST_MIP_NEAREST = 2,
		MIN_FILTER_NEAREST_MIP_LINEAR = 3,
		MIN_FILTER_LINEAR_MIP_NEAREST = 4,
		MIN_FILTER_LINEAR_MIP_LINEAR = 5
	};

	//-----------------------------------
	//Private Registers

	struct SMODE2 : public convertible<uint64>
	{
		unsigned int interlaced : 1;
		unsigned int ffmd : 1;
		unsigned int dpms : 2;
		unsigned int reserved0 : 28;
		unsigned int reserved1;
	};
	static_assert(sizeof(SMODE2) == sizeof(uint64), "Size of SMODE2 struct must be 8 bytes.");

	struct DISPFB : public convertible<uint64>
	{
		unsigned int nBufPtr : 9;
		unsigned int nBufWidth : 6;
		unsigned int nPSM : 5;
		unsigned int nReserved0 : 12;
		unsigned int nX : 11;
		unsigned int nY : 11;
		unsigned int nReserved1 : 10;
		uint32 GetBufPtr() const
		{
			return nBufPtr * 8192;
		};
		uint32 GetBufWidth() const
		{
			return nBufWidth * 64;
		};
	};
	static_assert(sizeof(DISPFB) == sizeof(uint64), "Size of DISPFB struct must be 8 bytes.");

	struct DISPLAY : public convertible<uint64>
	{
		unsigned int nX : 12;
		unsigned int nY : 11;
		unsigned int nMagX : 4;
		unsigned int nMagY : 5;
		unsigned int nW : 12;
		unsigned int nH : 12;
		unsigned int reserved : 8;
	};
	static_assert(sizeof(DISPLAY) == sizeof(uint64), "Size of DISPLAY struct must be 8 bytes.");

	struct SIGLBLID : public convertible<uint64>
	{
		unsigned int sigid : 32;
		unsigned int lblid : 32;
	};
	static_assert(sizeof(SIGLBLID) == sizeof(uint64), "Size of SIGLBLID struct must be 8 bytes.");

	struct TRXCONTEXT
	{
		uint32 nSize;
		uint32 nRealSize;
		uint32 nRRX;
		uint32 nRRY;
		bool nDirty;
	};

	typedef bool (CGSHandler::*TRANSFERWRITEHANDLER)(const void*, uint32);
	typedef void (CGSHandler::*TRANSFERREADHANDLER)(void*, uint32);

	void LogWrite(uint8, uint64);
	void LogPrivateWrite(uint32);

	void WriteToDelayedRegister(uint32, uint32, DELAYED_REGISTER&);

	void ThreadProc();
	virtual void InitializeImpl() = 0;
	virtual void ReleaseImpl() = 0;
	void ResetBase();
	virtual void ResetImpl();
	virtual void NotifyPreferencesChangedImpl();
	virtual void FlipImpl();
	virtual void MarkNewFrame();
	virtual void WriteRegisterImpl(uint8, uint64);
	void FeedImageDataImpl(const uint8*, uint32);
	void ReadImageDataImpl(void*, uint32);
	void WriteRegisterMassivelyImpl(const MASSIVEWRITE_INFO&);

	void BeginTransfer();

	virtual void BeginTransferWrite();
	virtual void TransferWrite(const uint8*, uint32);

	TRANSFERWRITEHANDLER m_transferWriteHandlers[PSM_MAX];
	TRANSFERREADHANDLER m_transferReadHandlers[PSM_MAX];

	bool TransferWriteHandlerInvalid(const void*, uint32);
	template <typename Storage>
	bool TransferWriteHandlerGeneric(const void*, uint32);
	bool TransferWriteHandlerPSMT4(const void*, uint32);
	bool TransferWriteHandlerPSMCT24(const void*, uint32);
	bool TransferWriteHandlerPSMT8H(const void*, uint32);
	template <uint32, uint32>
	bool TransferWriteHandlerPSMT4H(const void*, uint32);

	void TransferReadHandlerInvalid(void*, uint32);
	template <typename Storage>
	void TransferReadHandlerGeneric(void*, uint32);
	void TransferReadHandlerPSMCT24(void*, uint32);

	virtual void SyncCLUT(const TEX0&);
	bool ProcessCLD(const TEX0&);
	template <typename Indexor>
	bool ReadCLUT4_16(const TEX0&);
	template <typename Indexor>
	bool ReadCLUT8_16(const TEX0&);
	void ReadCLUT4(const TEX0&);
	void ReadCLUT8(const TEX0&);

	static bool IsCompatibleFramebufferPSM(unsigned int, unsigned int);

	void SendGSCall(const CMailBox::FunctionType&, bool = false, bool = false);
	void SendGSCall(CMailBox::FunctionType&&);

	PRESENTATION_VIEWPORT GetPresentationViewport() const;

	bool m_loggingEnabled;

	uint64 m_nPMODE;              //0x12000000
	uint64 m_nSMODE2;             //0x12000020
	DELAYED_REGISTER m_nDISPFB1;  //0x12000070
	DELAYED_REGISTER m_nDISPLAY1; //0x12000080
	DELAYED_REGISTER m_nDISPFB2;  //0x12000090
	DELAYED_REGISTER m_nDISPLAY2; //0x120000A0
	uint64 m_nCSR;                //0x12001000
	uint64 m_nIMR;                //0x12001010
	uint64 m_nSIGLBLID;           //0x12001080

	PRESENTATION_PARAMS m_presentationParams;

	TRXCONTEXT m_trxCtx;

	uint64 m_nReg[REGISTER_MAX];

	uint8* m_pRAM;

	uint16* m_pCLUT;
	uint32 m_nCBP0;
	uint32 m_nCBP1;

	uint32 m_drawCallCount;

	//Rename to register write buffer?
	RegisterWrite* m_writeBuffer;
	uint32 m_writeBufferSize = 0;
	uint32 m_writeBufferProcessIndex = 0;
	uint32 m_writeBufferSubmitIndex = 0;

	unsigned int m_nCrtMode;
	std::thread m_thread;
	std::recursive_mutex m_registerMutex;
	std::atomic<int> m_transferCount;
	bool m_threadDone;
	CFrameDump* m_frameDump;
	bool m_drawEnabled = true;
	CINTC* m_intc = nullptr;
	bool m_gsThreaded = true;
	bool m_flipped = false;

private:
	CMailBox m_mailBox;
};
