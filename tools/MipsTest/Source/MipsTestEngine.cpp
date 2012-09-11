#include <stdexcept>
#include "MipsTestEngine.h"
#include "StdStream.h"
#include "lexical_cast_ex.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "xml/Utils.h"

CMipsTestEngine::CMipsTestEngine(const char* sPath)
{
	auto pRootNode = std::unique_ptr<Framework::Xml::CNode>(
		Framework::Xml::CParser::ParseDocument(Framework::CStdStream(fopen(sPath, "rb")))
		);

	if(!pRootNode)
	{
		throw std::exception();
	}

	LoadInputs(pRootNode->Select("Test/Inputs"));
	LoadOutputs(pRootNode->Select("Test/Outputs"));
	LoadInstances(pRootNode->Select("Test/Instances"));
}

CMipsTestEngine::~CMipsTestEngine()
{

}

CMipsTestEngine::OutputsType::iterator CMipsTestEngine::GetOutputsBegin()
{
	return m_Outputs.begin();
}

CMipsTestEngine::OutputsType::iterator CMipsTestEngine::GetOutputsEnd()
{
	return m_Outputs.end();
}

CMipsTestEngine::CValueSet* CMipsTestEngine::GetInput(unsigned int nId)
{
	InputsByIdMapType::iterator itInput;
	itInput = m_InputsById.find(nId);
	return (itInput != m_InputsById.end()) ? (itInput->second) : (NULL);
}

CMipsTestEngine::CValueSet* CMipsTestEngine::GetOutput(unsigned int nInputId, unsigned int nInstanceId)
{
	for(OutputsType::iterator itOutput(m_Outputs.begin());
		itOutput != m_Outputs.end(); itOutput++)
	{
		if(
			(itOutput->GetInputId() == nInputId) && 
			(itOutput->GetInstanceId() == nInstanceId)
			)
		{
			return &(*itOutput);
		}
	}

	return NULL;
}

CMipsTestEngine::CInstance* CMipsTestEngine::GetInstance(unsigned int nId)
{
	InstancesByIdMapType::iterator itInstance;
	itInstance = m_InstancesById.find(nId);
	return (itInstance != m_InstancesById.end()) ? (itInstance->second) : (NULL);
}

void CMipsTestEngine::LoadInputs(Framework::Xml::CNode* pInputsNode)
{
	if(pInputsNode == NULL)
	{
		throw std::runtime_error("No 'Inputs' node was found in the test suite definition.");
	}

	for(Framework::Xml::CFilteringNodeIterator itNode(pInputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
	{
		CValueSet* pInput(new CValueSet(*itNode));
		m_Inputs.push_back(pInput);
		m_InputsById[pInput->GetInputId()] = pInput;
	}
}

void CMipsTestEngine::LoadOutputs(Framework::Xml::CNode* pOutputsNode)
{
	if(pOutputsNode == NULL)
	{
		throw std::runtime_error("No 'Outputs' node was found in the test suite definition.");
	}

	for(Framework::Xml::CFilteringNodeIterator itNode(pOutputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
	{
		m_Outputs.push_back(new CValueSet(*itNode));
	}
}

void CMipsTestEngine::LoadInstances(Framework::Xml::CNode* pInstancesNode)
{
	if(pInstancesNode == NULL)
	{
		throw std::runtime_error("No 'Instances' node was found in the test suite definition.");
	}

	for(Framework::Xml::CFilteringNodeIterator itNode(pInstancesNode, "Instance"); !itNode.IsEnd(); itNode++)
	{
		CInstance* pInstance(new CInstance(*itNode));
		m_Instances.push_back(pInstance);
		m_InstancesById[pInstance->GetId()] = pInstance;
	}
}

////////////////////////////////////////////////////
// CValueSet implementation
////////////////////////////////////////////////////

CMipsTestEngine::CValueSet::CValueSet(Framework::Xml::CNode* pValueSetNode)
{
	m_nInputId = 0;
	m_nInstanceId = 0;

	Framework::Xml::GetAttributeIntValue(pValueSetNode, "InputId", reinterpret_cast<int*>(&m_nInputId));
	Framework::Xml::GetAttributeIntValue(pValueSetNode, "InstanceId", reinterpret_cast<int*>(&m_nInstanceId));

	for(Framework::Xml::CNode::NodeIterator itNode(pValueSetNode->GetChildrenBegin());
		itNode != pValueSetNode->GetChildrenEnd(); itNode++)
	{
		Framework::Xml::CNode* pNode(*itNode);
		if(!pNode->IsTag()) continue;

		//Check the value type
		if(!strcmp(pNode->GetText(), "Register"))
		{
			m_Values.push_back(new CRegisterValue(pNode));
		}
		else if(!strcmp(pNode->GetText(), "Memory"))
		{
			m_Values.push_back(new CMemoryValue(pNode));
		}
		else if(!strcmp(pNode->GetText(), "SpecialRegister"))
		{
			m_Values.push_back(new CSpecialRegisterValue(pNode));
		}
		else
		{
			throw std::runtime_error(std::string("Unknown value type '") + pNode->GetText() + std::string("' encountered."));
		}
	}
}

CMipsTestEngine::CValueSet::~CValueSet()
{

}

unsigned int CMipsTestEngine::CValueSet::GetInputId() const
{
	return m_nInputId;
}

unsigned int CMipsTestEngine::CValueSet::GetInstanceId() const
{
	return m_nInstanceId;
}

CMipsTestEngine::CValueSet::ValueIterator CMipsTestEngine::CValueSet::GetValuesBegin() const
{
	return m_Values.begin();
}

CMipsTestEngine::CValueSet::ValueIterator CMipsTestEngine::CValueSet::GetValuesEnd() const
{
	return m_Values.end();
}

void CMipsTestEngine::CValueSet::AssembleLoad(CMIPSAssembler& Assembler)
{
	for(ValueListType::iterator itValue(m_Values.begin());
		itValue != m_Values.end(); itValue++)
	{
		itValue->AssembleLoad(Assembler);
	}
}

bool CMipsTestEngine::CValueSet::Verify(CMIPS& Context)
{
	bool nResult = true;

	for(ValueListType::iterator itValue(m_Values.begin());
		itValue != m_Values.end(); itValue++)
	{
		nResult &= itValue->Verify(Context);
	}

	return nResult;
}

////////////////////////////////////////////////////
// CInstance implementation
////////////////////////////////////////////////////

CMipsTestEngine::CInstance::CInstance(Framework::Xml::CNode* pInstanceNode)
{
	if(!Framework::Xml::GetAttributeIntValue(pInstanceNode, "Id", reinterpret_cast<int*>(&m_nId)))
	{
		throw std::runtime_error("No Id declared for instance.");
	}

	m_sSource = pInstanceNode->GetInnerText();
}

CMipsTestEngine::CInstance::~CInstance()
{

}

unsigned int CMipsTestEngine::CInstance::GetId()
{
	return m_nId;
}

const char* CMipsTestEngine::CInstance::GetSource()
{
	return m_sSource.c_str();
}

////////////////////////////////////////////////////
// CValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CValue::~CValue()
{

}

////////////////////////////////////////////////////
// CRegisterValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CRegisterValue::CRegisterValue(Framework::Xml::CNode* pNode)
{
	const char* sName;
	const char* sValue;
	if(!Framework::Xml::GetAttributeStringValue(pNode, "Name", &sName))
	{
		throw std::runtime_error("RegisterValue: Couldn't find attribute 'Name'.");
	}

	m_nRegister = CMIPSAssembler::GetRegisterIndex(sName);
	if(m_nRegister == -1)
	{
		throw std::runtime_error("RegisterValue: Invalid register name.");
	}

	m_nValue0 = 0;
	m_nValue1 = 0;

	if(Framework::Xml::GetAttributeStringValue(pNode, "Value0", &sValue))
	{
		m_nValue0 = lexical_cast_hex<std::string>(sValue);
	}

	if(Framework::Xml::GetAttributeStringValue(pNode, "Value1", &sValue))
	{
		m_nValue1 = lexical_cast_hex<std::string>(sValue);
	}
}

CMipsTestEngine::CRegisterValue::~CRegisterValue()
{

}

void CMipsTestEngine::CRegisterValue::AssembleLoad(CMIPSAssembler& Assembler)
{
	if((m_nValue0 == 0) && (m_nValue1 == 0))
	{
		Assembler.ADDIU(m_nRegister, 0, 0x0000);
	}
	else
	{
		uint16 nHalf[4];
		uint32 nSignExtension;

		nHalf[0] = static_cast<uint16>((m_nValue0 >>  0) & 0xFFFF);
		nHalf[1] = static_cast<uint16>((m_nValue0 >> 16) & 0xFFFF);
		nHalf[2] = static_cast<uint16>((m_nValue1 >>  0) & 0xFFFF);
		nHalf[3] = static_cast<uint16>((m_nValue1 >> 16) & 0xFFFF);

		nSignExtension = ((m_nValue0 & 0x80000000) == 0) ? (0x00000000) : (0xFFFFFFFF);

		if(m_nValue1 != nSignExtension)
		{
			Assembler.ADDIU(m_nRegister, 0, 0x0000);

			for(int i = 3; i >= 0; i--)
			{
				Assembler.ORI(m_nRegister, m_nRegister, nHalf[i]);
				if(i != 0)
				{
					Assembler.DSLL(m_nRegister, m_nRegister, 16);
				}
			}
		}
		else
		{
			Assembler.LUI(m_nRegister, nHalf[1]);
			Assembler.ORI(m_nRegister, m_nRegister, nHalf[0]);
		}
	}
}

bool CMipsTestEngine::CRegisterValue::Verify(CMIPS& Context)
{
	if(Context.m_State.nGPR[m_nRegister].nV[0] != m_nValue0)
	{
		return false;
	}

	if(Context.m_State.nGPR[m_nRegister].nV[1] != m_nValue1)
	{
		return false;
	}

	return true;
}

std::string CMipsTestEngine::CRegisterValue::GetString() const
{
	return std::string(CMIPS::m_sGPRName[m_nRegister]) + 
		": 0x" + lexical_cast_hex<std::string>(m_nValue1, 8) +
		" 0x" + lexical_cast_hex<std::string>(m_nValue0, 8);
}

////////////////////////////////////////////////////
// CSpecialRegisterValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CSpecialRegisterValue::CSpecialRegisterValue(Framework::Xml::CNode* pNode)
{
	const char* sName;
	const char* sValue;

	if(!Framework::Xml::GetAttributeStringValue(pNode, "Name", &sName))
	{
		throw std::runtime_error("SpecialRegisterValue: Couldn't find attribute 'Name'.");
	}

	if(!strcmp(sName, "LO"))
	{
		m_nRegister = LO;
	}
	else if(!strcmp(sName, "HI"))
	{
		m_nRegister = HI;
	}
	else if(!strcmp(sName, "LO1"))
	{
		m_nRegister = LO1;
	}
	else if(!strcmp(sName, "HI1"))
	{
		m_nRegister = HI1;
	}
	else if(!strcmp(sName, "PC"))
	{
		m_nRegister = PC;
	}
	else if(!strcmp(sName, "DelaySlot"))
	{
		m_nRegister = DELAYSLOT;
	}
	else
	{
		throw std::runtime_error("SpecialRegisterValue: Invalid register specified.");
	}

	m_nValue0 = 0;
	m_nValue1 = 0;

	if(Framework::Xml::GetAttributeStringValue(pNode, "Value0", &sValue))
	{
		m_nValue0 = lexical_cast_hex<std::string>(sValue);
	}

	if(Framework::Xml::GetAttributeStringValue(pNode, "Value1", &sValue))
	{
		m_nValue1 = lexical_cast_hex<std::string>(sValue);
	}
}

CMipsTestEngine::CSpecialRegisterValue::~CSpecialRegisterValue()
{

}

void CMipsTestEngine::CSpecialRegisterValue::AssembleLoad(CMIPSAssembler& Assembler)
{
	throw std::exception();
}

bool CMipsTestEngine::CSpecialRegisterValue::Verify(CMIPS& Context)
{
	switch(m_nRegister)
	{
	case LO:
		return (Context.m_State.nLO[0] == m_nValue0) && (Context.m_State.nLO[1] == m_nValue1);
		break;
	case HI:
		return (Context.m_State.nHI[0] == m_nValue0) && (Context.m_State.nHI[1] == m_nValue1);
		break;
	case DELAYSLOT:
		return (Context.m_State.nDelayedJumpAddr != MIPS_INVALID_PC) == (m_nValue0 != 0);
		break;
	}
	return false;
}

std::string CMipsTestEngine::CSpecialRegisterValue::GetString() const
{
	if(m_nRegister == DELAYSLOT)
	{
		return std::string("DelaySlot: ") + ((m_nValue0 != 0) ? "Yes" : "No");
	}
	else
	{
		const char* sName;

		switch(m_nRegister)
		{
		case LO:
			sName = "LO";
			break;
		case HI:
			sName = "HI";
			break;
		}

		return std::string(sName) + 
			": 0x" + lexical_cast_hex<std::string>(m_nValue1, 8) +
			" 0x" + lexical_cast_hex<std::string>(m_nValue0, 8);
	}
}

////////////////////////////////////////////////////
// CMemoryValue implementation
////////////////////////////////////////////////////

CMipsTestEngine::CMemoryValue::CMemoryValue(Framework::Xml::CNode* pNode)
{
	const char* sValue;

	m_nAddress = 0;
	m_nValue = 0;

	if(Framework::Xml::GetAttributeStringValue(pNode, "Address", &sValue))
	{
		m_nAddress = lexical_cast_hex<std::string>(sValue);
	}

	if(Framework::Xml::GetAttributeStringValue(pNode, "Value", &sValue))
	{
		m_nValue = lexical_cast_hex<std::string>(sValue);
	}
}

CMipsTestEngine::CMemoryValue::~CMemoryValue()
{

}

void CMipsTestEngine::CMemoryValue::AssembleLoad(CMIPSAssembler& Assembler)
{
	//Load value in register
	Assembler.LUI(CMIPS::T8,			static_cast<uint16>(m_nValue >> 16));
	Assembler.ORI(CMIPS::T8, CMIPS::T8,	static_cast<uint16>(m_nValue >>  0));

	//Load the address
	Assembler.LUI(CMIPS::T9,			static_cast<uint16>(m_nAddress >> 16));
	Assembler.ORI(CMIPS::T9, CMIPS::T9,	static_cast<uint16>(m_nAddress >>  0));

	//Store the value
	Assembler.SW(CMIPS::T8, 0, CMIPS::T9);
}

bool CMipsTestEngine::CMemoryValue::Verify(CMIPS& Context)
{
	return false;
}

std::string CMipsTestEngine::CMemoryValue::GetString() const
{
	return std::string("RAM[0x") + lexical_cast_hex<std::string>(m_nAddress, 8)
		+ "] := 0x" + lexical_cast_hex<std::string>(m_nValue, 8);
}
