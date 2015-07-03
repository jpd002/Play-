#include <string.h>
#include <boost/lexical_cast.hpp>
#include "string_format.h"
#include "InputManager.h"
#include "win32/GuidUtils.h"

#define CONFIG_PREFIX						("input")
#define CONFIG_BINDING_TYPE					("bindingtype")

#define CONFIG_SIMPLEBINDING_PREFIX			("simplebinding")

#define CONFIG_BINDINGINFO_DEVICE			("device")
#define CONFIG_BINDINGINFO_ID				("id")

#define CONFIG_POVHATBINDING_PREFIX			("povhatbinding")
#define CONFIG_POVHATBINDING_REFVALUE		("refvalue")

#define CONFIG_SIMULATEDAXISBINDING_PREFIX	("simulatedaxisbinding")
#define CONFIG_SIMULATEDAXISBINDING_KEY1	("key1")
#define CONFIG_SIMULATEDAXISBINDING_KEY2	("key2")

using namespace PH_DirectInput;

uint32 CInputManager::m_buttonDefaultValue[PS2::CControllerInfo::MAX_BUTTONS] =
{
	0x7FFF,
	0x7FFF,
	0x7FFF,
	0x7FFF,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
};

CInputManager::CInputManager(HWND hWnd, Framework::CConfig& config)
: m_config(config)
, m_directInputManager(new Framework::DirectInput::CManager())
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		m_config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), 0);
		CSimpleBinding::RegisterPreferences(m_config, prefBase.c_str());
		CPovHatBinding::RegisterPreferences(m_config, prefBase.c_str());
		if(PS2::CControllerInfo::IsAxis(static_cast<PS2::CControllerInfo::BUTTON>(i)))
		{
			CSimulatedAxisBinding::RegisterPreferences(m_config, prefBase.c_str());
		}
	}
	Load();

	m_directInputManager->RegisterInputEventHandler(std::bind(&CInputManager::OnInputEventReceived, 
		this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_directInputManager->CreateKeyboard(hWnd);
	m_directInputManager->CreateJoysticks(hWnd);
}

CInputManager::~CInputManager()
{
	delete m_directInputManager;
}

void CInputManager::Load()
{
	bool hasBindings = false;
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BINDINGTYPE bindingType = BINDING_UNBOUND;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		bindingType = static_cast<BINDINGTYPE>(m_config.GetPreferenceInteger((prefBase + "." + std::string(CONFIG_BINDING_TYPE)).c_str()));
		if(bindingType == BINDING_UNBOUND) continue;
		BindingPtr binding;
		switch(bindingType)
		{
		case BINDING_SIMPLE:
			binding.reset(new CSimpleBinding());
			break;
		case BINDING_POVHAT:
			binding.reset(new CPovHatBinding());
			break;
		case BINDING_SIMULATEDAXIS:
			binding.reset(new CSimulatedAxisBinding());
			break;
		}
		if(binding)
		{
			binding->Load(m_config, prefBase.c_str());
		}
		m_bindings[i] = binding;
		hasBindings = true;
	}
	if(!hasBindings)
	{
		AutoConfigureKeyboard();
	}
	ResetBindingValues();
}

void CInputManager::Save()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BindingPtr& binding = m_bindings[i];
		if(binding == NULL) continue;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		m_config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), binding->GetBindingType());
		binding->Save(m_config, prefBase.c_str());
	}
}

void CInputManager::AutoConfigureKeyboard()
{
	SetSimpleBinding(PS2::CControllerInfo::START,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_RETURN));
	SetSimpleBinding(PS2::CControllerInfo::SELECT,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_LSHIFT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_LEFT,	CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_LEFT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_RIGHT,	CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_RIGHT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_UP,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_UP));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_DOWN,	CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_DOWN));
	SetSimpleBinding(PS2::CControllerInfo::SQUARE,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_A));
	SetSimpleBinding(PS2::CControllerInfo::CROSS,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_Z));
	SetSimpleBinding(PS2::CControllerInfo::TRIANGLE,	CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_S));
	SetSimpleBinding(PS2::CControllerInfo::CIRCLE,		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_X));
	SetSimpleBinding(PS2::CControllerInfo::L1,			CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_1));
	SetSimpleBinding(PS2::CControllerInfo::L2,			CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_2));
	SetSimpleBinding(PS2::CControllerInfo::R1,			CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_9));
	SetSimpleBinding(PS2::CControllerInfo::R2,			CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_0));

	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_LEFT_X,
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_F),
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_H));
	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_LEFT_Y,
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_T),
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_G));

	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_RIGHT_X,
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_J),
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_L));
	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_RIGHT_Y,
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_I),
		CInputManager::BINDINGINFO(GUID_SysKeyboard, DIK_K));
}

const CInputManager::CBinding* CInputManager::GetBinding(PS2::CControllerInfo::BUTTON button) const
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	return m_bindings[button].get();
}

uint32 CInputManager::GetBindingValue(PS2::CControllerInfo::BUTTON button) const
{
	assert(button < PS2::CControllerInfo::MAX_BUTTONS);
	const auto& binding = m_bindings[button];
	if(binding)
	{
		return binding->GetValue();
	}
	else
	{
		return m_buttonDefaultValue[button];
	}
}

void CInputManager::ResetBindingValues()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		auto binding = m_bindings[i];
		if(!binding) continue;
		binding->SetValue(m_buttonDefaultValue[i]);
	}
}

void CInputManager::SetSimpleBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimpleBinding(binding.device, binding.id));
}

void CInputManager::SetPovHatBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding, uint32 refValue)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CPovHatBinding(binding.device, binding.id, refValue));
}

void CInputManager::SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding1, const BINDINGINFO& binding2)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimulatedAxisBinding(binding1, binding2));
}

void CInputManager::OnInputEventReceived(const GUID& device, uint32 id, uint32 value)
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		auto binding = m_bindings[i];
		if(!binding) continue;
		binding->ProcessEvent(device, id, value);
	}
}

std::tstring CInputManager::GetBindingInfoDescription(Framework::DirectInput::CManager* directInputManager, const BINDINGINFO& binding)
{
	DIDEVICEINSTANCE deviceInstance;
	DIDEVICEOBJECTINSTANCE objectInstance;
	if(!directInputManager->GetDeviceInfo(binding.device, &deviceInstance))
	{
		return _T("");
	}
	if(!directInputManager->GetDeviceObjectInfo(binding.device, binding.id, &objectInstance))
	{
		return _T("");
	}
	return std::tstring(deviceInstance.tszInstanceName) + _T(": ") + std::tstring(objectInstance.tszName);
}

Framework::DirectInput::CManager* CInputManager::GetDirectInputManager() const
{
	return m_directInputManager;
}

std::tstring CInputManager::GetBindingDescription(PS2::CControllerInfo::BUTTON button) const
{
	assert(button < PS2::CControllerInfo::MAX_BUTTONS);
	const auto& binding = m_bindings[button];
	if(binding)
	{
		return binding->GetDescription(m_directInputManager);
	}
	else
	{
		return _T("");
	}
}

////////////////////////////////////////////////
// SimpleBinding
////////////////////////////////////////////////

CInputManager::CSimpleBinding::CSimpleBinding(const GUID& device, uint32 id) 
: m_binding(device, id)
{

}

CInputManager::CSimpleBinding::~CSimpleBinding()
{

}

CInputManager::BINDINGTYPE CInputManager::CSimpleBinding::GetBindingType() const
{
	return BINDING_SIMPLE;
}

void CInputManager::CSimpleBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(m_binding.device).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), m_binding.id);
}

void CInputManager::CSimpleBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	m_binding.device = boost::lexical_cast<GUID>(config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	m_binding.id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
}

std::tstring CInputManager::CSimpleBinding::GetDescription(Framework::DirectInput::CManager* directInputManager) const
{
	return GetBindingInfoDescription(directInputManager, m_binding);
}

void CInputManager::CSimpleBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value)
{
	if(id != m_binding.id) return;
	if(device != m_binding.device) return;
	m_value = value;
}

uint32 CInputManager::CSimpleBinding::GetValue() const
{
	return m_value;
}

void CInputManager::CSimpleBinding::SetValue(uint32 value)
{
	m_value = value;
}

void CInputManager::CSimpleBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(GUID()).c_str());
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
}

////////////////////////////////////////////////
// PovHatBinding
////////////////////////////////////////////////

CInputManager::CPovHatBinding::CPovHatBinding(const GUID& device, uint32 id, uint32 refValue) 
: m_binding(device, id)
, m_refValue(refValue)
{

}

CInputManager::CPovHatBinding::~CPovHatBinding()
{

}

CInputManager::BINDINGTYPE CInputManager::CPovHatBinding::GetBindingType() const
{
	return BINDING_POVHAT;
}

void CInputManager::CPovHatBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(m_binding.device).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), m_binding.id);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), m_refValue);
}

void CInputManager::CPovHatBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	m_binding.device = boost::lexical_cast<GUID>(config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	m_binding.id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
	m_refValue = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str());
}

std::tstring CInputManager::CPovHatBinding::GetDescription(Framework::DirectInput::CManager* directInputManager) const
{
	return GetBindingInfoDescription(directInputManager, m_binding);
}

void CInputManager::CPovHatBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value)
{
	if(id != m_binding.id) return;
	if(device != m_binding.device) return;
	m_value = value;
}

uint32 CInputManager::CPovHatBinding::GetValue() const
{
	if(m_value == -1) return 0;
	int32 normalizedRefValue = m_refValue / 100;
	int32 normalizedValue = m_value / 100;
	if(GetShortestDistanceBetweenAngles(normalizedValue, normalizedRefValue) <= 45)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void CInputManager::CPovHatBinding::SetValue(uint32 value)
{
	m_value = value ? m_refValue : -1;
}

int32 CInputManager::CPovHatBinding::GetShortestDistanceBetweenAngles(int32 angle1, int32 angle2)
{
	if(angle1 >  180) angle1 -= 360;
	if(angle1 < -180) angle1 += 360;
	if(angle2 >  180) angle2 -= 360;
	if(angle2 < -180) angle2 += 360;
	int32 angle = abs(angle1 - angle2);
	if(angle > 180)
	{
		angle = 360 - angle;
	}
	return angle;
}

void CInputManager::CPovHatBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(GUID()).c_str());
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), -1);
}

////////////////////////////////////////////////
// SimulatedAxisBinding
////////////////////////////////////////////////

CInputManager::CSimulatedAxisBinding::CSimulatedAxisBinding(const BINDINGINFO& binding1, const BINDINGINFO& binding2)
: m_key1Binding(binding1)
, m_key2Binding(binding2)
{

}

CInputManager::CSimulatedAxisBinding::~CSimulatedAxisBinding()
{

}

void CInputManager::CSimulatedAxisBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	RegisterKeyBindingPreferences(config, key1PrefBase.c_str());
	RegisterKeyBindingPreferences(config, key2PrefBase.c_str());
}

void CInputManager::CSimulatedAxisBinding::RegisterKeyBindingPreferences(Framework::CConfig& config, const char* prefBase)
{
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(GUID()).c_str());
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
}

CInputManager::BINDINGTYPE CInputManager::CSimulatedAxisBinding::GetBindingType() const
{
	return BINDING_SIMULATEDAXIS;
}

void CInputManager::CSimulatedAxisBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	SaveKeyBinding(config, key1PrefBase.c_str(), m_key1Binding);
	SaveKeyBinding(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputManager::CSimulatedAxisBinding::SaveKeyBinding(Framework::CConfig& config, const char* prefBase, const BINDINGINFO& binding) const
{
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), boost::lexical_cast<std::string>(binding.device).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), binding.id);
}

void CInputManager::CSimulatedAxisBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	LoadKeyBinding(config, key1PrefBase.c_str(), m_key1Binding);
	LoadKeyBinding(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputManager::CSimulatedAxisBinding::LoadKeyBinding(Framework::CConfig& config, const char* prefBase, BINDINGINFO& binding)
{
	binding.device = boost::lexical_cast<GUID>(config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	binding.id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
}

std::tstring CInputManager::CSimulatedAxisBinding::GetDescription(Framework::DirectInput::CManager* directInputManager) const
{
	auto key1BindingDesc = GetBindingInfoDescription(directInputManager, m_key1Binding);
	auto key2BindingDesc = GetBindingInfoDescription(directInputManager, m_key2Binding);
	return string_format(_T("Simulated (%s / %s)"), key1BindingDesc.c_str(), key2BindingDesc.c_str());
}

uint32 CInputManager::CSimulatedAxisBinding::GetValue() const
{
	uint32 value = 0x7FFF;
	if(m_key1State && m_key2State)
	{
		value = 0x7FFF;
	}
	if(m_key1State)
	{
		value = 0;
	}
	else if(m_key2State)
	{
		value = 0xFFFF;
	}
	return value;
}

void CInputManager::CSimulatedAxisBinding::SetValue(uint32 value)
{
	m_key1State = 0;
	m_key2State = 0;
}

void CInputManager::CSimulatedAxisBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value)
{
	if(id == m_key1Binding.id && device == m_key1Binding.device)
	{
		m_key1State = value;
	}
	else if(id == m_key2Binding.id && device == m_key2Binding.device)
	{
		m_key2State = value;
	}
}
