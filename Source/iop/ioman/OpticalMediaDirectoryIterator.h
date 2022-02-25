#pragma once

#include "../Ioman_DirectoryIterator.h"
#include "ISO9660/DirectoryRecord.h"

namespace Iop
{
	namespace Ioman
	{
		class COpticalMediaDirectoryIterator : public CDirectoryIterator
		{
		public:
			COpticalMediaDirectoryIterator(Framework::CStream*);
			virtual ~COpticalMediaDirectoryIterator();

			void ReadEntry(DIRENTRY* dirEntry) override;
			bool IsDone() override;

		private:
			void SeekToNextEntry();

			ISO9660::CDirectoryRecord m_currentRecord;
			Framework::CStream* m_directoryStream = nullptr;
		};
	}
}
