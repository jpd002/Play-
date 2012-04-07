#ifndef _IOP_INTRMAN_H_
#define _IOP_INTRMAN_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CIntrman : public CModule
	{
	public:
						CIntrman(CIopBios&, uint8*);
		virtual			~CIntrman();

		std::string		GetId() const;
		std::string		GetFunctionName(unsigned int) const;
		void			Invoke(CMIPS&, unsigned int);

	private:
		uint32			RegisterIntrHandler(uint32, uint32, uint32, uint32);
		uint32			ReleaseIntrHandler(uint32);
		uint32			EnableIntrLine(CMIPS&, uint32);
		uint32			DisableIntrLine(CMIPS&, uint32, uint32);
		uint32			EnableInterrupts(CMIPS&);
		uint32			DisableInterrupts(CMIPS&);
		uint32			SuspendInterrupts(CMIPS&, uint32*);
		uint32			ResumeInterrupts(CMIPS&, uint32);
		uint32			QueryIntrContext(CMIPS&);
		uint8*			m_ram;
		CIopBios&		m_bios;
	};
}

#endif

