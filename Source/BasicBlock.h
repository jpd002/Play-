#pragma once

#include "MIPS.h"
#include "MemoryFunction.h"
#ifdef AOT_BUILD_CACHE
#include "StdStream.h"
#include <mutex>
#endif

struct AOT_BLOCK_KEY
{
	uint32		crc;
	uint32		begin;
	uint32		end;

	bool operator <(const AOT_BLOCK_KEY& k2) const
	{
		const AOT_BLOCK_KEY& k1 = (*this);
		if(k1.crc == k2.crc)
		{
			if(k1.begin == k2.begin)
			{
				return k1.end < k2.end;
			}
			else
			{
				return k1.begin < k2.begin;
			}
		}
		else
		{
			return k1.crc < k2.crc;
		}
	}
};

namespace Jitter
{
	class CJitter;
};

class CBasicBlock
{
public:
	typedef std::set<uint32> RefererSet;

	enum
	{
		MAX_LINKEDBLOCKS = 2
	};
									CBasicBlock(CMIPS&, uint32, uint32);
	virtual							~CBasicBlock();
	unsigned int					Execute();
	void							Compile();

	uint32							GetBeginAddress() const;
	uint32							GetEndAddress() const;
	bool							IsCompiled() const;
	unsigned int					GetSelfLoopCount() const;
	void							SetSelfLoopCount(unsigned int);

	CBasicBlock*					GetLinkedBlock(unsigned int) const;
	void							SetLinkedBlock(unsigned int, CBasicBlock*);

	const RefererSet&				GetReferers() const;
	void							ClearReferers();

#ifdef AOT_BUILD_CACHE
	static void						SetAotBlockOutputStream(Framework::CStdStream*);
#endif

protected:
	uint32							m_begin;
	uint32							m_end;
	CMIPS&							m_context;

	virtual void					CompileRange(CMipsJitter*);

private:
	void							AddReferer(uint32);

#ifdef AOT_BUILD_CACHE
	static Framework::CStdStream*	m_aotBlockOutputStream;
	static std::mutex				m_aotBlockOutputStreamMutex;
#endif

#ifndef AOT_USE_CACHE
	CMemoryFunction					m_function;
#else
	void							(*m_function)(void*);
#endif

	unsigned int					m_selfLoopCount;
	CBasicBlock*					m_linkedBlocks[MAX_LINKEDBLOCKS];
	RefererSet						m_referers;
};
