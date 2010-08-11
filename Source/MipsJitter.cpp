#include <assert.h>
#include "MipsJitter.h"

CMipsJitter::CMipsJitter(Jitter::CCodeGen* codeGen)
: CJitter(codeGen)
, m_lastBlockLabel(-1)
{

}

CMipsJitter::~CMipsJitter()
{

}

void CMipsJitter::Begin()
{
	CJitter::Begin();

	m_lastBlockLabel = -1;
}

void CMipsJitter::End()
{
	if(m_lastBlockLabel != -1)
	{
		MarkLabel(m_lastBlockLabel);
	}

	CJitter::End();
}

void CMipsJitter::PushRel(size_t offset)
{
    VARIABLESTATUS* status = GetVariableStatus(offset);
    if(status == NULL)
    {
        CJitter::PushRel(offset);
    }
    else
    {
        switch(status->operandType)
        {
		case Jitter::SYM_CONSTANT:
            CJitter::PushCst(status->operandValue);
            break;
        default:
			throw std::runtime_error("Unsupported operand type.");
            break;
        }
    }
}

Jitter::CJitter::LABEL CMipsJitter::GetFinalBlockLabel()
{
	m_lastBlockLabel = CreateLabel();
	return m_lastBlockLabel;
}

void CMipsJitter::SetVariableAsConstant(size_t variableId, uint32 value)
{
    VARIABLESTATUS status;
	status.operandType = Jitter::SYM_CONSTANT;
    status.operandValue = value;
    SetVariableStatus(variableId, status);
}

CMipsJitter::VARIABLESTATUS* CMipsJitter::GetVariableStatus(size_t variableId)
{
    VariableStatusMap::iterator statusIterator(m_variableStatus.find(variableId));
    return statusIterator == m_variableStatus.end() ? NULL : &statusIterator->second;
}

void CMipsJitter::SetVariableStatus(size_t variableId, const VARIABLESTATUS& status)
{
    assert(GetVariableStatus(variableId) == NULL);
    m_variableStatus[variableId] = status;
}
