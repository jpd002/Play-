#pragma once

#include "GSHandler.h"

class CGSH_Null : public CGSHandler
{
public:
	CGSH_Null() = default;
	virtual ~CGSH_Null() = default;

	void ProcessHostToLocalTransfer() override;
	void ProcessLocalToHostTransfer() override;
	void ProcessLocalToLocalTransfer() override;
	void ProcessClutTransfer(uint32, uint32) override;

	static FactoryFunction GetFactoryFunction();

private:
	void InitializeImpl() override;
	void ReleaseImpl() override;
};
