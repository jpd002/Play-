#ifndef _PSP_IOFILEMGRFORUSER_H_
#define _PSP_IOFILEMGRFORUSER_H_

#include "PspModule.h"

namespace Psp
{
	class CIoFileMgrForUser : public CModule
	{
	public:
						CIoFileMgrForUser(uint8*);
		virtual			~CIoFileMgrForUser();

		void			Invoke(uint32, CMIPS&);
		std::string		GetName() const;

		enum
		{
			FD_STDOUT	= 1,
			FD_STDIN	= 2,
			FD_STDERR	= 3,
		};

	private:
		uint32			IoWrite(uint32, uint32, uint32);

		uint8*			m_ram;
	};
}

#endif
