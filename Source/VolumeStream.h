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
			CVolumeStream(char);
			virtual ~CVolumeStream();
			virtual void Seek(int64, STREAM_SEEK_DIRECTION);
			virtual uint64 Tell();
			virtual uint64 Read(void*, uint64);
			virtual uint64 Write(const void*, uint64);
			virtual bool IsEOF();

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
