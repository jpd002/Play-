#include <stdexcept>
#include "MipsTestEngine.h"
#include "StdStream.h"
#include "lexical_cast_ex.h"
#include "xml/Node.h"
#include "xml/Parser.h"
#include "xml/FilteringNodeIterator.h"
#include "xml/Utils.h"

using namespace Framework;
using namespace std;

CMipsTestEngine::CMipsTestEngine(const char* sPath)
{
    Xml::CNode* pRootNode;
    pRootNode = Xml::CParser::ParseDocument(&CStdStream(fopen(sPath, "rb")));

    if(pRootNode == NULL)
    {
        throw exception();
    }

    LoadInputs(pRootNode->Select("Test/Inputs"));
    LoadOutputs(pRootNode->Select("Test/Outputs"));
    LoadInstances(pRootNode->Select("Test/Instances"));

    delete pRootNode;
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

CMipsTestEngine::CInstance* CMipsTestEngine::GetInstance(unsigned int nId)
{
    InstancesByIdMapType::iterator itInstance;
    itInstance = m_InstancesById.find(nId);
    return (itInstance != m_InstancesById.end()) ? (itInstance->second) : (NULL);
}

void CMipsTestEngine::LoadInputs(Xml::CNode* pInputsNode)
{
    if(pInputsNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pInputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
    {
        CValueSet* pInput(new CValueSet(*itNode));
        m_Inputs.push_back(pInput);
        m_InputsById[pInput->GetInputId()] = pInput;
    }
}

void CMipsTestEngine::LoadOutputs(Xml::CNode* pOutputsNode)
{
    if(pOutputsNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pOutputsNode, "ValueSet"); !itNode.IsEnd(); itNode++)
    {
        m_Outputs.push_back(new CValueSet(*itNode));
    }
}

void CMipsTestEngine::LoadInstances(Xml::CNode* pInstancesNode)
{
    if(pInstancesNode == NULL)
    {
        throw exception();
    }

    for(Xml::CFilteringNodeIterator itNode(pInstancesNode, "Instance"); !itNode.IsEnd(); itNode++)
    {
        CInstance* pInstance(new CInstance(*itNode));
        m_Instances.push_back(pInstance);
        m_InstancesById[pInstance->GetId()] = pInstance;
    }
}

////////////////////////////////////////////////////
// CValueSet implementation
////////////////////////////////////////////////////

CMipsTestEngine::CValueSet::CValueSet(Xml::CNode* pValueSetNode)
{
    m_nInputId = 0;
    m_nInstanceId = 0;

    Xml::GetAttributeIntValue(pValueSetNode, "InputId", reinterpret_cast<int*>(&m_nInputId));
    Xml::GetAttributeIntValue(pValueSetNode, "InstanceId", reinterpret_cast<int*>(&m_nInstanceId));

    for(Xml::CNode::NodeIterator itNode(pValueSetNode->GetChildrenBegin());
        itNode != pValueSetNode->GetChildrenEnd(); itNode++)
    {
        Xml::CNode* pNode(*itNode);
        if(!pNode->IsTag()) continue;
        
        //Check the value type
        if(!strcmp(pNode->GetText(), "Register"))
        {
            m_Values.push_back(new CRegisterValue(pNode));
        }
        else
        {
            throw runtime_error(string("Unknown value type '") + pNode->GetText() + string("' encountered."));
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

void CMipsTestEngine::CValueSet::AssembleLoad(CMIPSAssembler& Assembler)
{
    for(ValueListType::iterator itValue(m_Values.begin());
        itValue != m_Values.end(); itValue++)
    {
        itValue->AssembleLoad(Assembler);
    }
}

////////////////////////////////////////////////////
// CInstance implementation
////////////////////////////////////////////////////

CMipsTestEngine::CInstance::CInstance(Xml::CNode* pInstanceNode)
{
    if(!Xml::GetAttributeIntValue(pInstanceNode, "Id", reinterpret_cast<int*>(&m_nId)))
    {
        throw runtime_error("No Id declared for instance.");
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

CMipsTestEngine::CRegisterValue::CRegisterValue(Xml::CNode* pNode)
{
    const char* sName;
    const char* sValue;
    if(!Xml::GetAttributeStringValue(pNode, "Name", &sName))
    {
        throw runtime_error("RegisterValue: Couldn't find attribute 'Name'.");
    }

    m_nRegister = CMIPSAssembler::GetRegisterIndex(sName);
    if(m_nRegister == -1)
    {
        throw runtime_error("RegisterValue: Invalid register name.");
    }

    m_nValue0 = 0;
    m_nValue1 = 0;

    if(Xml::GetAttributeStringValue(pNode, "Value0", &sValue))
    {
        m_nValue0 = lexical_cast_hex<string>(sValue);
    }

    if(Xml::GetAttributeStringValue(pNode, "Value1", &sValue))
    {
        m_nValue1 = lexical_cast_hex<string>(sValue);
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
 
        nHalf[0] = (m_nValue0 >>  0) & 0xFFFF;
        nHalf[1] = (m_nValue0 >> 16) & 0xFFFF;
        nHalf[2] = (m_nValue1 >>  0) & 0xFFFF;
        nHalf[3] = (m_nValue1 >> 16) & 0xFFFF;

        Assembler.LUI(m_nRegister, nHalf[1]);
        Assembler.ORI(m_nRegister, m_nRegister, nHalf[0]);

        nSignExtension = ((m_nValue0 & 0x80000000) == 0) ? (0x00000000) : (0xFFFFFFFF);

        if(m_nValue1 != nSignExtension)
        {
            Assembler.DSRL(m_nRegister, m_nRegister, 4);
            Assembler.ORI(m_nRegister, m_nRegister, nHalf[2]);
            Assembler.DSRL(m_nRegister, m_nRegister, 4);
            Assembler.ORI(m_nRegister, m_nRegister, nHalf[3]);
        }
    }
}

void CMipsTestEngine::CRegisterValue::Verify(CMIPS&)
{

}
