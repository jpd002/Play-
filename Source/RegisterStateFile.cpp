#include "RegisterStateFile.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

using namespace Framework;
using namespace std;

CRegisterStateFile::CRegisterStateFile(const char* name) :
CZipFile(name)
{
    
}

CRegisterStateFile::CRegisterStateFile(CStream& stream) :
CZipFile("")
{
    Read(stream);
}

CRegisterStateFile::~CRegisterStateFile()
{

}

void CRegisterStateFile::Read(CStream& stream)
{
    m_registers.clear();
    Xml::CNode* rootNode = Xml::CParser::ParseDocument(&stream);
    Xml::CNode::NodeList registerList = rootNode->SelectNodes("RegisterFile/Register");
    for(Xml::CNode::NodeIterator nodeIterator(registerList.begin());
        nodeIterator != registerList.end(); nodeIterator++)
    {
        try
        {
            Xml::CNode* node(*nodeIterator);
            const char* namePtr = node->GetAttribute("Name");
            const char* valuePtr = node->GetAttribute("Value");
            if(namePtr == NULL || valuePtr == NULL) continue;
            string valueString(valuePtr);
            uint128 value;
            memset(&value, 0, sizeof(uint128));
            for(unsigned int i = 0; i < 4; i++)
            {
                if(valueString.length() == 0) break;
                int start = max<int>(0, static_cast<int>(valueString.length()) - 8);
                string subString(valueString.begin() + start, valueString.end());
                value.nV[i] = lexical_cast_hex<string>(subString);
                valueString = string(valueString.begin(), valueString.begin() + start);
            }
            m_registers[namePtr] = Register(4, value);
        }
        catch(...)
        {

        }
    }
    delete rootNode;
}

void CRegisterStateFile::Write(CStream& stream)
{
    Xml::CNode* rootNode = new Xml::CNode("RegisterFile", true);
    for(RegisterList::const_iterator registerIterator(m_registers.begin());
        registerIterator != m_registers.end(); registerIterator++)
    {
        const Register& reg(registerIterator->second);
        Xml::CNode* registerNode = new Xml::CNode("Register", true);
        string valueString;
        for(unsigned int i = 0; i < reg.first; i++)
        {
            valueString = lexical_cast_hex<string>(reg.second.nV[i], 8) + valueString;
        }
        registerNode->InsertAttribute("Name", registerIterator->first.c_str());
        registerNode->InsertAttribute("Value", valueString.c_str());
        rootNode->InsertNode(registerNode);
    }
    Xml::CWriter::WriteDocument(&stream, rootNode);
    delete rootNode;
}

void CRegisterStateFile::SetRegister32(const char* name, uint32 value)
{
    uint128 longValue;
    longValue.nD0 = value;
    longValue.nD1 = 0;
    m_registers[name] = Register(1, longValue);
}

void CRegisterStateFile::SetRegister64(const char* name, uint64 value)
{
    uint128 longValue;
    longValue.nD0 = value;
    longValue.nD1 = 0;
    m_registers[name] = Register(2, longValue);
}

uint32 CRegisterStateFile::GetRegister32(const char* name)
{
    RegisterList::const_iterator registerIterator(m_registers.find(name));
    if(registerIterator == m_registers.end()) return 0;
    return registerIterator->second.second.nV0;
}

uint64 CRegisterStateFile::GetRegister64(const char* name)
{
    RegisterList::const_iterator registerIterator(m_registers.find(name));
    if(registerIterator == m_registers.end()) return 0;
    return registerIterator->second.second.nD0;
}
