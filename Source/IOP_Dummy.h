#ifndef _IOP_DUMMY_H_
#define _IOP_DUMMY_H_

#include "IOP_Module.h"

namespace IOP
{
	class CDummy : public CModule
	{
	public:
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	LoadState(Framework::CStream*);
		virtual void	SaveState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x00000030,
		};

	};
};

#endif
