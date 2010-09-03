#ifndef _VUBASICBLOCK_H_
#define _VUBASICBLOCK_H_

#include "BasicBlock.h"

class CVuBasicBlock : public CBasicBlock
{
public:
					CVuBasicBlock(CMIPS&, uint32, uint32);
	virtual			~CVuBasicBlock();

protected:
	virtual void	CompileRange(CMipsJitter*);

private:

};

#endif
