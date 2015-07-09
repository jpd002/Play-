#pragma once

#include "Singleton.h"

class CSettingsManager : public CSingleton<CSettingsManager>
{
public:
	void			Save();

	void			RegisterPreferenceBoolean(const std::string&, bool);
	bool			GetPreferenceBoolean(const std::string&);
	void			SetPreferenceBoolean(const std::string&, bool);
	
private:

};
