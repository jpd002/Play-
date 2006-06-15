#include <string.h>
#include <stdlib.h>
#include "Config.h"
#include "StdStream.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "xml/Utils.h"

#define CONFIGPATH "config.xml"

using namespace Framework;

CConfig*		CConfig::m_pInstance = NULL;

CConfig::CConfig() :
m_Mutex(1)
{
	Load();
}

CConfig::~CConfig()
{
	Save();
	while(m_Preference.Count() != 0)
	{
		delete m_Preference.Pull();
	}
}

CConfig* CConfig::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new CConfig();
		atexit(DeleteInstance);
	}
	return m_pInstance;
}

void CConfig::DeleteInstance()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
	}
}

template <> CConfig::CPreference* CConfig::CastPreference<CConfig::CPreference>(CPreference* pPreference)
{
	return pPreference;
}

template <> CConfig::CPreferenceInteger* CConfig::CastPreference<CConfig::CPreferenceInteger>(CPreference* pPreference)
{
	if(pPreference->GetType() != TYPE_INTEGER)
	{
		return NULL;
	}
	return (CPreferenceInteger*)pPreference;
}

template <> CConfig::CPreferenceBoolean* CConfig::CastPreference<CConfig::CPreferenceBoolean>(CPreference* pPreference)
{
	if(pPreference->GetType() != TYPE_BOOLEAN)
	{
		return NULL;
	}
	return (CPreferenceBoolean*)pPreference;
}

template <> CConfig::CPreferenceString* CConfig::CastPreference<CConfig::CPreferenceString>(CPreference* pPreference)
{
	if(pPreference->GetType() != TYPE_STRING)
	{
		return NULL;
	}
	return (CPreferenceString*)pPreference;
}

template <typename Type> Type* CConfig::FindPreference(const char* sName)
{
	CList<CPreference>::ITERATOR itPref;
	CPreference* pPref;
	CPreference* pRet;
	Type* pPrefCast;

	pRet = NULL;

	m_Mutex.Wait();

	for(itPref = m_Preference.Begin(); itPref.HasNext(); itPref++)
	{
		pPref = (*itPref);
		if(!strcmp(pPref->GetName(), sName))
		{
			pRet = pPref;
			break;
		}
	}

	m_Mutex.Signal();

	if(pRet == NULL) return NULL;

	pPrefCast = CastPreference<Type>(pRet);

	return pPrefCast;
}

void CConfig::RegisterPreferenceInteger(const char* sName, int nValue)
{
	CPreferenceInteger* pPref;

	if(FindPreference<CPreference>(sName) != NULL)
	{
		return;
	}

	pPref = new CPreferenceInteger(sName, nValue);

	InsertPreference(pPref);
}

void CConfig::RegisterPreferenceBoolean(const char* sName, bool nValue)
{
	CPreferenceBoolean* pPref;

	if(FindPreference<CPreference>(sName) != NULL)
	{
		return;
	}

	pPref = new CPreferenceBoolean(sName, nValue);
	InsertPreference(pPref);
}

void CConfig::RegisterPreferenceString(const char* sName, const char* sValue)
{
	CPreferenceString* pPref;

	if(FindPreference<CPreference>(sName) != NULL)
	{
		return;
	}

	pPref = new CPreferenceString(sName, sValue);
	InsertPreference(pPref);
}

int CConfig::GetPreferenceInteger(const char* sName)
{
	CPreferenceInteger* pPref;

	pPref = FindPreference<CPreferenceInteger>(sName);
	if(pPref == NULL) return 0;

	return pPref->GetValue();
}

bool CConfig::GetPreferenceBoolean(const char* sName)
{
	CPreferenceBoolean* pPref;

	pPref = FindPreference<CPreferenceBoolean>(sName);
	if(pPref == NULL) return false;

	return pPref->GetValue();
}

const char* CConfig::GetPreferenceString(const char* sName)
{
	CPreferenceString* pPref;

	pPref = FindPreference<CPreferenceString>(sName);
	if(pPref == NULL) return false;

	return pPref->GetValue();
}

bool CConfig::SetPreferenceInteger(const char* sName, int nValue)
{
	CPreferenceInteger* pPref;

	pPref = FindPreference<CPreferenceInteger>(sName);
	if(pPref == NULL) return false;

	pPref->SetValue(nValue);
	return true;
}

bool CConfig::SetPreferenceBoolean(const char* sName, bool nValue)
{
	CPreferenceBoolean* pPref;

	pPref = FindPreference<CPreferenceBoolean>(sName);
	if(pPref == NULL) return false;

	pPref->SetValue(nValue);
	return true;
}

bool CConfig::SetPreferenceString(const char* sName, const char* sValue)
{
	CPreferenceString* pPref;

	pPref = FindPreference<CPreferenceString>(sName);
	if(pPref == NULL) return false;

	pPref->SetValue(sValue);
	return true;
}

void CConfig::Load()
{
	CStdStream* pStream;
	Xml::CNode* pDocument;
	Xml::CNode* pConfig;
	Xml::CNode* pPref;
	CList<Xml::CNode>::ITERATOR itNode;
	const char* sType;
	const char* sName;

	try
	{
		pStream = new CStdStream(fopen(CONFIGPATH, "rb"));
	}
	catch(...)
	{
		return;
	}

	pDocument = Xml::CParser::ParseDocument(pStream);

	delete pStream;

	pConfig = pDocument->Select("Config");
	if(pConfig == NULL)
	{
		delete pDocument;
		return;
	}

	for(itNode = pConfig->GetChildIterator(); itNode.HasNext(); itNode++)
	{
		pPref = (*itNode);
		if(!pPref->IsTag()) continue;
		if(strcmp(pPref->GetText(), "Preference")) continue;

		sType = pPref->GetAttribute("Type");
		sName = pPref->GetAttribute("Name");

		if(sType == NULL) continue;
		if(sName == NULL) continue;

		if(!strcmp(sType, "integer"))
		{
			int nValue;
			if(Xml::GetAttributeIntValue(pPref, "Value", &nValue))
			{
				RegisterPreferenceInteger(sName, nValue);
			}
		}
		else if(!strcmp(sType, "boolean"))
		{
			bool nValue;
			if(Xml::GetAttributeBoolValue(pPref, "Value", &nValue))
			{
				RegisterPreferenceBoolean(sName, nValue);
			}
		}
		else if(!strcmp(sType, "string"))
		{
			const char* sValue;
			if(Xml::GetAttributeStringValue(pPref, "Value", &sValue))
			{
				RegisterPreferenceString(sName, sValue);
			}
		}
	}

	delete pDocument;
}

void CConfig::Save()
{
	CStdStream* pStream;
	Xml::CNode*	pConfig;
	Xml::CNode* pDocument;
	Xml::CNode* pPrefNode;
	CPreference* pPref;
	CList<CPreference>::ITERATOR itPref;

	try
	{
		pStream = new CStdStream(fopen(CONFIGPATH, "wb"));
	}
	catch(...)
	{
		return;
	}

	pConfig = new Xml::CNode("Config", true);

	for(itPref = m_Preference.Begin(); itPref.HasNext(); itPref++)
	{
		pPref = (*itPref);

		pPrefNode = new Xml::CNode("Preference", true);
		pPref->Serialize(pPrefNode);

		pConfig->InsertNode(pPrefNode);
	}

	pDocument = new Xml::CNode;
	pDocument->InsertNode(pConfig);

	Xml::CWriter::WriteDocument(pStream, pDocument);

	delete pDocument;
	delete pStream;
}

void CConfig::InsertPreference(CPreference* pPref)
{
	m_Mutex.Wait();
	m_Preference.Insert(pPref);
	m_Mutex.Signal();
}

/////////////////////////////////////////////////////////
//CPreference implementation
/////////////////////////////////////////////////////////

CConfig::CPreference::CPreference(const char* sName, PREFERENCE_TYPE nType)
{
	m_sName = sName;
	m_nType = nType;
}

CConfig::CPreference::~CPreference()
{
	
}

const char* CConfig::CPreference::GetName()
{
	return m_sName;
}

CConfig::PREFERENCE_TYPE CConfig::CPreference::GetType()
{
	return m_nType;
}

const char* CConfig::CPreference::GetTypeString()
{
	switch(m_nType)
	{
	case TYPE_INTEGER:
		return "integer";
		break;
	case TYPE_STRING:
		return "string";
		break;
	case TYPE_BOOLEAN:
		return "boolean";
		break;
	}
	return "";
}

void CConfig::CPreference::Serialize(Xml::CNode* pNode)
{
	pNode->InsertAttribute(Xml::CreateAttributeStringValue("Name", m_sName));
	pNode->InsertAttribute(Xml::CreateAttributeStringValue("Type", GetTypeString()));
}

/////////////////////////////////////////////////////////
//CPreferenceInteger implementation
/////////////////////////////////////////////////////////

CConfig::CPreferenceInteger::CPreferenceInteger(const char* sName, int nValue) :
CPreference(sName, TYPE_INTEGER)
{
	m_nValue = nValue;
}

CConfig::CPreferenceInteger::~CPreferenceInteger()
{

}

int CConfig::CPreferenceInteger::GetValue()
{
	return m_nValue;
}

void CConfig::CPreferenceInteger::SetValue(int nValue)
{
	m_nValue = nValue;
}

void CConfig::CPreferenceInteger::Serialize(Xml::CNode* pNode)
{
	CPreference::Serialize(pNode);

	pNode->InsertAttribute(Xml::CreateAttributeIntValue("Value", m_nValue));
}

/////////////////////////////////////////////////////////
//CPreferenceBoolean implementation
/////////////////////////////////////////////////////////

CConfig::CPreferenceBoolean::CPreferenceBoolean(const char* sName, bool nValue) :
CPreference(sName, TYPE_BOOLEAN)
{
	m_nValue = nValue;
}

CConfig::CPreferenceBoolean::~CPreferenceBoolean()
{

}

bool CConfig::CPreferenceBoolean::GetValue()
{
	return m_nValue;
}

void CConfig::CPreferenceBoolean::SetValue(bool nValue)
{
	m_nValue = nValue;
}

void CConfig::CPreferenceBoolean::Serialize(Xml::CNode* pNode)
{
	CPreference::Serialize(pNode);

	pNode->InsertAttribute(Xml::CreateAttributeBoolValue("Value", m_nValue));
}

/////////////////////////////////////////////////////////
//CPreferenceString implementation
/////////////////////////////////////////////////////////

CConfig::CPreferenceString::CPreferenceString(const char* sName, const char* sValue) :
CPreference(sName, TYPE_STRING)
{
	m_sValue = sValue;
}

CConfig::CPreferenceString::~CPreferenceString()
{

}

const char* CConfig::CPreferenceString::GetValue()
{
	return m_sValue;
}

void CConfig::CPreferenceString::SetValue(const char* sValue)
{
	m_sValue = sValue;
}

void CConfig::CPreferenceString::Serialize(Xml::CNode* pNode)
{
	CPreference::Serialize(pNode);

	pNode->InsertAttribute(Xml::CreateAttributeStringValue("Value", m_sValue));
}
