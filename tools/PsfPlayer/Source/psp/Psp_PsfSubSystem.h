#ifndef _PSP_SUBSYSTEM_H_
#define _PSP_SUBSYSTEM_H_

#include "Types.h"
#include "MA_ALLEGREX.h"
#include "COP_SCU.h"
#include "COP_FPU.h"
#include "MIPS.h"
#include "MIPSExecutor.h"
#include "../PsfVmSubSystem.h"
#include "Psp_PsfBios.h"
#include "MemStream.h"

namespace Psp
{
	class CPsfSubSystem : public CPsfVmSubSystem
	{
	public:
		enum
		{
			DEFAULT_RAMSIZE	= 0x02000000,
		};

								CPsfSubSystem(uint32 = DEFAULT_RAMSIZE);
		virtual					~CPsfSubSystem();

		virtual void			Reset();
		virtual CMIPS&			GetCpu();
		virtual uint8*			GetRam();
		virtual Iop::CSpuBase&	GetSpuCore(unsigned int);

#ifdef DEBUGGER_INCLUDED
		virtual MipsModuleList	GetModuleList();
		virtual bool			MustBreak();
		virtual void			LoadDebugTags(Framework::Xml::CNode*);
		virtual void			SaveDebugTags(Framework::Xml::CNode*);
#endif

		virtual void			Update(bool, CSoundHandler*);

		Psp::CPsfBios&			GetBios();

	private:
		enum SPURAMSIZE
		{
			SPURAMSIZE	= 0x400000,
		};

		int						ExecuteCpu(bool);

		CMIPS					m_cpu;
		CMipsExecutor			m_executor;
		CMA_ALLEGREX			m_cpuArch;
		CCOP_SCU				m_copScu;
		CCOP_FPU				m_copFpu;
		uint8*					m_ram;
		uint32					m_ramSize;
		uint8*					m_spuRam;
		Iop::CSpuBase			m_spuCore0;
		Iop::CSpuBase			m_spuCore1;
		Framework::CMemStream	m_audioStream;
		int						m_samplesToFrame;

		Psp::CPsfBios			m_bios;
	};

	typedef std::tr1::shared_ptr<CPsfSubSystem> PsfSubSystemPtr;
}

#endif
