#pragma once

#include <vector>
#include "../Ioman_Device.h"

namespace Iop
{
	namespace Ioman
	{
		class CHardDiskDevice : public CDevice
		{
		public:
			CHardDiskDevice();

			Framework::CStream* GetFile(uint32, const char*) override;
			DirectoryIteratorPtr GetDirectory(const char*) override;
			fs::path GetMountPath(const char*) override;

		private:
			void CreatePartition(const std::vector<std::string>&);

			fs::path m_basePath;
		};

		class CHardDiskPartition : public Framework::CStream
		{
		public:
			void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
			uint64 Tell() override;
			uint64 Read(void*, uint64) override;
			uint64 Write(const void*, uint64) override;
			bool IsEOF() override;
		};
	}
}
