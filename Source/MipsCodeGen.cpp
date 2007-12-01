#include "MipsCodeGen.h"

using namespace std;

CMipsCodeGen::CMipsCodeGen()
{

}

CMipsCodeGen::~CMipsCodeGen()
{

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
    uint32 opType = m_Shadow.Pull();
    switch(opType)
    {
    case CONSTANT:
        {
            VARIABLESTATUS status;
            status.operandType = CONSTANT;
            status.operandValue = m_Shadow.Pull();
            status.isDirty = true;
            SetVariableStatus(offset, status);
        }
        break;
    case REGISTER:
        {
            VARIABLESTATUS status;
            status.operandType = REGISTER;
            status.operandValue = m_Shadow.Pull();
            status.isDirty = true;
            SetVariableStatus(offset, status);
        }
        break;
    default:
        throw runtime_error("Unsupported operand type.");
        break;
    }
}

void CMipsCodeGen::SetVariableAsConstant(size_t variableId, uint32 value)
{
    VARIABLESTATUS status;
    status.operandType = CONSTANT;
    status.operandValue = value;
    status.isDirty = false;
    SetVariableStatus(variableId, status);
}

void CMipsCodeGen::DumpVariables()
{
    for(VariableStatusMap::const_iterator statusIterator(m_variableStatus.begin());
        m_variableStatus.end() != statusIterator; statusIterator++)
    {
        size_t variableId(statusIterator->first);
        const VARIABLESTATUS& status(statusIterator->second);
        if(status.isDirty == false) continue;
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
}

CMipsCodeGen::VARIABLESTATUS* CMipsCodeGen::GetVariableStatus(size_t variableId)
{
    VariableStatusMap::iterator statusIterator(m_variableStatus.find(variableId));
    return statusIterator == m_variableStatus.end() ? NULL : &statusIterator->second;
}

void CMipsCodeGen::SetVariableStatus(size_t variableId, const VARIABLESTATUS& status)
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
    }
    m_variableStatus[variableId] = status;
}
