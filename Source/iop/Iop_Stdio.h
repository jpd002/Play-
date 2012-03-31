#ifndef _IOP_STDIO_H_
#define _IOP_STDIO_H_

#include "Iop_Module.h"
#include "ArgumentIterator.h"

namespace Iop
{
	class CStdio : public CModule
	{
	public:
						CStdio(uint8*);
		virtual			~CStdio();

		std::string		GetId() const;
		std::string		GetFunctionName(unsigned int) const;
		void			Invoke(CMIPS&, unsigned int);

		void			__printf(CMIPS&);
		std::string		PrintFormatted(CArgumentIterator&);

	private:
		uint8*			m_ram;
	};
}

#endif
