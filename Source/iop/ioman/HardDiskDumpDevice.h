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
			bool TryGetStat(const char*, bool&, STAT&) override;

		private:
			std::unique_ptr<Framework::CStream> m_stream;
		};

	class CHardDiskDumpDirectoryIterator : public CDirectoryIterator
	{
	public:
		CHardDiskDumpDirectoryIterator(std::vector<Hdd::APA_HEADER>);

		void ReadEntry(DIRENTRY*) override;
		bool IsDone() override;
		
	private:
		size_t m_index = 0;
		std::vector<Hdd::APA_HEADER> m_partitions;
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
