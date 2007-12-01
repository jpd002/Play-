#ifndef _MIPSCODEGEN_H_
#define _MIPSCODEGEN_H_

#include "CodeGen.h"

class CVariableStatusInfo
{
public:

private:

};

class CMipsCodeGen : public CCodeGen
{
public:
                        CMipsCodeGen();
    virtual             ~CMipsCodeGen();

    virtual void        PushRel(size_t);
    virtual void        PullRel(size_t);

    void                SetVariableAsConstant(size_t, uint32);
    void                DumpVariables();

private:
    struct VARIABLESTATUS
    {
        uint32          operandType;
        uint32          operandValue;
        bool            isDirty;
    };

    typedef std::map< size_t, VARIABLESTATUS > VariableStatusMap;

    VARIABLESTATUS*     GetVariableStatus(size_t);
    void                SetVariableStatus(size_t, const VARIABLESTATUS&);

    VariableStatusMap   m_variableStatus;
};

#endif
