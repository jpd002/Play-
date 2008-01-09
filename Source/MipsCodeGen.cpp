#include "MipsCodeGen.h"

using namespace std;

CMipsCodeGen::CMipsCodeGen()
{

}

CMipsCodeGen::~CMipsCodeGen()
{

}

void CMipsCodeGen::EndQuota()
{
    assert(m_Shadow.GetCount() == 0);
    DumpVariables(m_IfStack.GetCount());
    DumpAllVariablesAndKeepState();
	CCodeGen::EndQuota();
}

void CMipsCodeGen::PushRel(size_t offset)
{
    VARIABLESTATUS* status = GetVariableStatus(offset);
    if(status == NULL)
    {
        CCodeGen::PushRel(offset);
    }
    else
    {
        switch(status->operandType)
        {
        case CONSTANT:
            CCodeGen::PushCst(status->operandValue);
            break;
        case REGISTER:
            CCodeGen::PushReg(status->operandValue);
            break;
        default:
            throw runtime_error("Unsupported operand type.");
            break;
        }
    }
}

void CMipsCodeGen::PullRel(size_t offset)
{
    InvalidateVariableStatus(offset);
    uint32 opType = m_Shadow.Pull();
    switch(opType)
    {
    case CONSTANT:
        {
            VARIABLESTATUS status;
            status.operandType = CONSTANT;
            status.operandValue = m_Shadow.Pull();
            status.isDirty = true;
            status.ifStackLevel = m_IfStack.GetCount();
            SetVariableStatus(offset, status);
        }
        break;
    case REGISTER:
        {
            m_Shadow.Push(REGISTER);
            CCodeGen::PullRel(offset);
//            VARIABLESTATUS status;
//            status.operandType = REGISTER;
//            status.operandValue = m_Shadow.Pull();
//            status.isDirty = true;
//            status.ifStackLevel = m_IfStack.GetCount();
//            SetVariableStatus(offset, status);
        }
        break;
    case RELATIVE:
        {
            //Don't try to make an alias, it could lead to some problems...
            m_Shadow.Push(RELATIVE);
            CCodeGen::PullRel(offset);
        }
        break;
    default:
        throw runtime_error("Unsupported operand type.");
        break;
    }
}

void CMipsCodeGen::FP_PushSingle(size_t offset)
{
    DumpVariable(offset);
    CCodeGen::FP_PushSingle(offset);
}

void CMipsCodeGen::FP_PushWord(size_t offset)
{
    DumpVariable(offset);
    CCodeGen::FP_PushWord(offset);
}

void CMipsCodeGen::FP_PullSingle(size_t offset)
{
    assert(GetVariableStatus(offset) == NULL);
    CCodeGen::FP_PullSingle(offset);
}

void CMipsCodeGen::FP_PullWordTruncate(size_t offset)
{
    assert(GetVariableStatus(offset) == NULL);
    CCodeGen::FP_PullWordTruncate(offset);
}

void CMipsCodeGen::EndIf()
{
    assert(m_Shadow.GetCount() == 0);
    DumpVariables(m_IfStack.GetCount());
    CCodeGen::EndIf();
}

void CMipsCodeGen::BeginIfElseAlt()
{
    assert(m_Shadow.GetCount() == 0);
    DumpVariables(m_IfStack.GetCount());
    CCodeGen::BeginIfElseAlt();
}

void CMipsCodeGen::Call(void* function, unsigned int paramCount, bool saveResult)
{
    //We need to dump any variable aliased with a register that won't be saved across the call
/*
    for(VariableStatusMap::iterator statusIterator(m_variableStatus.begin());
        m_variableStatus.end() != statusIterator; )
    {
        size_t variableId(statusIterator->first);
        const VARIABLESTATUS& status(statusIterator->second);
        if(status.isDirty && status.operandType == REGISTER && !IsRegisterSaved(status.operandValue))
        {
            PushReg(status.operandValue);
            CCodeGen::PullRel(variableId);
            m_variableStatus.erase(statusIterator++);
        }
        else
        {
            statusIterator++;
        }
    }
*/
    DumpVariables(m_IfStack.GetCount());
    CCodeGen::Call(function, paramCount, saveResult);
}

void CMipsCodeGen::SetVariableAsConstant(size_t variableId, uint32 value)
{
    VARIABLESTATUS status;
    status.operandType = CONSTANT;
    status.operandValue = value;
    status.isDirty = false;
    status.ifStackLevel = 0;
    SetVariableStatus(variableId, status);
}

void CMipsCodeGen::DumpVariable(size_t variableId)
{
    VariableStatusMap::iterator statusIterator(m_variableStatus.find(variableId));
    if(statusIterator == m_variableStatus.end()) return;
    const VARIABLESTATUS& status(statusIterator->second);
    if(!status.isDirty) return;
    SaveVariableStatus(variableId, status);
    m_variableStatus.erase(statusIterator++);
}

void CMipsCodeGen::DumpVariables(unsigned int ifStackLevel)
{
    for(VariableStatusMap::iterator statusIterator(m_variableStatus.begin());
        m_variableStatus.end() != statusIterator; )
    {
        size_t variableId(statusIterator->first);
        const VARIABLESTATUS& status(statusIterator->second);
        assert(status.ifStackLevel <= ifStackLevel);
        if(status.isDirty && status.ifStackLevel == ifStackLevel)
        {
            SaveVariableStatus(variableId, status);
            m_variableStatus.erase(statusIterator++);
        }
        else
        {
            statusIterator++;
        }
    }
}

void CMipsCodeGen::DumpAllVariablesAndKeepState()
{
    for(VariableStatusMap::iterator statusIterator(m_variableStatus.begin());
        m_variableStatus.end() != statusIterator; statusIterator++)
    {
        size_t variableId(statusIterator->first);
        const VARIABLESTATUS& status(statusIterator->second);
        if(status.isDirty)
        {
            SaveVariableStatus(variableId, status);
        }
    }
}

CMipsCodeGen::VARIABLESTATUS* CMipsCodeGen::GetVariableStatus(size_t variableId)
{
    VariableStatusMap::iterator statusIterator(m_variableStatus.find(variableId));
    return statusIterator == m_variableStatus.end() ? NULL : &statusIterator->second;
}

void CMipsCodeGen::InvalidateVariableStatus(size_t variableId)
{
    VARIABLESTATUS* oldVariableStatus = GetVariableStatus(variableId);
    if(oldVariableStatus != NULL)
    {
        switch(oldVariableStatus->operandType)
        {
        case CONSTANT:
            //No problems here
            break;
        default:
            throw runtime_error("Unsupported operand type.");
            break;
        }
        m_variableStatus.erase(m_variableStatus.find(variableId));
    }
}

void CMipsCodeGen::SetVariableStatus(size_t variableId, const VARIABLESTATUS& status)
{
    assert(GetVariableStatus(variableId) == NULL);
    m_variableStatus[variableId] = status;
}

void CMipsCodeGen::SaveVariableStatus(size_t variableId, const VARIABLESTATUS& status)
{
    switch(status.operandType)
    {
    case CONSTANT:
        PushCst(status.operandValue);
        CCodeGen::PullRel(variableId);
        break;
    case REGISTER:
        PushReg(status.operandValue);
        CCodeGen::PullRel(variableId);
        break;
    default:
        throw runtime_error("Unsupported operand type.");
        break;
    }
}
