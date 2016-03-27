#ifndef _IOPPSFSUBSYSTEM_H_
#define _IOPPSFSUBSYSTEM_H_

#include "iop/Iop_SubSystem.h"
#include "PsfVmSubSystem.h"

namespace Iop
{
	class CPsfSubSystem : public CPsfVmSubSystem
	{
	public:
											CPsfSubSystem(bool ps2Mode);
		virtual								~CPsfSubSystem();

		void								Reset() override;
		CMIPS&								GetCpu() override;
		uint8*								GetRam() override;
		uint8*								GetSpr() override;
		Iop::CSpuBase&						GetSpuCore(unsigned int) override;

#ifdef DEBUGGER_INCLUDED
		virtual bool						MustBreak();
		virtual void						DisableBreakpointsOnce();
		virtual CBiosDebugInfoProvider*		GetBiosDebugInfoProvider();
		virtual void						LoadDebugTags(Framework::Xml::CNode*);
		virtual void						SaveDebugTags(Framework::Xml::CNode*);
#endif

		void								SetBios(const Iop::BiosBasePtr&);

		void								Update(bool, CSoundHandler*) override;

	private:
		CSubSystem							m_iop;
		uint32								m_frameTicks;
		uint32								m_spuUpdateTicks;
		int									m_frameCounter;
		int									m_spuUpdateCounter;

		enum
		{
			SAMPLE_COUNT = 44,
			BLOCK_SIZE = SAMPLE_COUNT * 2,
			BLOCK_COUNT = 10,
		};

		int16								m_samples[BLOCK_SIZE * BLOCK_COUNT];
		int									m_currentBlock;
	};

	typedef std::shared_ptr<CPsfSubSystem> PsfSubSystemPtr;
}

#endif
