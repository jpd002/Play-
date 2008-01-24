#ifndef _POSIX_VOLUMESTREAM_H_
#define _POSIX_VOLUMESTREAM_H_

#include "Stream.h"

namespace Framework
{
	namespace Posix
	{
		class CVolumeStream : public CStream
		{
		public:
							CVolumeStream(const char*);
			virtual			~CVolumeStream();
			
			virtual void	Seek(int64, STREAM_SEEK_DIRECTION);
			virtual uint64	Tell();
			virtual uint64	Read(void*, uint64);
			virtual uint64	Write(const void*, uint64);
			virtual bool	IsEOF();
			
		private:
			void			SyncCache();
			
			int				m_fd;
			void*			m_cache;
			uint64			m_cacheSector;
			
			uint64			m_position;
			uint32			m_sectorSize;
		};
	}
}

#endif
