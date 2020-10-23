#pragma once

#include "Types.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "../gs/GSHandler.h"
#include "../Profiler.h"

class CGIF
{
public:
	enum REGISTER
	{
		GIF_STAT = 0x10003020
	};

	enum
	{
		GIF_STAT_M3P = 0x002,
		GIF_STAT_APATH3 = 0xC00,
	};

	enum
	{
		REGS_START = 0x10003000,
		REGS_END = 0x100030B0,
	};

	struct TAG
	{
		unsigned int loops : 15;
		unsigned int eop : 1;
		unsigned int reserved0 : 16;
		unsigned int reserved1 : 14;
		unsigned int pre : 1;
		unsigned int prim : 11;
		unsigned int cmd : 2;
		unsigned int nreg : 4;
		uint64 regs;
	};
	static_assert(sizeof(TAG) == 0x10, "Size of TAG must be 16 bytes.");

	CGIF(CGSHandler*&, uint8*, uint8*);
	virtual ~CGIF() = default;

	void Reset();
	uint32 ReceiveDMA(uint32, uint32, uint32, bool);

	uint32 ProcessSinglePacket(const uint8*, uint32, uint32, uint32, const CGsPacketMetadata&);
	uint32 ProcessMultiplePackets(const uint8*, uint32, uint32, uint32, const CGsPacketMetadata&);

	uint32 GetRegister(uint32);
	void SetRegister(uint32, uint32);

	CGSHandler* GetGsHandler();

	void SetPath3Masked(bool);

	void LoadState(Framework::CZipArchiveReader&);
	void SaveState(Framework::CZipArchiveWriter&);

private:
	enum SIGNAL_STATE
	{
		SIGNAL_STATE_NONE,
		SIGNAL_STATE_ENCOUNTERED,
		SIGNAL_STATE_PENDING,
	};

	uint32 ProcessPacked(const uint8*, uint32, uint32);
	uint32 ProcessRegList(const uint8*, uint32, uint32);
	uint32 ProcessImage(const uint8*, uint32, uint32, uint32);

	void DisassembleGet(uint32);
	void DisassembleSet(uint32, uint32);

	bool m_path3Masked = false;
	uint32 m_activePath = 0;

	uint16 m_loops = 0;
	uint8 m_cmd = 0;
	uint8 m_regs = 0;
	uint8 m_regsTemp = 0;
	uint64 m_regList = 0;
	bool m_eop = false;
	uint32 m_qtemp;
	SIGNAL_STATE m_signalState = SIGNAL_STATE_NONE;
	uint8* m_ram;
	uint8* m_spr;
	CGSHandler*& m_gs;

	CProfiler::ZoneHandle m_gifProfilerZone = 0;
};
