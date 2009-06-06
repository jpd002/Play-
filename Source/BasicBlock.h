#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "MIPS.h"

class CCodeGen;

class CBasicBlock
{
public:
                    CBasicBlock(CMIPS&, uint32, uint32);
    virtual         ~CBasicBlock();
    unsigned int    Execute();
    void            Compile();

    uint32          GetBeginAddress() const;
    uint32          GetEndAddress() const;
    bool            IsCompiled() const;
    unsigned int    GetSelfLoopCount() const;
    void            SetSelfLoopCount(unsigned int);
    CBasicBlock*    GetBranchHint() const;
    void            SetBranchHint(CBasicBlock*);

protected:
    uint32          m_begin;
    uint32          m_end;
    CMIPS&          m_context;

	virtual void	CompileRange(CCodeGen&);

private:
    uint8*          m_text;
	uint32			m_textSize;
    CBasicBlock*    m_branchHint;
    unsigned int    m_selfLoopCount;
};

#endif
