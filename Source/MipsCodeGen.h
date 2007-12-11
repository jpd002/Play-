#ifndef _MIPSCODEGEN_H_
#define _MIPSCODEGEN_H_

#include "CodeGen.h"

class CMipsCodeGen : public CCodeGen
{
public:
                        CMipsCodeGen();
    virtual             ~CMipsCodeGen();

    virtual void        PushRel(size_t);
    virtual void        PullRel(size_t);

    virtual void        EndIf();
    virtual void        BeginIfElseAlt();

    virtual void        Call(void*, unsigned int, bool);

    void                SetVariableAsConstant(size_t, uint32);
    void                DumpVariables(unsigned int);

private:
    struct VARIABLESTATUS
    {
        uint32          operandType;
        uint32          operandValue;
        bool            isDirty;
        unsigned int    ifStackLevel;
    };

    typedef std::map< size_t, VARIABLESTATUS > VariableStatusMap;

    VARIABLESTATUS*     GetVariableStatus(size_t);
    void                SetVariableStatus(size_t, const VARIABLESTATUS&);
    void                InvalidateVariableStatus(size_t);

    VariableStatusMap   m_variableStatus;
};

#endif
