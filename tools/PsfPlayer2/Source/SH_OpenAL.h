#ifndef _CSH_OPENAL_H_
#define _CSH_OPENAL_H_

#include <deque>
#include <time.h>
#include "Spu.h"
#include "openal/Device.h"
#include "openal/Context.h"
#include "openal/Source.h"
#include "openal/Buffer.h"

class CSH_OpenAL
{
public:
						CSH_OpenAL();
	virtual				~CSH_OpenAL();

	void				Reset();
	void				Update(CSpu&);
	bool				HasFreeBuffers();

private:
	typedef std::deque<ALuint> BufferList;

	enum
	{
		MAX_BUFFERS = 10,
	};

	void				RecycleBuffers();

	OpenAl::CDevice		m_device;
	OpenAl::CContext	m_context;
	OpenAl::CSource		m_source;

	BufferList			m_availableBuffers;
	uint64				m_lastUpdateTime;
	bool				m_mustSync;
};

#endif
