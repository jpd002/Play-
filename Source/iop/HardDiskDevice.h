#pragma once

#include "Ioman_Device.h"

namespace Iop
{
	namespace Ioman
	{
		class CHardDiskDevice : public CDevice
		{
		public:
			Framework::CStream* GetFile(uint32, const char*) override;
			Directory GetDirectory(const char*) override;
			void CreateDirectory(const char* devicePath) override;

		private:
			
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
