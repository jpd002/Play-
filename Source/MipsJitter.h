#pragma once

#include <map>
#include "Jitter.h"

class CMipsJitter : public Jitter::CJitter
{
public:
	CMipsJitter(Jitter::CCodeGen*);
	virtual ~CMipsJitter() = default;

	void Begin() override;
	void End() override;
	void PushRel(size_t) override;
	void PushRel64(size_t) override;

	void SetVariableAsConstant(size_t, uint32);
	LABEL GetFinalBlockLabel();

private:
	struct VARIABLESTATUS
	{
		uint32 operandType;
		uint32 operandValue;
	};

	typedef std::map<size_t, VARIABLESTATUS> VariableStatusMap;

	VARIABLESTATUS* GetVariableStatus(size_t);
	void SetVariableStatus(size_t, const VARIABLESTATUS&);

	VariableStatusMap m_variableStatus;
	LABEL m_lastBlockLabel;
};
