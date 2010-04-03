#ifndef _PSP_KERNELLIBRARY_H_
#define _PSP_KERNELLIBRARY_H_

#include "PspModule.h"

namespace Psp
{
	class CKernelLibrary : public CModule
	{
	public:
						CKernelLibrary();
		virtual			~CKernelLibrary();
		
		std::string		GetName() const;
		void			Invoke(uint32, CMIPS&);

	private:


	};
}

#endif
