#ifndef _GSH_NULL_H_
#define _GSH_NULL_H_

#include "GSHandler.h"

class CGSH_Null : public CGSHandler
{
public:
                        CGSH_Null();
    virtual             ~CGSH_Null();

	virtual void        UpdateViewport();
	virtual void        ProcessImageTransfer(uint32, uint32);
	virtual void        Flip();

    static void         CreateGSHandler();

private:
    static CGSHandler*  GSHandlerFactory(void*);
};

#endif
