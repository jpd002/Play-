#ifndef _VIF_H_
#define _VIF_H_

#include "Types.h"
#include "Stream.h"
#include "MIPS.h"

class CVIF
{
public:
	static void		Reset();
	static void		SaveState(Framework::CStream*);
	static void		LoadState(Framework::CStream*);

	static uint32	ReceiveDMA1(uint32, uint32, bool);

	static uint32*	GetTop1Address();

	static void		StopVU(CMIPS*);
	static void		ProcessXGKICK(uint32);

	static bool		IsVU1Running();

private:
	enum STAT_BITS
	{
		STAT_VBS0	= 0x0001,
		STAT_VBS1	= 0x0100,
	};

	enum STAT1_BITS
	{
		STAT1_DBF = 0x80,
	};

#pragma pack(push, 1)
	struct CODE
	{
		unsigned int	nIMM	: 16;
		unsigned int	nNUM	: 8;
		unsigned int	nCMD	: 7;
		unsigned int	nI		: 1;
	};
#pragma pack(pop)

	class CVPU
	{
	public:
						CVPU(uint8**, uint8**, CMIPS*);
		virtual			~CVPU();
		virtual void	Reset();
		virtual uint32*	GetTOP();
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);
		virtual void	ProcessPacket(uint32, uint32);

	protected:
		struct STAT
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

		struct CYCLE
		{
			unsigned int	nCL			: 8;
			unsigned int	nWL			: 8;

			CYCLE& operator =(unsigned int nValue)
			{
				(*this) = *(CYCLE*)&nValue;
				return (*this);
			}
		};

		void			ExecuteMicro(uint32, uint32);
		virtual uint32	ExecuteCommand(CODE, uint32, uint32);
		virtual uint32	Cmd_UNPACK(CODE, uint32, uint32);

		uint32			Cmd_MPG(CODE, uint32, uint32);

		uint32			Unpack_V432(uint32, uint32, uint32);

		STAT			m_STAT;
		CYCLE			m_CYCLE;
		CODE			m_CODE;
		uint8			m_NUM;

		uint8**			m_pMicroMem;
		uint8**			m_pVUMem;
		CMIPS*			m_pCtx;
	};

	class CVPU1 : public CVPU
	{
	public:
						CVPU1(uint8**, uint8**, CMIPS*);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);
		virtual uint32*	GetTOP();
		virtual void	Reset();

	protected:
		virtual uint32	ExecuteCommand(CODE, uint32, uint32);
		virtual uint32	Cmd_DIRECT(CODE, uint32, uint32);
		virtual uint32	Cmd_UNPACK(CODE, uint32, uint32);

	private:
		uint32			m_BASE;
		uint32			m_OFST;
		uint32			m_TOP;
		uint32			m_TOPS;
	};

	static uint32	m_VPU_STAT;
	static CVPU*	m_pVPU[2];
};

#endif
