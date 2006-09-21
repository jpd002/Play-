#ifndef _IOP_UNKNOWN_H_
#define _IOP_UNKNOWN_H_

#include "IOP_Module.h"

namespace IOP
{
	class CUnknown : public CModule
	{
	public:
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	LoadState(Framework::CStream*);
		virtual void	SaveState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x00012345,
		};
	};
};

#endif
