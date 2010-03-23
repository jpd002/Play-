#ifndef _PSP_IOFILEMGRFORUSER_H_
#define _PSP_IOFILEMGRFORUSER_H_

#include "PspModule.h"
#include "Psp_IoDevice.h"

namespace Psp
{
	class CIoFileMgrForUser : public CModule
	{
	public:
						CIoFileMgrForUser(uint8*);
		virtual			~CIoFileMgrForUser();

		void			Invoke(uint32, CMIPS&);
		std::string		GetName() const;

		void			RegisterDevice(const char*, const IoDevicePtr&);

		enum
		{
			FD_STDOUT	= 1,
			FD_STDIN	= 2,
			FD_STDERR	= 3,
			FD_FIRSTUSERID,
		};

		enum OPENFLAGS
		{
			OPEN_READ	= 0x01,
			OPEN_WRITE	= 0x02,
		};

	private:
		typedef std::tr1::shared_ptr<Framework::CStream> StreamPtr;
		typedef std::map<std::string, IoDevicePtr> IoDeviceList;
		typedef std::map<unsigned int, StreamPtr> FileList;

		uint32			IoOpen(uint32, uint32, uint32);
		uint32			IoRead(uint32, uint32, uint32);
		uint32			IoWrite(uint32, uint32, uint32);
		uint32			IoLseek(uint32, uint32, uint32);
		uint32			IoClose(uint32);

		uint8*			m_ram;
		uint32			m_nextFileId;
		IoDeviceList	m_devices;
		FileList		m_files;
	};

	typedef std::tr1::shared_ptr<CIoFileMgrForUser> IoFileMgrForUserModulePtr;
}

#endif
