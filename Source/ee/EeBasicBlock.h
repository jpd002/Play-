#pragma once

#include "BasicBlock.h"

class CEeBasicBlock : public CBasicBlock
{
public:
	using CBasicBlock::CBasicBlock;

protected:
	void CompileEpilog(CMipsJitter*) override;

private:
	bool IsIdleLoopBlock() const;
};
