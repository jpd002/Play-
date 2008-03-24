#ifndef _VUEXECUTOR_H_
#define _VUEXECUTOR_H_

#include "MipsExecutor.h"

class CVuExecutor : public CMipsExecutor
{
public:
                    CVuExecutor(CMIPS&);
    virtual         ~CVuExecutor();



private:
    virtual void    PartitionFunction(uint32);

};

#endif

