#include <assert.h>
#include "MipsJitter.h"
#include "MIPS.h"
#include "offsetof_def.h"

CMipsJitter::CMipsJitter(Jitter::CCodeGen* codeGen)
    : CJitter(codeGen)
    , m_firstBlockLabel(-1)
    , m_lastBlockLabel(-1)
{
	for(unsigned int i = 0; i < 4; i++)
	{
		SetVariableAsConstant(
		    offsetof(CMIPS, m_State.nGPR[CMIPS::R0].nV[i]),
		    0);
	}
}

void CMipsJitter::Begin()
{
	CJitter::Begin();
	m_firstBlockLabel = -1;
	m_lastBlockLabel = -1;
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

void CMipsJitter::PushRel64(size_t offset)
{
	VARIABLESTATUS* statusLo = GetVariableStatus(offset + 0);
	VARIABLESTATUS* statusHi = GetVariableStatus(offset + 4);
	if(statusLo == NULL || statusHi == NULL)
	{
		CJitter::PushRel64(offset);
	}
	else
	{
		if((statusLo->operandType == Jitter::SYM_CONSTANT) && (statusHi->operandType == Jitter::SYM_CONSTANT))
		{
			uint64 result = static_cast<uint64>(statusLo->operandValue) | (static_cast<uint64>(statusHi->operandValue) << 32);
			CJitter::PushCst64(result);
		}
		else
		{
			throw std::runtime_error("Unsupported operand type.");
		}
	}
}

Jitter::CJitter::LABEL CMipsJitter::GetFirstBlockLabel()
{
	assert(m_firstBlockLabel != -1);
	return m_firstBlockLabel;
}

Jitter::CJitter::LABEL CMipsJitter::GetLastBlockLabel()
{
	if(m_lastBlockLabel == -1)
	{
		m_lastBlockLabel = CreateLabel();
	}
	return m_lastBlockLabel;
}

void CMipsJitter::MarkFirstBlockLabel()
{
	m_firstBlockLabel = CreateLabel();
	MarkLabel(m_firstBlockLabel);
}

void CMipsJitter::MarkLastBlockLabel()
{
	if(m_lastBlockLabel != -1)
	{
		MarkLabel(m_lastBlockLabel);
	}
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
	auto statusIterator(m_variableStatus.find(variableId));
	return statusIterator == m_variableStatus.end() ? nullptr : &statusIterator->second;
}

void CMipsJitter::SetVariableStatus(size_t variableId, const VARIABLESTATUS& status)
{
	assert(GetVariableStatus(variableId) == NULL);
	m_variableStatus[variableId] = status;
}
