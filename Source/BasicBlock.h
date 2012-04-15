#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "MIPS.h"
#include "MemoryFunction.h"
#include <boost/intrusive_ptr.hpp>

namespace Jitter
{
	class CJitter;
};

class CBasicBlock;
typedef boost::intrusive_ptr<CBasicBlock> BasicBlockPtr;

static void intrusive_ptr_add_ref(CBasicBlock*);
static void intrusive_ptr_release(CBasicBlock*);

class CBasicBlock
{
public:
						CBasicBlock(CMIPS&, uint32, uint32);
	virtual				~CBasicBlock();
	unsigned int		Execute();
	void				Compile();

	uint32				GetBeginAddress() const;
	uint32				GetEndAddress() const;
	bool				IsCompiled() const;
	unsigned int		GetSelfLoopCount() const;
	void				SetSelfLoopCount(unsigned int);

protected:
	uint32				m_begin;
	uint32				m_end;
	CMIPS&				m_context;

	virtual void		CompileRange(CMipsJitter*);

private:
	friend void intrusive_ptr_add_ref(CBasicBlock*);
	friend void intrusive_ptr_release(CBasicBlock*);

	CMemoryFunction*	m_function;
	unsigned int		m_selfLoopCount;

	unsigned int		m_refCount;
};

static void intrusive_ptr_add_ref(CBasicBlock* block)
{
	block->m_refCount++;
}

static void intrusive_ptr_release(CBasicBlock* block)
{
	block->m_refCount--;
	if(block->m_refCount == 0)
	{
		delete block;
	}
}

#endif
