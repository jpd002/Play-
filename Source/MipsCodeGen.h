#ifndef _MIPSCODEGEN_H_
#define _MIPSCODEGEN_H_

#include "CodeGen.h"

class CMipsCodeGen : public CCodeGen
{
public:
                        CMipsCodeGen();
    virtual             ~CMipsCodeGen();

	virtual void		EndQuota();
	
    virtual void        PushRel(size_t);
    virtual void        PullRel(size_t);

    virtual void        FP_PushSingle(size_t);
    virtual void        FP_PushWord(size_t);
    virtual void        FP_PullSingle(size_t);
    virtual void        FP_PullWordTruncate(size_t);

    virtual void        MD_PushRel(size_t);
    virtual void        MD_PushRelExpand(size_t);
    virtual void        MD_PullRel(size_t);

    virtual void        BeginIf(bool);
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

    void                DumpVariable(size_t);
    void                DumpAllVariablesAndKeepState();
    VARIABLESTATUS*     GetVariableStatus(size_t);
    void                SetVariableStatus(size_t, const VARIABLESTATUS&);
    void                SaveVariableStatus(size_t, const VARIABLESTATUS&);
    void                InvalidateVariableStatus(size_t);

    VariableStatusMap   m_variableStatus;
};

#endif
