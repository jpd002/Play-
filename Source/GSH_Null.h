#ifndef _GSH_NULL_H_
#define _GSH_NULL_H_

#include "GSHandler.h"

class CGSH_Null : public CGSHandler
{
public:
								CGSH_Null();
	virtual						~CGSH_Null();

	virtual void				ProcessImageTransfer(uint32, uint32, bool);
	virtual void				ProcessClutTransfer(uint32, uint32);
	virtual void				ProcessLocalToLocalTransfer();
	virtual void				ReadFramebuffer(uint32, uint32, void*);

	static FactoryFunction		GetFactoryFunction();

private:
	virtual void				InitializeImpl();
	virtual void				ReleaseImpl();
	static CGSHandler*			GSHandlerFactory();
};

#endif
