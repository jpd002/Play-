#ifndef _GSH_NULL_H_
#define _GSH_NULL_H_

#include "GSHandler.h"
#include "PS2VM.h"

class CGSH_Null : public CGSHandler
{
public:
                        CGSH_Null();
    virtual             ~CGSH_Null();

	virtual void        UpdateViewportImpl();
	virtual void        ProcessImageTransfer(uint32, uint32);

    static void         CreateGSHandler(CPS2VM&);

private:
    virtual void        InitializeImpl();
	virtual void        FlipImpl();
    static CGSHandler*  GSHandlerFactory(void*);
};

#endif
