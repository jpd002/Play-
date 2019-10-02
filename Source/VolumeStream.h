#ifndef _VOLUMESTREAM_H_
#define _VOLUMESTREAM_H_

#include <windows.h>
#include "Stream.h"

namespace Framework
{
	namespace Win32
	{
		class CVolumeStream : public CStream
		{
		public:
			CVolumeStream(const TCHAR*);
			virtual ~CVolumeStream();

			void Seek(int64, STREAM_SEEK_DIRECTION);
			uint64 Tell() override;
			uint64 Read(void*, uint64) override;
			uint64 Write(const void*, uint64) override;
			bool IsEOF() override;

		private:
			void SyncCache();

			HANDLE m_nVolume;
			void* m_pCache;
			uint64 m_nCacheSector;

			uint64 m_nPosition;
			uint64 m_nSectorSize;
		};
	}
}

#endif
