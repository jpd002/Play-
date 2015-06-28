#pragma once

#include <map>
#include <memory>
#include "Iop_Module.h"
#include "Ioman_Device.h"
#include "Stream.h"

namespace Iop
{
	class CIoman : public CModule
	{
	public:
		enum
		{
			FID_STDOUT = 1,
			FID_STDERR = 2
		};

		enum
		{
			SEEK_DIR_SET = 0,
			SEEK_DIR_CUR = 1,
			SEEK_DIR_END = 2
		};

		class CFile
		{
		public:
						CFile(uint32, CIoman&);
			virtual		~CFile();

						operator uint32();
		private:
			uint32		m_handle;
			CIoman&		m_ioman;
		};

		typedef std::shared_ptr<Ioman::CDevice> DevicePtr;

								CIoman(uint8*);
		virtual					~CIoman();

		virtual std::string		GetId() const override;
		virtual std::string		GetFunctionName(unsigned int) const override;
		virtual void			Invoke(CMIPS&, unsigned int) override;

		void					RegisterDevice(const char*, const DevicePtr&);

		uint32					Open(uint32, const char*);
		uint32					Close(uint32);
		uint32					Read(uint32, uint32, void*);
		uint32					Write(uint32, uint32, const void*);
		uint32					Seek(uint32, uint32, uint32);
		uint32					AddDrv(uint32);
		uint32					DelDrv(uint32);

		Framework::CStream*		GetFileStream(uint32);
		void					SetFileStream(uint32, Framework::CStream*);

	private:
		typedef std::map<uint32, Framework::CStream*> FileMapType;
		typedef std::map<std::string, DevicePtr> DeviceMapType;

		void					Open(CMIPS&);

		FileMapType				m_files;
		DeviceMapType			m_devices;
		uint8*					m_ram;
		uint32					m_nextFileHandle;
	};
}
