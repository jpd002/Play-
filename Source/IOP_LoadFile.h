#ifndef _IOP_LOADFILE_H_
#define _IOP_LOADFILE_H_

#include "IOP_Module.h"

namespace IOP
{
	class CLoadFile : public CModule
	{
	public:
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000006
		};

	private:
		void			LoadModule(void*, uint32, void*, uint32);
		void			Initialize(void*, uint32, void*, uint32);
		void			Log(const char*, ...);
	};

};

#endif
