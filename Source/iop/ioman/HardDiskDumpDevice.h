#pragma once

#include "../Ioman_Device.h"
#include "hdd/PfsReader.h"

namespace Iop
{
	namespace Ioman
	{
		class CHardDiskDumpDevice : public CDevice
		{
		public:
			CHardDiskDumpDevice(std::unique_ptr<Framework::CStream>);
			virtual ~CHardDiskDumpDevice() = default;
			
			Framework::CStream* GetFile(uint32, const char*) override;
			DirectoryIteratorPtr GetDirectory(const char*) override;
			DevicePtr Mount(const char*) override;
			
		private:
			std::unique_ptr<Framework::CStream> m_stream;
		};
	
		class CHardDiskDumpPartitionDevice : public CDevice
		{
		public:
			CHardDiskDumpPartitionDevice(Framework::CStream&, const Hdd::APA_HEADER&);
			virtual ~CHardDiskDumpPartitionDevice() = default;
			
			Framework::CStream* GetFile(uint32, const char*) override;
			DirectoryIteratorPtr GetDirectory(const char*) override;
			
		private:
			Hdd::CPfsReader m_pfsReader;
		};
	}
}
