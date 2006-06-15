#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "List.h"
#include "Str.h"
#include "Semaphore.h"
#include "xml/Node.h"

class CConfig
{
public:
	static CConfig*						GetInstance();

	void								RegisterPreferenceInteger(const char*, int);
	void								RegisterPreferenceBoolean(const char*, bool);
	void								RegisterPreferenceString(const char*, const char*);

	int									GetPreferenceInteger(const char*);
	bool								GetPreferenceBoolean(const char*);
	const char*							GetPreferenceString(const char*);

	bool								SetPreferenceInteger(const char*, int);
	bool								SetPreferenceBoolean(const char*, bool);
	bool								SetPreferenceString(const char*, const char*);

	void								Save();

private:
	enum PREFERENCE_TYPE
	{
		TYPE_INTEGER,
		TYPE_BOOLEAN,
		TYPE_STRING,
	};

	class CPreference
	{
	public:
										CPreference(const char*, PREFERENCE_TYPE);
		virtual							~CPreference();
		const char*						GetName();
		PREFERENCE_TYPE					GetType();
		const char*						GetTypeString();
		virtual	void					Serialize(Framework::Xml::CNode*);

	private:
		Framework::CStrA				m_sName;
		PREFERENCE_TYPE					m_nType;
	};

	class CPreferenceInteger : public CPreference
	{
	public:
										CPreferenceInteger(const char*, int);
		virtual							~CPreferenceInteger();
		int								GetValue();
		void							SetValue(int);
		virtual void					Serialize(Framework::Xml::CNode*);

	private:
		int								m_nValue;
	};

	class CPreferenceBoolean : public CPreference
	{
	public:
										CPreferenceBoolean(const char*, bool);
		virtual							~CPreferenceBoolean();
		bool							GetValue();
		void							SetValue(bool);
		virtual void					Serialize(Framework::Xml::CNode*);

	private:
		bool							m_nValue;
	};

	class CPreferenceString : public CPreference
	{
	public:
										CPreferenceString(const char*, const char*);
		virtual							~CPreferenceString();
		const char*						GetValue();
		void							SetValue(const char*);
		virtual void					Serialize(Framework::Xml::CNode*);

	private:
		Framework::CStrA				m_sValue;
	};

										CConfig();
										~CConfig();
	void								Load();
	template <typename Type> Type*		FindPreference(const char*);
	template <typename Type> Type*		CastPreference(CPreference*);
	void								InsertPreference(CPreference*);
	static void							DeleteInstance();

	static CConfig*						m_pInstance;

	Framework::CList<CPreference>		m_Preference;
	Framework::CSemaphore				m_Mutex;
};

#endif
