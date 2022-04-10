#include "DebugSupportSettings.h"
#include <cassert>

#define PREF_DEBUGGER_MONOSPACE_FONT_FACE_NAME ("debugger.monospacefont.facename")
#define PREF_DEBUGGER_MONOSPACE_FONT_SIZE ("debugger.monospacefont.size")

void CDebugSupportSettings::Initialize(Framework::CConfig* config)
{
	//Needs to be initialized only once
	assert(!m_config);
	m_config = config;

	RegisterPreferences();
}

void CDebugSupportSettings::RegisterPreferences()
{
	m_config->RegisterPreferenceString(PREF_DEBUGGER_MONOSPACE_FONT_FACE_NAME, "Courier New");
	m_config->RegisterPreferenceInteger(PREF_DEBUGGER_MONOSPACE_FONT_SIZE, 8);
}

std::string CDebugSupportSettings::GetMonospaceFontFaceName()
{
	return m_config->GetPreferenceString(PREF_DEBUGGER_MONOSPACE_FONT_FACE_NAME);
}

int CDebugSupportSettings::GetMonospaceFontSize()
{
	return m_config->GetPreferenceInteger(PREF_DEBUGGER_MONOSPACE_FONT_SIZE);
}
