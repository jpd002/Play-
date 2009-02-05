#include "StructCollectionStateFile.h"
#include "xml/Node.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "lexical_cast_ex.h"

#define STRUCT_DOCUMENT_HEADER      ("StructCollection")
#define STRUCT_DOCUMENT_DETAIL      ("Struct")
#define STRUCT_DOCUMENT_DETAIL_NAME ("Name")

using namespace Framework;
using namespace std;

CStructCollectionStateFile::CStructCollectionStateFile(const char* name) :
CZipFile(name)
{

}

CStructCollectionStateFile::CStructCollectionStateFile(CStream& stream) :
CZipFile("")
{
    Read(stream);
}

CStructCollectionStateFile::~CStructCollectionStateFile()
{

}

CStructCollectionStateFile::StructIterator CStructCollectionStateFile::GetStructBegin() const
{
    return m_structs.begin();
}

CStructCollectionStateFile::StructIterator CStructCollectionStateFile::GetStructEnd() const
{
    return m_structs.end();
}

void CStructCollectionStateFile::InsertStruct(const char* name, const CStructFile& structFile)
{
    m_structs[name] = structFile;
}

void CStructCollectionStateFile::Read(CStream& stream)
{
    m_structs.clear();
    Xml::CNode* rootNode = Xml::CParser::ParseDocument(&stream);
    Xml::CNode::NodeList registerList = rootNode->SelectNodes((string(STRUCT_DOCUMENT_HEADER) + "/" + string(STRUCT_DOCUMENT_DETAIL)).c_str());
    for(Xml::CNode::NodeIterator nodeIterator(registerList.begin());
        nodeIterator != registerList.end(); nodeIterator++)
    {
        try
        {
            Xml::CNode* node(*nodeIterator);
            const char* namePtr = node->GetAttribute(STRUCT_DOCUMENT_DETAIL_NAME);
            if(namePtr == NULL) continue;
            CStructFile structFile;
            structFile.Read(node);
            m_structs[namePtr] = structFile;
        }
        catch(...)
        {

        }
    }
    delete rootNode;
}

void CStructCollectionStateFile::Write(CStream& stream)
{
    Xml::CNode* rootNode = new Xml::CNode(STRUCT_DOCUMENT_HEADER, true);
    for(StructMap::const_iterator structIterator(m_structs.begin());
        structIterator != m_structs.end(); structIterator++)
    {
        const CStructFile& structFile(structIterator->second);
        Xml::CNode* structNode = new Xml::CNode(STRUCT_DOCUMENT_DETAIL, true);
        structNode->InsertAttribute(STRUCT_DOCUMENT_DETAIL_NAME, structIterator->first.c_str());
        structFile.Write(structNode);
        rootNode->InsertNode(structNode);
    }
    Xml::CWriter::WriteDocument(&stream, rootNode);
    delete rootNode;
}
