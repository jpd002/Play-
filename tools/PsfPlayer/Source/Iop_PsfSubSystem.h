#ifndef _IOPPSFSUBSYSTEM_H_
#define _IOPPSFSUBSYSTEM_H_

#include "iop/Iop_SubSystem.h"
#include "PsfVmSubSystem.h"

namespace Iop
{
	class CPsfSubSystem : public CPsfVmSubSystem
	{
	public:
									CPsfSubSystem();
		virtual						~CPsfSubSystem();

		virtual void				Reset();
		virtual CMIPS&				GetCpu();
		virtual uint8*				GetRam();
		virtual Iop::CSpuBase&		GetSpuCore(unsigned int);

#ifdef DEBUGGER_INCLUDED
		virtual bool				MustBreak();
		virtual MipsModuleList		GetModuleList();
		virtual void				LoadDebugTags(Framework::Xml::CNode*);
		virtual void				SaveDebugTags(Framework::Xml::CNode*);
#endif

		void						SetBios(const Iop::CSubSystem::BiosPtr&);

		virtual void				Update(bool, CSoundHandler*);

	private:
		CSubSystem					m_iop;
		int							m_spuUpdateCounter;
		int							m_frameCounter;

		enum
		{
			SAMPLE_COUNT = 44,
			BLOCK_SIZE = SAMPLE_COUNT * 2,
			BLOCK_COUNT = 10,
		};

		int16						m_samples[BLOCK_SIZE * BLOCK_COUNT];
		int							m_currentBlock;
	};

	typedef std::tr1::shared_ptr<CPsfSubSystem> PsfSubSystemPtr;
}

#endif
