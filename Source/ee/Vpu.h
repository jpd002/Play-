#pragma once

#include "Types.h"
#include "../MIPS.h"
#include "../Profiler.h"
#include "VuExecutor.h"
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CVif;
class CGIF;

class CVpu
{
public:
	enum VU1REGISTERS
	{
		VU_TOP		= 0x8400,
		VU_XGKICK	= 0x8410,
		VU_ITOP		= 0x8420,
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

							CVpu(unsigned int, const VPUINIT&, CGIF&, uint8*, uint8*);
	virtual					~CVpu();

	virtual void			Execute(bool);
	void					Reset();
	void					SaveState(Framework::CZipArchiveWriter&);
	void					LoadState(Framework::CZipArchiveReader&);

	CMIPS&					GetContext() const;
	uint8*					GetMicroMemory() const;
	uint8*					GetVuMemory() const;
	bool					IsVuRunning() const;

	CVif&					GetVif();

	void					ExecuteMicroProgram(uint32);
	void					InvalidateMicroProgram();

	void					ProcessXgKick(uint32);

#ifdef DEBUGGER_INCLUDED
	bool					MustBreak() const;
	void					DisableBreakpointsOnce();

	void					SaveMiniState();
	const MIPSSTATE&		GetVuMiniState() const;
	uint8*					GetVuMemoryMiniState() const;
	uint8*					GetMicroMemoryMiniState() const;
	uint32					GetVuTopMiniState() const;
	uint32					GetVuItopMiniState() const;
#endif

protected:
	typedef std::unique_ptr<CVif> VifPtr;

	uint8*					m_microMem = nullptr;
	uint8*					m_vuMem = nullptr;
	CMIPS*					m_ctx = nullptr;
	CGIF&					m_gif;
	VifPtr					m_vif;

#ifdef DEBUGGER_INCLUDED
	MIPSSTATE				m_vuMiniState;
	uint8*					m_microMemMiniState;
	uint8*					m_vuMemMiniState;
	uint32					m_topMiniState;
	uint32					m_itopMiniState;
#endif

	unsigned int			m_number = 0;
	CVuExecutor				m_executor;
	bool					m_running = false;

	CProfiler::ZoneHandle	m_vuProfilerZone = 0;
};
