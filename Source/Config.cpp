#include <string.h>
#include <stdlib.h>
#ifdef MACOSX
#include <pwd.h>
#endif
#ifdef WIN32
#include <shlobj.h>
#include <boost/filesystem/operations.hpp>
#endif
#include "Config.h"
#include "StdStream.h"
#include "xml/Writer.h"
#include "xml/Parser.h"
#include "xml/Utils.h"
#include "xml/FilteringNodeIterator.h"

#define DEFAULT_CONFIG_PATH "config.xml"

using namespace Framework;
using namespace std;
using namespace boost;

CConfig::CConfig()
{
	Load();
}

CConfig::~CConfig()
{
    Save();
    for(PreferenceMapType::iterator preferenceIterator(m_preferences.begin());
        preferenceIterator != m_preferences.end(); preferenceIterator++)
    {
        delete preferenceIterator->second;
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
    CPreference* pRet = NULL;

    {
        mutex::scoped_lock mutexLock(m_mutex);
        PreferenceMapType::iterator preferenceIterator(m_preferences.find(sName));
        if(preferenceIterator != m_preferences.end())
        {
            pRet = preferenceIterator->second;
        }
    }

    if(pRet == NULL) return NULL;

    Type* pPrefCast = CastPreference<Type>(pRet);
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

string CConfig::GetConfigPath() const
{
#if defined(MACOSX)
	passwd* userInfo = getpwuid(getuid());
	if(userInfo == NULL) return DEFAULT_CONFIG_PATH;
	return string(userInfo->pw_dir) + "/Library/Preferences/com.vapps.Purei.xml";
#elif defined(WIN32)
    char userPathString[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, userPathString);
    filesystem::path userPath(userPathString, filesystem::native);
    filesystem::path companyPath = userPath / "Virtual Applications";
    filesystem::path productPath = companyPath / "Purei";
    try
    {
        filesystem::create_directory(companyPath);
        filesystem::create_directory(productPath);
    }
    catch(...)
    {
        //Creation failed (maybe because it already exists)
    }
    return (productPath / "Config.xml").string();
#else
	return DEFAULT_CONFIG_PATH;
#endif
}

void CConfig::Load()
{
    Xml::CNode* pDocument;

    try
    {
        CStdStream configFile(fopen(GetConfigPath().c_str(), "rb"));
        pDocument = Xml::CParser::ParseDocument(&configFile);
    }
    catch(...)
    {
        return;
    }

    Xml::CNode* pConfig = pDocument->Select("Config");
    if(pConfig == NULL)
    {
        delete pDocument;
        return;
    }

    for(Xml::CFilteringNodeIterator itNode(pConfig, "Preference"); !itNode.IsEnd(); itNode++)
    {
        Xml::CNode* pPref = (*itNode);

        const char* sType = pPref->GetAttribute("Type");
        const char* sName = pPref->GetAttribute("Name");

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
    try
    {
        CStdStream stream(fopen(GetConfigPath().c_str(), "wb"));

        Xml::CNode*	pConfig = new Xml::CNode("Config", true);

        for(PreferenceMapType::const_iterator preferenceIterator(m_preferences.begin());
            preferenceIterator != m_preferences.end(); preferenceIterator++)
        {
            CPreference* pPref = (preferenceIterator->second);

            Xml::CNode* pPrefNode = new Xml::CNode("Preference", true);
            pPref->Serialize(pPrefNode);

            pConfig->InsertNode(pPrefNode);
        }

        {
            Xml::CNode* pDocument = new Xml::CNode;
            pDocument->InsertNode(pConfig);

            Xml::CWriter::WriteDocument(&stream, pDocument);

            delete pDocument;
        }
    }
    catch(...)
    {
        return;
    }
}

void CConfig::InsertPreference(CPreference* pPref)
{
    mutex::scoped_lock mutexLock(m_mutex);
    m_preferences[pPref->GetName()] = pPref;
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
	return m_sName.c_str();
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
	pNode->InsertAttribute(Xml::CreateAttributeStringValue("Name", m_sName.c_str()));
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
	return m_sValue.c_str();
}

void CConfig::CPreferenceString::SetValue(const char* sValue)
{
	m_sValue = sValue;
}

void CConfig::CPreferenceString::Serialize(Xml::CNode* pNode)
{
	CPreference::Serialize(pNode);

	pNode->InsertAttribute(Xml::CreateAttributeStringValue("Value", m_sValue.c_str()));
}
