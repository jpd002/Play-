#pragma once

#include "Types.h"
#include "MemoryMap.h"
#include "MipsExecutor.h"
#include "MIPSArchitecture.h"
#include "MIPSCoprocessor.h"
#include "MIPSAnalysis.h"
#include "MIPSTags.h"
#include "uint128.h"
#include <set>

struct REGISTER_PIPELINE
{
	uint32 counter;
	uint32 heldValue;
};

enum
{
	//Must be a power of 2
	FLAG_PIPELINE_SLOTS = 8,
};

//Invariants:
//- Pipe times are sorted when iterating from index to index + PIPELINE_SLOTS
//- The value at index - 1 is the latest value
struct FLAG_PIPELINE
{
	uint32 index;
	uint32 values[FLAG_PIPELINE_SLOTS];
	uint32 pipeTimes[FLAG_PIPELINE_SLOTS];
};

enum
{
	MIPS_EXCEPTION_NONE = 0,
	MIPS_EXCEPTION_SYSCALL,
	MIPS_EXCEPTION_CHECKPENDINGINT,
	MIPS_EXCEPTION_IDLE,
	MIPS_EXCEPTION_RETURNFROMEXCEPTION,
	MIPS_EXCEPTION_BREAKPOINT,
	MIPS_EXCEPTION_TLB,
	MIPS_EXCEPTION_VU_CALLMS,
	MIPS_EXCEPTION_VU_DBIT,
	MIPS_EXCEPTION_VU_TBIT,
	MIPS_EXCEPTION_VU_EBIT,
	MIPS_EXCEPTION_MAX, //Used to make sure we don't conflict with the QUOTADONE status
};
static_assert(MIPS_EXCEPTION_MAX <= 0x80);

static constexpr uint32 MIPS_EXCEPTION_STATUS_QUOTADONE = 0x80;

struct TLBENTRY
{
	uint32 entryLo0;
	uint32 entryLo1;
	uint32 entryHi;
	uint32 pageMask;
};

struct MIPSSTATE
{
	uint32 nPC;
	uint32 nDelayedJumpAddr;
	uint32 nHasException;
	int32 cycleQuota;

	alignas(16) uint128 nGPR[32];

	uint32 nHI[2];
	uint32 nLO[2];
	uint32 nHI1[2];
	uint32 nLO1[2];
	uint32 nSA;

	//COP0
	uint32 nCOP0[32];

	uint32 cop0_pccr;
	uint32 cop0_pcr[2];

	//COP1
	uint32 nCOP1[32];
	uint32 nCOP1A;
	uint32 nFCSR;

	//COP2
	alignas(16) uint128 nCOP2[33];

	uint128 nCOP2A;

	uint128 nCOP2VF_PreUp;
	uint128 nCOP2VF_UpRes;

	uint32 nCOP2Q;
	uint32 nCOP2I;
	uint32 nCOP2P;
	uint32 nCOP2R;
	uint32 nCOP2CF; //Mirror of CLIP flag (computed with values from pipeClip)
	uint32 nCOP2MF; //Mirror of MACflag (computed with values from pipeMac)
	uint32 nCOP2SF; //Sticky values of sign and zero MACflag (ie.: SxSySzSw ZxZyZzZw)
	uint32 nCOP2DF; //Division by 0 flag from DIV operations
	uint32 nCOP2T;

	uint32 nCOP2VI[16];

	REGISTER_PIPELINE pipeQ;
	REGISTER_PIPELINE pipeP;
	FLAG_PIPELINE pipeMac;
	FLAG_PIPELINE pipeSticky;
	FLAG_PIPELINE pipeClip;

	alignas(16) uint128 pipeFmacWrite[3]; //Pending FMAC write operations (for dynamic stall computation)

	uint32 pipeTime;

	uint32 cmsar0;
	uint32 callMsEnabled;
	uint32 callMsAddr;

	uint32 savedIntReg;
	uint32 savedIntRegTemp;

	// In the case that the final delay slot includes an integer altering instruction
	// save the register number here along with the value it has assuming the first
	// instruction of the next block is a conditional branch on the same register.
	uint32 savedNextBlockIntRegIdx;
	uint32 savedNextBlockIntRegVal;
	uint32 savedNextBlockIntRegValTemp;

	uint32 xgkickAddress;

	enum
	{
		TLB_ENTRY_MAX = 48,
	};

	TLBENTRY tlbEntries[TLB_ENTRY_MAX];
};

#define MIPS_INVALID_PC (0x00000001)
#define MIPS_PAGE_SIZE (0x1000)

class CMIPS
{
public:
	typedef uint32 (*AddressTranslator)(CMIPS*, uint32);
	typedef uint32 (*TLBExceptionChecker)(CMIPS*, uint32, uint32);

	typedef std::set<uint32> BreakpointSet;

	CMIPS(MEMORYMAP_ENDIANNESS, bool usePageTable = false);
	~CMIPS();
	void ToggleBreakpoint(uint32);
	bool HasBreakpointInRange(uint32, uint32) const;
	bool IsBranch(uint32);
	static int32 GetBranch(uint16);
	static uint32 TranslateAddress64(CMIPS*, uint32);

	void Reset();

	bool CanGenerateInterrupt() const;
	bool GenerateInterrupt(uint32);
	bool GenerateException(uint32);

	void MapPages(uint32, uint32, uint8*);

	MIPSSTATE m_State;

	void* m_vuMem = nullptr;
	void** m_pageLookup = nullptr;

	std::function<void(CMIPS*)> m_emptyBlockHandler;

	CMIPSArchitecture* m_pArch = nullptr;
	CMIPSCoprocessor* m_pCOP[4];
	CMemoryMap* m_pMemoryMap = nullptr;
	std::unique_ptr<CMipsExecutor> m_executor;
	BreakpointSet m_breakpoints;

	CMIPSAnalysis* m_analysis = nullptr;
	CMIPSTags m_Comments;
	CMIPSTags m_Functions;
	CMIPSTags m_Variables;

	AddressTranslator m_pAddrTranslator = nullptr;
	TLBExceptionChecker m_TLBExceptionChecker = nullptr;

	enum REGISTER
	{
		R0 = 0,
		AT,
		V0,
		V1,
		A0,
		A1,
		A2,
		A3,
		T0,
		T1,
		T2,
		T3,
		T4,
		T5,
		T6,
		T7,
		S0,
		S1,
		S2,
		S3,
		S4,
		S5,
		S6,
		S7,
		T8,
		T9,
		K0,
		K1,
		GP,
		SP,
		FP,
		RA
	};

	enum
	{
		STATUS_IE = (1 << 0),
		STATUS_EXL = (1 << 1),
		STATUS_ERL = (1 << 2),
		STATUS_EIE = (1 << 16), //PS2 EE specific
	};

	static const char* m_sGPRName[];
};
