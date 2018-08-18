#pragma once

#include "Types.h"
#include "../MIPS.h"
#include "../Profiler.h"
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CVif;
class CGIF;
class CINTC;

class CVpu
{
public:
	enum VU1REGISTERS
	{
		VU_TOP = 0x8400,
		VU_XGKICK = 0x8410,
		VU_ITOP = 0x8420,
		VU_CMSAR1 = 0x1000FFC0, //This is meant to be used by the EE through CTC2
	};

	struct VPUINIT
	{
		VPUINIT(uint8* microMem, uint8* vuMem, CMIPS* context)
		    : microMem(microMem)
		    , vuMem(vuMem)
		    , context(context)
		{
		}

		uint8* microMem;
		uint8* vuMem;
		CMIPS* context;
	};

	CVpu(unsigned int, const VPUINIT&, CGIF&, CINTC&, uint8*, uint8*);
	virtual ~CVpu();

	void Execute(int32);
	void Reset();
	void SaveState(Framework::CZipArchiveWriter&);
	void LoadState(Framework::CZipArchiveReader&);

	CMIPS& GetContext() const;
	uint8* GetMicroMemory() const;
	uint8* GetVuMemory() const;
	uint32 GetVuMemorySize() const;
	bool IsVuRunning() const;

	CVif& GetVif();

	void ExecuteMicroProgram(uint32);
	void InvalidateMicroProgram();

	void ProcessXgKick(uint32);

#ifdef DEBUGGER_INCLUDED
	void SaveMiniState();
	const MIPSSTATE& GetVuMiniState() const;
	uint8* GetVuMemoryMiniState() const;
	uint8* GetMicroMemoryMiniState() const;
	uint32 GetVuTopMiniState() const;
	uint32 GetVuItopMiniState() const;
#endif

protected:
	typedef std::unique_ptr<CVif> VifPtr;

	uint8* m_microMem = nullptr;
	uint8* m_vuMem = nullptr;
	uint32 m_vuMemSize = 0;
	CMIPS* m_ctx = nullptr;
	CGIF& m_gif;
	VifPtr m_vif;

#ifdef DEBUGGER_INCLUDED
	std::unique_ptr<MIPSSTATE> m_vuMiniState;
	uint8* m_microMemMiniState;
	uint8* m_vuMemMiniState;
	uint32 m_topMiniState;
	uint32 m_itopMiniState;
#endif

	unsigned int m_number = 0;
	bool m_running = false;

	CProfiler::ZoneHandle m_vuProfilerZone = 0;
};
