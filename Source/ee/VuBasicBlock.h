#pragma once

#include "../BasicBlock.h"

class CVuBasicBlock : public CBasicBlock
{
public:
					CVuBasicBlock(CMIPS&, uint32, uint32);
	virtual			~CVuBasicBlock();

protected:
	void			CompileRange(CMipsJitter*) override;

private:

};
