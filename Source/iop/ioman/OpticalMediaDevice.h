#pragma once

#include <memory>
#include "../Ioman_Device.h"
#include "../../OpticalMedia.h"

namespace Iop
{
	namespace Ioman
	{
		class COpticalMediaDevice : public CDevice
		{
		public:
			typedef std::unique_ptr<COpticalMedia> OpticalMediaPtr;

			COpticalMediaDevice(OpticalMediaPtr&);
			virtual ~COpticalMediaDevice() = default;

			Framework::CStream* GetFile(uint32, const char*) override;
			DirectoryIteratorPtr GetDirectory(const char*) override;

		private:
			static char FixSlashes(char);
			static std::string RemoveExtraVersionSpecifiers(const std::string&);

			OpticalMediaPtr& m_opticalMedia;
		};

		class COpticalMediaFile : public Framework::CStream
		{
		public:
			COpticalMediaFile(std::unique_ptr<Framework::CStream>);
			virtual ~COpticalMediaFile() = default;

			void Seek(int64, Framework::STREAM_SEEK_DIRECTION) override;
			uint64 Tell() override;
			uint64 Read(void*, uint64) override;
			uint64 Write(const void*, uint64) override;
			bool IsEOF() override;

		private:
			std::unique_ptr<Framework::CStream> m_baseStream;
		};
	}
}
