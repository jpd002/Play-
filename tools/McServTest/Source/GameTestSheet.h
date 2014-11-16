#pragma once

#include <vector>
#include <unordered_map>
#include "Stream.h"

class CGameTestSheet
{
public:
	enum ENVIRONMENT_ACTION_TYPE
	{
		ENVIRONMENT_ACTION_NONE,
		ENVIRONMENT_ACTION_CREATE_DIRECTORY,
		ENVIRONMENT_ACTION_CREATE_FILE,
	};

	struct ENVIRONMENT_ACTION
	{
		ENVIRONMENT_ACTION_TYPE		type = ENVIRONMENT_ACTION_NONE;
		std::string					name;
		uint32						size = 0;
	};
	typedef std::vector<ENVIRONMENT_ACTION> EnvironmentActionArray;
	typedef EnvironmentActionArray ENVIRONMENT;
	typedef std::unordered_map<uint32, ENVIRONMENT> EnvironmentMap;

	typedef std::vector<std::string> EntryArray;

	struct TEST
	{
		std::string		query;
		uint32			environmentId = 0;
		int32			maxEntries = 0;
		int32			result = 0;
		std::string		currentDirectory;
		EntryArray		entries;
	};
	typedef std::vector<TEST> TestArray;

							CGameTestSheet();
							CGameTestSheet(Framework::CStream&);
	virtual					~CGameTestSheet();

	ENVIRONMENT				GetEnvironment(uint32) const;
	const TestArray&		GetTests() const;

private:
	void					ParseSheet(Framework::CStream&);

	EnvironmentMap			m_environments;
	TestArray				m_tests;
};
