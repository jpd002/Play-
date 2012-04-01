#include "InputConfig.h"
#include "win32/GuidUtils.h"
#include <string.h>
#include <boost/lexical_cast.hpp>

#define CONFIG_PREFIX				("input")
#define CONFIG_BINDING_TYPE			("bindingtype")

#define CONFIG_SIMPLEBINDING_PREFIX	("simplebinding")

#define CONFIG_BINDINGINFO_DEVICE	("device")
#define CONFIG_BINDINGINFO_ID		("id")

#define CONFIG_SIMULATEDAXISBINDING_PREFIX	("simulatedaxisbinding")
#define CONFIG_SIMULATEDAXISBINDING_KEY1	("key1")
#define CONFIG_SIMULATEDAXISBINDING_KEY2	("key2")

CInputConfig::CInputConfig(CAppConfig& config)
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		config.RegisterPreferenceInteger(
			Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(),
			0);
		CSimpleBinding::RegisterPreferences(config, prefBase.c_str());
	}
	Load();
}

CInputConfig::~CInputConfig()
{

}

void CInputConfig::Load()
{
	bool hasBindings = false;
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BINDINGTYPE bindingType = BINDING_UNBOUND;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		bindingType = static_cast<BINDINGTYPE>(CAppConfig::GetInstance().GetPreferenceInteger((prefBase + "." + std::string(CONFIG_BINDING_TYPE)).c_str()));
		if(bindingType == BINDING_UNBOUND) continue;
		BindingPtr binding;
		switch(bindingType)
		{
		case BINDING_SIMPLE:
			binding.reset(new CSimpleBinding());
			break;
		}
		if(binding)
		{
			binding->Load(CAppConfig::GetInstance(), prefBase.c_str());
		}
		m_bindings[i] = binding;
		hasBindings = true;
	}
	if(!hasBindings)
	{
		AutoConfigureKeyboard();
	}
}

void CInputConfig::Save()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BindingPtr& binding = m_bindings[i];
		if(binding == NULL) continue;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		CAppConfig::GetInstance().SetPreferenceInteger(
			Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), 
			binding->GetBindingType());
		binding->Save(CAppConfig::GetInstance(), prefBase.c_str());
	}
}

void CInputConfig::AutoConfigureKeyboard()
{
	SetSimpleBinding(PS2::CControllerInfo::START,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RETURN));
	SetSimpleBinding(PS2::CControllerInfo::SELECT,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LSHIFT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_LEFT,	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LEFT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_RIGHT,	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RIGHT));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_UP,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_UP));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_DOWN,	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_DOWN));
	SetSimpleBinding(PS2::CControllerInfo::SQUARE,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_A));
	SetSimpleBinding(PS2::CControllerInfo::CROSS,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_Z));
	SetSimpleBinding(PS2::CControllerInfo::TRIANGLE,	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_S));
	SetSimpleBinding(PS2::CControllerInfo::CIRCLE,		CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_X));

	//CInputConfig::GetInstance().SetSimulatedAxisBinding(CControllerInfo::ANALOG_LEFT_X,
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LEFT),
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RIGHT));
	//CInputConfig::GetInstance().SetSimulatedAxisBinding(CControllerInfo::ANALOG_LEFT_Y,
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_UP),
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_DOWN));

	//CInputConfig::GetInstance().SetSimulatedAxisBinding(CControllerInfo::ANALOG_RIGHT_X,
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_LEFT),
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_RIGHT));
	//CInputConfig::GetInstance().SetSimulatedAxisBinding(CControllerInfo::ANALOG_RIGHT_Y,
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_UP),
	//	CInputConfig::BINDINGINFO(GUID_SysKeyboard, DIK_DOWN));
}

const CInputConfig::CBinding* CInputConfig::GetBinding(PS2::CControllerInfo::BUTTON button) const
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	return m_bindings[button].get();
}

void CInputConfig::SetSimpleBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimpleBinding(binding.device, binding.id));
}

void CInputConfig::SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding1, const BINDINGINFO& binding2)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimulatedAxisBinding(binding1, binding2));
}

void CInputConfig::TranslateInputEvent(const GUID& device, uint32 id, uint32 value, const InputEventHandler& eventHandler)
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BindingPtr& binding = m_bindings[i];
		if(!binding) continue;
		binding->ProcessEvent(device, id, value, static_cast<PS2::CControllerInfo::BUTTON>(i), eventHandler);
	}
}

std::tstring CInputConfig::GetBindingDescription(Framework::DirectInput::CManager* directInputManager, PS2::CControllerInfo::BUTTON button) const
{
	assert(button < PS2::CControllerInfo::MAX_BUTTONS);
	const BindingPtr& binding = m_bindings[button];
	if(binding)
	{
		return binding->GetDescription(directInputManager);
	}
	else
	{
		return _T("");
	}
}

////////////////////////////////////////////////
// SimpleBinding
////////////////////////////////////////////////

CInputConfig::CSimpleBinding::CSimpleBinding(const GUID& device, uint32 id) 
: BINDINGINFO(device, id)
{

}

CInputConfig::CSimpleBinding::~CSimpleBinding()
{

}

CInputConfig::BINDINGTYPE CInputConfig::CSimpleBinding::GetBindingType() const
{
	return BINDING_SIMPLE;
}

void CInputConfig::CSimpleBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.SetPreferenceString(
		Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), 
		boost::lexical_cast<std::string>(device).c_str());
	config.SetPreferenceInteger(
		Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 
		id);
}

void CInputConfig::CSimpleBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	device = boost::lexical_cast<GUID>(config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
}

std::tstring CInputConfig::CSimpleBinding::GetDescription(Framework::DirectInput::CManager* directInputManager) const
{
	DIDEVICEINSTANCE deviceInstance;
	DIDEVICEOBJECTINSTANCE objectInstance;
	if(!directInputManager->GetDeviceInfo(device, &deviceInstance))
	{
		return _T("");
	}
	if(!directInputManager->GetDeviceObjectInfo(device, id, &objectInstance))
	{
		return _T("");
	}
	return std::tstring(deviceInstance.tszInstanceName) + _T(": ") + std::tstring(objectInstance.tszName);
}

void CInputConfig::CSimpleBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value, PS2::CControllerInfo::BUTTON button, const InputEventHandler& eventHandler)
{
	if(id != BINDINGINFO::id) return;
	if(device != BINDINGINFO::device) return;
	eventHandler(button, value);
}

void CInputConfig::CSimpleBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.RegisterPreferenceString(
		Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), 
		boost::lexical_cast<std::string>(GUID()).c_str());
	config.RegisterPreferenceInteger(
		Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 
		0);
}

////////////////////////////////////////////////
// SimulatedAxisBinding
////////////////////////////////////////////////

CInputConfig::CSimulatedAxisBinding::CSimulatedAxisBinding(const BINDINGINFO& binding1, const BINDINGINFO& binding2)
: m_key1Binding(binding1)
, m_key2Binding(binding2)
{

}

CInputConfig::CSimulatedAxisBinding::~CSimulatedAxisBinding()
{

}

void CInputConfig::CSimulatedAxisBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
//    string prefBase = string(buttonBase) + "." + string(CONFIG_SIMPLEBINDING_PREFIX);
//    config.RegisterPreferenceString(
//        (prefBase + "." + string(CONFIG_SIMPLEBINDING_DEVICE)).c_str(), lexical_cast<string>(GUID()).c_str());
//    config.RegisterPreferenceInteger(
//        (prefBase + "." + string(CONFIG_SIMPLEBINDING_ID)).c_str(), 0);
}

CInputConfig::BINDINGTYPE CInputConfig::CSimulatedAxisBinding::GetBindingType() const
{
	return BINDING_SIMULATEDAXIS;
}

void CInputConfig::CSimulatedAxisBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	//string prefBase = CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	//config.SetPreferenceString(
	//    CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), 
	//    boost::lexical_cast<string>(device).c_str());
	//config.SetPreferenceInteger(
	//    CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 
	//    id);
}

void CInputConfig::CSimulatedAxisBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	//string prefBase = CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	//device = boost::lexical_cast<GUID>(config.GetPreferenceString(CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	//id = config.GetPreferenceInteger(CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
}

std::tstring CInputConfig::CSimulatedAxisBinding::GetDescription(Framework::DirectInput::CManager* directInputManager) const
{
	//DIDEVICEINSTANCE deviceInstance;
	//DIDEVICEOBJECTINSTANCE objectInstance;
	//if(!directInputManager->GetDeviceInfo(device, &deviceInstance))
	//{
	//    return _T("");
	//}
	//if(!directInputManager->GetDeviceObjectInfo(device, id, &objectInstance))
	//{
	//    return _T("");
	//}
	//return tstring(deviceInstance.tszInstanceName) + _T(": ") + tstring(objectInstance.tszName);
	return std::tstring(_T("pwned!"));
}

void CInputConfig::CSimulatedAxisBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value, PS2::CControllerInfo::BUTTON button, const InputEventHandler& eventHandler)
{
	bool update = false;
	if(id == m_key1Binding.id && device == m_key1Binding.device)
	{
		m_key1State = value;
		update = true;
	}
	else if(id == m_key2Binding.id && device == m_key2Binding.device)
	{
		m_key2State = value;
		update = true;
	}

	if(update)
	{
		uint32 value = 0;
		if(m_key1State && m_key2State)
		{
			value = 0;
		}
		if(m_key1State)
		{
			value = 0x7FFF;
		}
		else if(m_key2State)
		{
			value = 0x8000;
		}

		eventHandler(button, value);
	}
}
