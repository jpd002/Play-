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
	enum
	{
		VU_ADDR_VU1AREA_START = 0x4000, //Only on VU0
		VU_ADDR_TOP = 0x8400,           //Only on VU1
		VU_ADDR_XGKICK = 0x8410,        //Only on VU1
		VU_ADDR_ITOP = 0x8420,

		EE_ADDR_VU1AREA_START = 0x1000FB00,
		EE_ADDR_VU1AREA_END = 0x1000FEFF,
		EE_ADDR_VU_CMSAR1 = 0x1000FFC0, //This is meant to be used by the EE through CTC2
	};

	enum VU_STATE
	{
		VU_STATE_READY,
		VU_STATE_RUNNING,
		VU_STATE_STOPPED,
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

	typedef Framework::CSignal<void(VU_STATE)> VuStateChangedEvent;
	typedef Framework::CSignal<void()> VuInterruptTriggeredEvent;

	CVpu(unsigned int, const VPUINIT&, CGIF&, CINTC&, uint8*, uint8*);
	virtual ~CVpu();

	void Execute(int32);
	void Reset();
	void SaveState(Framework::CZipArchiveWriter&);
	void LoadState(Framework::CZipArchiveReader&);

	CMIPS& GetContext() const;
	uint8* GetMicroMemory() const;
	uint32 GetMicroMemorySize() const;
	uint8* GetVuMemory() const;
	uint32 GetVuMemorySize() const;
	
	inline VU_STATE GetVuState() const
	{
		return m_vuState;
	}

	inline bool IsVuReady() const
	{
		return m_vuState == VU_STATE_READY;
	}

	inline bool IsVuRunning() const
	{
		return m_vuState == VU_STATE_RUNNING;
	}

	CVif& GetVif();

	void ExecuteMicroProgram(uint32);
	void InvalidateMicroProgram();
	void InvalidateMicroProgram(uint32, uint32);

	void ProcessXgKick(uint32);

#ifdef DEBUGGER_INCLUDED
	void SaveMiniState();
	const MIPSSTATE& GetVuMiniState() const;
	uint8* GetVuMemoryMiniState() const;
	uint8* GetMicroMemoryMiniState() const;
	uint32 GetVuTopMiniState() const;
	uint32 GetVuItopMiniState() const;
#endif

	VuStateChangedEvent VuStateChanged;
	VuInterruptTriggeredEvent VuInterruptTriggered;

protected:
	typedef std::unique_ptr<CVif> VifPtr;

	unsigned int m_number = 0;
	VifPtr m_vif;
	uint8* m_microMem = nullptr;
	uint32 m_microMemSize = 0;
	uint8* m_vuMem = nullptr;
	uint32 m_vuMemSize = 0;
	CMIPS* m_ctx = nullptr;
	CGIF& m_gif;

#ifdef DEBUGGER_INCLUDED
	MIPSSTATE m_vuMiniState;
	uint8* m_microMemMiniState;
	uint8* m_vuMemMiniState;
	uint32 m_topMiniState;
	uint32 m_itopMiniState;
#endif

	VU_STATE m_vuState = VU_STATE_READY;

	CProfiler::ZoneHandle m_vuProfilerZone = 0;
};
