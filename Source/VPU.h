#pragma once

#include "Types.h"
#include "VIF.h"
#include "MIPS.h"
#include "VuExecutor.h"
#include "Convertible.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

//#define DELAYED_MSCAL

class CVPU
{
public:
	typedef CVIF::CFifoStream StreamType;

						CVPU(CVIF&, unsigned int, const CVIF::VPUINIT&);
	virtual				~CVPU();

	void				Execute(bool);
	virtual void		Reset();
	virtual uint32		GetTOP() const;
	virtual uint32		GetITOP() const;
	virtual void		SaveState(Framework::CZipArchiveWriter&);
	virtual void		LoadState(Framework::CZipArchiveReader&);
	virtual void		ProcessPacket(StreamType&);

	void				StartMicroProgram(uint32);
	CMIPS&				GetContext() const;
	uint8*				GetVuMemory() const;
	bool				IsRunning() const;
	bool				IsWaitingForProgramEnd() const;

#ifdef DEBUGGER_INCLUDED
	bool				MustBreak() const;
	void				DisableBreakpointsOnce();

	void				SaveMiniState();
	const MIPSSTATE&	GetVuMiniState() const;
	uint8*				GetVuMemoryMiniState() const;
	uint8*				GetMicroMemoryMiniState() const;
	uint32				GetVuTopMiniState() const;
	uint32				GetVuItopMiniState() const;
#endif

protected:
	struct STAT : public convertible<uint32>
	{
		unsigned int	nVPS		: 2;
		unsigned int	nVEW		: 1;
		unsigned int	nReserved0	: 3;
		unsigned int	nMRK		: 1;
		unsigned int	nDBF		: 1;
		unsigned int	nVSS		: 1;
		unsigned int	nVFS		: 1;
		unsigned int	nVIS		: 1;
		unsigned int	nINT		: 1;
		unsigned int	nER0		: 1;
		unsigned int	nER1		: 1;
		unsigned int	nReserved2	: 10;
		unsigned int	nFQC		: 4;
		unsigned int	nReserved3	: 4;
	};

	struct CYCLE : public convertible<uint32>
	{
		unsigned int	nCL			: 8;
		unsigned int	nWL			: 8;
		unsigned int	reserved	: 16;
	};

#pragma pack(push, 1)
	struct CODE : public convertible<uint32>
	{
		unsigned int	nIMM	: 16;
		unsigned int	nNUM	: 8;
		unsigned int	nCMD	: 7;
		unsigned int	nI		: 1;
	};
	static_assert(sizeof(CODE) == sizeof(uint32), "Size of CODE struct must be 4 bytes.");
#pragma pack(pop)

	enum ADDMODE
	{
		MODE_NORMAL = 0,
		MODE_OFFSET = 1,
		MODE_DIFFERENCE = 2
	};

	enum MASKOP
	{
		MASK_DATA = 0,
		MASK_ROW = 1,
		MASK_COL = 2,
		MASK_MASK = 3
	};

	enum
	{
		CMDBUFFER_SIZE = 0x10000,
	};

	void				ExecuteMicroProgram(uint32);
	virtual void		PrepareMicroProgram();
#ifdef DELAYED_MSCAL
	void				StartDelayedMicroProgram(uint32);
	bool				ResumeDelayedMicroProgram();
#endif

	virtual void		ExecuteCommand(StreamType&, CODE);
	virtual void		Cmd_UNPACK(StreamType&, CODE, uint32);

	void				Cmd_MPG(StreamType&, CODE);
	void				Cmd_STROW(StreamType&, CODE);
	void				Cmd_STCOL(StreamType&, CODE);
	void				Cmd_STMASK(StreamType&, CODE);

	bool				Unpack_ReadValue(const CODE&, StreamType&, uint128&, bool);
	bool				Unpack_S32(StreamType&, uint128&);
	bool				Unpack_S16(StreamType&, uint128&, bool);
	bool				Unpack_S8(StreamType&, uint128&, bool);
	bool				Unpack_V16(StreamType&, uint128&, unsigned int, bool);
	bool				Unpack_V8(StreamType&, uint128&, unsigned int, bool);
	bool				Unpack_V32(StreamType&, uint128&, unsigned int);
	bool				Unpack_V45(StreamType&, uint128&);

	uint32				GetVbs() const;
	bool				IsVuRunning() const;
	uint32				GetMaskOp(unsigned int, unsigned int) const;

	void				DisassembleCommand(CODE);

	STAT				m_STAT;
	CYCLE				m_CYCLE;
	CODE				m_CODE;
	uint8				m_NUM;
	uint32				m_MODE;
	uint32				m_R[4];
	uint32				m_C[4];
	uint32				m_MASK;
	uint32				m_MARK;
	uint32				m_ITOP;
	uint32				m_ITOPS;
	uint32				m_readTick;
	uint32				m_writeTick;
#ifdef DELAYED_MSCAL
	uint32				m_pendingMicroProgram;
	CODE				m_previousCODE;
#endif

	uint128				m_buffer;

	uint8*				m_microMem;
	uint8*				m_vuMem;
	CMIPS*				m_ctx;
	CVIF&				m_vif;

#ifdef DEBUGGER_INCLUDED
	MIPSSTATE			m_vuMiniState;
	uint8*				m_microMemMiniState;
	uint8*				m_vuMemMiniState;
	uint32				m_topMiniState;
	uint32				m_itopMiniState;
#endif

	unsigned int		m_vpuNumber;
	CVuExecutor			m_executor;
};
