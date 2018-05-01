#ifndef _PSP_STDIOFORUSER_H_
#define _PSP_STDIOFORUSER_H_

#include "PspModule.h"

namespace Psp
{
	class CStdioForUser : public CModule
	{
	public:
		CStdioForUser();
		virtual ~CStdioForUser();

		std::string GetName() const;
		void Invoke(uint32, CMIPS&);
	};
}

#endif
