#ifndef _IOP_SYSMEM_H_
#define _IOP_SYSMEM_H_

#include "IOP_Module.h"

namespace IOP
{
	class CSysMem : public CModule
	{
	public:
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000003
		};

	private:
		uint32			Allocate(uint32);
		uint32			AllocateSystemMemory(uint32, uint32, uint32);

		void			Log(const char*, ...);
	};
};

#endif
