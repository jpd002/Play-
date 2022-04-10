#pragma once

#include "Config.h"
#include "Singleton.h"

class CDebugSupportSettings : public CSingleton<CDebugSupportSettings>
{
public:
	void Initialize(Framework::CConfig*);

	std::string GetMonospaceFontFaceName();
	int GetMonospaceFontSize();

private:
	void RegisterPreferences();

	Framework::CConfig* m_config = nullptr;
};
