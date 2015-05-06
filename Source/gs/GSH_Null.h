#pragma once

#include "GSHandler.h"

class CGSH_Null : public CGSHandler
{
public:
								CGSH_Null();
	virtual						~CGSH_Null();

	virtual void				ProcessImageTransfer() override;
	virtual void				ProcessClutTransfer(uint32, uint32) override;
	virtual void				ProcessLocalToLocalTransfer() override;
	virtual void				ReadFramebuffer(uint32, uint32, void*) override;

	static FactoryFunction		GetFactoryFunction();

private:
	virtual void				InitializeImpl() override;
	virtual void				ReleaseImpl() override;

	static CGSHandler*			GSHandlerFactory();
};
