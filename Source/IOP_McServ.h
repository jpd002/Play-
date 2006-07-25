#ifndef _IOP_MCSERV_H_
#define _IOP_MCSERV_H_

#include "IOP_Module.h"

namespace IOP
{

	class CMcServ : public CModule
	{
	public:
						CMcServ();
		virtual			~CMcServ();
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
		};

	private:
		void			GetInfo(void*, uint32, void*, uint32);
		void			GetVersionInformation(void*, uint32, void*, uint32);
		void			Log(const char*, ...);
	};

}

#endif
