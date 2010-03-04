#ifndef _PSP_SYSMEMUSERFORUSER_H_
#define _PSP_SYSMEMUSERFORUSER_H_

#include "PspModule.h"

namespace Psp
{
	class CSysMemUserForUser : public CModule
	{
	public:
								CSysMemUserForUser();
		virtual					~CSysMemUserForUser();

		void					Invoke(uint32, CMIPS&);
		std::string				GetName() const;

	private:
		void					SetCompilerVersion(uint32);

	};
}

#endif
