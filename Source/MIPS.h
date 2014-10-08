#ifndef _MIPS_H_
#define _MIPS_H_

#include "Types.h"
#include "MemoryMap.h"
#include "MIPSArchitecture.h"
#include "MIPSCoprocessor.h"
#include "MIPSAnalysis.h"
#include "MIPSTags.h"
#include "uint128.h"
#include <set>

struct REGISTER_PIPELINE
{
	uint32		counter;
	uint32		heldValue;
};

enum
{
	MACFLAG_PIPELINE_SLOTS = 4,
};

struct MACFLAG_PIPELINE
{
	uint32		index;
	uint32		values[MACFLAG_PIPELINE_SLOTS];
	uint32		pipeTimes[MACFLAG_PIPELINE_SLOTS];
};

enum
{
	MIPS_EXCEPTION_NONE = 0,
	MIPS_EXCEPTION_SYSCALL,
	MIPS_EXCEPTION_CHECKPENDINGINT,
	MIPS_EXCEPTION_RETURNFROMEXCEPTION,
	MIPS_EXCEPTION_CALLMS,
};

struct MIPSSTATE
{
	uint32				nPC;
	uint32				nDelayedJumpAddr;
	uint32				nHasException;

#ifdef WIN32
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
	uint128				nGPR[32];

	uint32				nHI[2];
	uint32				nLO[2];
	uint32				nHI1[2];
	uint32				nLO1[2];
	uint32				nSA;

	//COP0
	uint32				nCOP0[32];

	//COP1
	uint32				nCOP1[32];
	uint32				nCOP1A;
	uint32				nFCSR;

	//COP2
#ifdef WIN32
__declspec(align(16))
#else
__attribute__((aligned(16)))
#endif
	uint128				nCOP2[33];

	uint128				nCOP2A;

	uint128				nCOP2VF_PreUp;
	uint128				nCOP2VF_UpRes;

	uint32				nCOP2Q;
	uint32				nCOP2I;
	uint32				nCOP2P;
	uint32				nCOP2R;
	uint32				nCOP2CF;
	uint32				nCOP2MF;
	uint32				nCOP2T;

	uint32				nCOP2VI[16];

	REGISTER_PIPELINE	pipeQ;
	REGISTER_PIPELINE	pipeP;
	MACFLAG_PIPELINE	pipeMac;

	uint32				pipeTime;

	uint32				cmsar0;
	uint32				callMsEnabled;
	uint32				callMsAddr;

	void*				vuMem;
};

#define MIPS_INVALID_PC			(0x00000001)

class CMIPS
{
public:
	typedef unsigned int		(*TickFunctionType)(unsigned int, CMIPS*);
	typedef void				(*SysCallHandlerType)(CMIPS*);
	typedef uint32				(*AddressTranslator)(CMIPS*, uint32);
	typedef std::set<uint32>	BreakpointSet;

								CMIPS(MEMORYMAP_ENDIANESS);
								~CMIPS();
	void						ToggleBreakpoint(uint32);
	bool						IsBranch(uint32);
	static long					GetBranch(uint16);
	static uint32				TranslateAddress64(CMIPS*, uint32);
	static void					DefaultSysCallHandler(CMIPS*);

	void						Reset();

	bool						GenerateInterrupt(uint32);
	bool						GenerateException(uint32);

	MIPSSTATE					m_State;

	CMIPSArchitecture*			m_pArch;
	CMIPSCoprocessor*			m_pCOP[4];
	CMemoryMap*					m_pMemoryMap;
	BreakpointSet				m_breakpoints;

	CMIPSAnalysis*				m_analysis;
	CMIPSTags					m_Comments;
	CMIPSTags					m_Functions;

	AddressTranslator			m_pAddrTranslator;

	enum REGISTER
	{
		R0 = 0,	AT,	V0,	V1,	A0,	A1,	A2,	A3,
		T0,		T1,	T2,	T3,	T4,	T5,	T6,	T7,
		S0,		S1,	S2,	S3,	S4,	S5,	S6,	S7,
		T8,		T9,	K0,	K1,	GP,	SP,	FP,	RA
	};

	enum
	{
		STATUS_INT = 0x01,
		STATUS_EXL = 0x02,
		STATUS_ERL = 0x04
	};

	static const char*			m_sGPRName[];
};

#endif
