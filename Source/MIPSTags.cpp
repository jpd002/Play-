#include "MIPSTags.h"
#include "PtrMacro.h"
#include "StdStream.h"

using namespace Framework;

CMIPSTags::CMIPSTags()
{

}

CMIPSTags::~CMIPSTags()
{
	RemoveTags();
}

void CMIPSTags::InsertTag(uint32 nAddress, char* sTag)
{
	char* sTemp;

	sTemp = m_Tag.Find(nAddress);
	if(sTemp == NULL)
	{
		//No comment yet
		sTemp = (char*)malloc(strlen(sTag) + 1);
		strcpy(sTemp, sTag);
		m_Tag.Insert(sTemp, nAddress);
		return;
	}
	FREEPTR(sTemp);
	m_Tag.Remove(nAddress);
	if(sTag == NULL) return;
	if(strlen(sTag) == 0) return;

	sTemp = (char*)malloc(strlen(sTag) + 1);
	strcpy(sTemp, sTag);
	m_Tag.Insert(sTemp, nAddress);
}

void CMIPSTags::RemoveTags()
{
	while(m_Tag.Count() != 0)
	{
		free(m_Tag.Pull());
	}
}

char* CMIPSTags::Find(uint32 nAddress)
{
	return m_Tag.Find(nAddress);
}

void CMIPSTags::Serialize(char* sPath)
{
	FILE* pFile;
	CStdStream* pStream;
	uint32 nCount, nKey;
	uint8 nLenght;
	char* sTag;
	CList<char>::ITERATOR itTag;

	pFile = fopen(sPath, "wb");
	if(pFile == NULL) return;
	pStream = new CStdStream(pFile);

	nCount = m_Tag.Count();

	pStream->Write(&nCount, 4);

	for(itTag = m_Tag.Begin(); itTag.HasNext(); itTag++)
	{
		sTag = (*itTag);
		nLenght = (uint8)strlen(sTag);
		nKey = itTag.GetKey();

		pStream->Write(&nKey, 4);
		pStream->Write(&nLenght, 1);
		pStream->Write(sTag, nLenght);
	}

	DELETEPTR(pStream);

}

void CMIPSTags::Unserialize(char* sPath)
{
	FILE* pFile;
	CStdStream* pStream;
	unsigned int i;
	uint32 nCount, nKey;
	uint8 nLenght;
	char sTag[257];

	pFile = fopen(sPath, "rb");
	if(pFile == NULL) return;
	pStream = new CStdStream(pFile);

	RemoveTags();

	pStream->Read(&nCount, 4);

	for(i = 0; i < nCount; i++)
	{
		pStream->Read(&nKey, 4);
		pStream->Read(&nLenght, 1);
		pStream->Read(sTag, nLenght);
		sTag[nLenght] = 0x00;
		InsertTag(nKey, sTag);
	}

	DELETEPTR(pStream);
}
