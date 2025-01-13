#pragma once

#include "BasicBlock.h"

class CEeBasicBlock : public CBasicBlock
{
public:
	using CBasicBlock::CBasicBlock;

	void SetFpRoundingMode(Jitter::CJitter::ROUNDINGMODE);

protected:
	void CompileProlog(CMipsJitter*) override;
	void CompileEpilog(CMipsJitter*, bool) override;

private:
	bool IsIdleLoopBlock() const;

	static constexpr auto DEFAULT_FP_ROUNDING_MODE = Jitter::CJitter::ROUND_TRUNCATE;
	Jitter::CJitter::ROUNDINGMODE m_fpRoundingMode = DEFAULT_FP_ROUNDING_MODE;
};
