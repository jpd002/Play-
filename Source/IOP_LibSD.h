#ifndef _IOP_LIBSD_H_
#define _IOP_LIBSD_H_

#include "IOP_Module.h"

namespace IOP
{
	class CLibSD : public CModule
	{
	public:
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000701
		};

	private:
		void			GetBufferSize(void*, uint32, void*, uint32);
	};

};

#endif
