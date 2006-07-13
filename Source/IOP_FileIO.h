#ifndef _IOP_FILEIO_H_
#define _IOP_FILEIO_H_

#include <stdio.h>
#include "IOP_Module.h"
#include "List.h"
#include "Stream.h"

namespace IOP
{
	class CFileIO : public CModule
	{
	public:
												CFileIO();
												~CFileIO();
		virtual void							Invoke(uint32, void*, uint32, void*, uint32);
		virtual void							SaveState(Framework::CStream*);
		virtual void							LoadState(Framework::CStream*);
		Framework::CStream*						GetFile(uint32, const char*);
		uint32									Read(uint32, uint32, void*);
		uint32									Write(uint32, uint32, void*);

		enum MODULE_ID
		{
			MODULE_ID	= 0x80000001
		};

		enum OPEN_FLAGS
		{
			O_RDONLY	= 0x00000001,
			O_WRONLY	= 0x00000002,
			O_RDWR		= 0x00000003,
			O_CREAT		= 0x00000200,
		};

	private:
		class CDevice
		{
		public:
												CDevice(const char*);
			virtual								~CDevice() {}
			virtual Framework::CStream*			OpenFile(uint32, const char*) = 0;
			virtual const char*					GetName();

		private:
			const char*							m_sName;
		};

		class CDirectoryDevice : public CDevice
		{
		public:
												CDirectoryDevice(const char*, const char*);
			virtual Framework::CStream*			OpenFile(uint32, const char*);
		private:
			const char*							m_sBasePathPreference;
		};

		class CCDROM0Device : public CDevice
		{
		public:
												CCDROM0Device();
			virtual								~CCDROM0Device();
			virtual Framework::CStream*			OpenFile(uint32, const char*);
		};

		bool									SplitPath(const char*, char*, char*);
		CDevice*								FindDevice(const char*);
		uint32									RegisterFile(Framework::CStream*);

		uint32									Open(uint32, const char*);
		uint32									Close(uint32);
		uint32									Seek(uint32, uint32, uint32);

		void									Log(const char*, ...);

		uint32									m_nFileID;

		Framework::CList<Framework::CStream>	m_File;
		Framework::CList<CDevice>				m_Device;
	};
}

#endif
