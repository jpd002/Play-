#include <QKeySequence>
#include <cstring>
#include <libevdev.h>
#include "InputBindingManager.h"
#include "GamePad/GamePadUtils.h"
#include "string_format.h"

#define CONFIG_PREFIX						("input")
#define CONFIG_BINDING_TYPE					("bindingtype")

#define CONFIG_SIMPLEBINDING_PREFIX			("simplebinding")

#define CONFIG_BINDINGINFO_DEVICE			("device")
#define CONFIG_BINDINGINFO_ID				("id")
#define CONFIG_BINDINGINFO_TYPE				("type")

#define CONFIG_POVHATBINDING_PREFIX			("povhatbinding")
#define CONFIG_POVHATBINDING_REFVALUE		("refvalue")

#define CONFIG_SIMULATEDAXISBINDING_PREFIX	("simulatedaxisbinding")
#define CONFIG_SIMULATEDAXISBINDING_KEY1	("key1")
#define CONFIG_SIMULATEDAXISBINDING_KEY2	("key2")

uint32 CInputBindingManager::m_buttonDefaultValue[PS2::CControllerInfo::MAX_BUTTONS] =
{
	0x7F,
	0x7F,
	0x7F,
	0x7F,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

CInputBindingManager::CInputBindingManager(Framework::CConfig& config)
	: m_config(config)
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
}

CInputBindingManager::~CInputBindingManager()
{
}

void CInputBindingManager::OnInputEventReceived(std::array<uint32, 6> device, uint32 id, uint32 value)
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		auto binding = m_bindings[i];
		if(!binding) continue;
		binding->ProcessEvent(device, id, value);
	}
}

void CInputBindingManager::Load()
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

void CInputBindingManager::Save()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BindingPtr& binding = m_bindings[i];
		if(binding == NULL) continue;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		m_config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), binding->GetBindingType());
		binding->Save(m_config, prefBase.c_str());
	}
	m_config.Save();
}

void CInputBindingManager::AutoConfigureKeyboard()
{
	SetSimpleBinding(PS2::CControllerInfo::START,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_Return));
	SetSimpleBinding(PS2::CControllerInfo::SELECT,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_Backspace));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_LEFT,	CInputBindingManager::BINDINGINFO({0}, Qt::Key_Left));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_RIGHT,	CInputBindingManager::BINDINGINFO({0}, Qt::Key_Right));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_UP,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_Up));
	SetSimpleBinding(PS2::CControllerInfo::DPAD_DOWN,	CInputBindingManager::BINDINGINFO({0}, Qt::Key_Down));
	SetSimpleBinding(PS2::CControllerInfo::SQUARE,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_A));
	SetSimpleBinding(PS2::CControllerInfo::CROSS,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_Z));
	SetSimpleBinding(PS2::CControllerInfo::TRIANGLE,	CInputBindingManager::BINDINGINFO({0}, Qt::Key_S));
	SetSimpleBinding(PS2::CControllerInfo::CIRCLE,		CInputBindingManager::BINDINGINFO({0}, Qt::Key_X));
	SetSimpleBinding(PS2::CControllerInfo::L1,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_1));
	SetSimpleBinding(PS2::CControllerInfo::L2,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_2));
	SetSimpleBinding(PS2::CControllerInfo::L3,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_3));
	SetSimpleBinding(PS2::CControllerInfo::R1,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_8));
	SetSimpleBinding(PS2::CControllerInfo::R2,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_9));
	SetSimpleBinding(PS2::CControllerInfo::R3,			CInputBindingManager::BINDINGINFO({0}, Qt::Key_0));

	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_LEFT_X,
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_F),
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_H));
	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_LEFT_Y,
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_T),
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_G));

	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_RIGHT_X,
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_J),
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_L));
	SetSimulatedAxisBinding(PS2::CControllerInfo::ANALOG_RIGHT_Y,
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_I),
							CInputBindingManager::BINDINGINFO({0}, Qt::Key_K));
}

const CInputBindingManager::CBinding* CInputBindingManager::GetBinding(PS2::CControllerInfo::BUTTON button) const
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	return m_bindings[button].get();
}

uint32 CInputBindingManager::GetBindingValue(PS2::CControllerInfo::BUTTON button) const
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

void CInputBindingManager::ResetBindingValues()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		auto binding = m_bindings[i];
		if(!binding) continue;
		binding->SetValue(m_buttonDefaultValue[i]);
	}
}

void CInputBindingManager::SetSimpleBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimpleBinding(binding.device, binding.id, binding.type));
}

void CInputBindingManager::SetPovHatBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding, uint32 refValue)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CPovHatBinding(binding, refValue));
}

void CInputBindingManager::SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON button, const BINDINGINFO& binding1, const BINDINGINFO& binding2)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button].reset(new CSimulatedAxisBinding(binding1, binding2));
}

////////////////////////////////////////////////
// SimpleBinding
////////////////////////////////////////////////
CInputBindingManager::CSimpleBinding::CSimpleBinding()
{

}

CInputBindingManager::CSimpleBinding::CSimpleBinding(Qt::Key keyCode)
	: m_keyCode(keyCode)
	, m_state(0)
	, m_device({0})
	, m_type(0)
{

}
CInputBindingManager::CSimpleBinding::CSimpleBinding(std::array<uint32, 6> device, uint32 keyCode, uint32 type)
	: m_keyCode(keyCode)
	, m_state(0)
	, m_device(device)
	, m_type(type)
{

}

CInputBindingManager::CSimpleBinding::~CSimpleBinding()
{

}

void CInputBindingManager::CSimpleBinding::ProcessEvent(std::array<uint32, 6> device, uint32 keyCode, uint32 state)
{
	if(device != m_device || keyCode != m_keyCode) return;
	m_state = state;
}

uint32 CInputBindingManager::CSimpleBinding::GetValue() const
{
	return m_state;
}

CInputBindingManager::BINDINGTYPE CInputBindingManager::CSimpleBinding::GetBindingType() const
{
	return BINDING_SIMPLE;
}

const char* CInputBindingManager::CSimpleBinding::GetBindingTypeName() const
{
	return CONFIG_SIMPLEBINDING_PREFIX;
}

std::string CInputBindingManager::CSimpleBinding::GetDescription() const
{
	if(m_type == 0)
	{
		return QString("Key: %1").arg(QKeySequence(m_keyCode).toString()).toStdString();
	}
	else
	{
		const char* buttonname = libevdev_event_code_get_name(m_type, m_keyCode);
		if (buttonname != NULL)
		{
			return QString("Key: %1").arg(buttonname).toStdString();
		}
		else
		{
			return QString("Key: %1").arg(QString::number(m_keyCode)).toStdString();
		}
	}
}

void CInputBindingManager::CSimpleBinding::SetValue(uint32 state)
{
	m_state = state;
}

void CInputBindingManager::CSimpleBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), string_format("%02x:%02x:%02x:%02x:%02x:%02x", m_device[0], m_device[1], m_device[2], m_device[3], m_device[4], m_device[5]).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), m_keyCode);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), m_type);
}

void CInputBindingManager::CSimpleBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	CGamePadUtils::ParseMAC(m_device, config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	m_keyCode = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
	m_type = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str());
}

void CInputBindingManager::CSimpleBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), "00:00:00:00:00");
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), 0);
}

////////////////////////////////////////////////
// PovHatBinding
////////////////////////////////////////////////
CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding(){}

CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding(const BINDINGINFO& binding1, const BINDINGINFO& binding2)
	: m_key1Binding(binding1)
	, m_key2Binding(binding2)
{

}

CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding(int id1, int id2)
	: m_key1Binding(BINDINGINFO({0},id1))
	, m_key2Binding(BINDINGINFO({0},id2))
{

}

CInputBindingManager::CSimulatedAxisBinding::~CSimulatedAxisBinding()
{

}

void CInputBindingManager::CSimulatedAxisBinding::ProcessEvent(std::array<uint32, 6> device, uint32 keyCode, uint32 state)
{
	if(keyCode == m_key1Binding.id && m_key1Binding.device == device)
	{
		m_key1State = state;
	}

	if(keyCode == m_key2Binding.id && m_key2Binding.device == device)
	{
		m_key2State = state;
	}
}

uint32 CInputBindingManager::CSimulatedAxisBinding::GetValue() const
{
	uint32 value = 0x7F;
	if(m_key1State && m_key2State)
	{
		value = 0x7F;
	}
	if(m_key1State)
	{
		value = 0;
	}
	else if(m_key2State)
	{
		value = 0xFF;
	}
	return value;
}

CInputBindingManager::BINDINGTYPE CInputBindingManager::CSimulatedAxisBinding::GetBindingType() const
{
	return BINDING_SIMULATEDAXIS;
}

const char* CInputBindingManager::CSimulatedAxisBinding::GetBindingTypeName() const
{
	return CONFIG_SIMULATEDAXISBINDING_PREFIX;
}

std::string CInputBindingManager::CSimulatedAxisBinding::GetDescription() const
{

	std::string desc = ("Key: ");
	if(m_key1Binding.type  == 0)
	{
		desc += QKeySequence(m_key1Binding.id).toString().toStdString();
	}
	else
	{
		const char* buttonname = libevdev_event_code_get_name(m_key1Binding.type, m_key1Binding.id);
		if (buttonname != NULL)
		{
			desc += buttonname;
		}
		else
		{
			desc += string_format("%d", m_key1Binding.id);
		}
	}
	desc += "/ Key: ";
	if(m_key2Binding.type == 0)
	{
		desc += QKeySequence(m_key2Binding.id).toString().toStdString();
	}
	else
	{
		const char* buttonname = libevdev_event_code_get_name(m_key2Binding.type, m_key2Binding.id);
		if (buttonname != NULL)
		{
			desc += buttonname;
		}
		else
		{
			desc += string_format("%d", m_key2Binding.id);
		}
	}

	return desc;
}

void CInputBindingManager::CSimulatedAxisBinding::SetValue(uint32 state)
{
	m_key1State = state;
	m_key2State = state;
}

void CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	SaveKeyBinding(config, key1PrefBase.c_str(), m_key1Binding);
	SaveKeyBinding(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputBindingManager::CSimulatedAxisBinding::SaveKeyBinding(Framework::CConfig& config, const char* prefBase, const BINDINGINFO& binding) const
{
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), string_format("%02x:%02x:%02x:%02x:%02x:%02x", binding.device[0], binding.device[1], binding.device[2], binding.device[3], binding.device[4], binding.device[5]).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), binding.id);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), binding.type);
}

void CInputBindingManager::CSimulatedAxisBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	LoadKeyBinding(config, key1PrefBase.c_str(), m_key1Binding);
	LoadKeyBinding(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputBindingManager::CSimulatedAxisBinding::LoadKeyBinding(Framework::CConfig& config, const char* prefBase, BINDINGINFO& binding)
{
	CGamePadUtils::ParseMAC(binding.device, config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	binding.id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
	binding.type = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str());
}

void CInputBindingManager::CSimulatedAxisBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	RegisterKeyBindingPreferences(config, key1PrefBase.c_str());
	RegisterKeyBindingPreferences(config, key2PrefBase.c_str());
}

void CInputBindingManager::CSimulatedAxisBinding::RegisterKeyBindingPreferences(Framework::CConfig& config, const char* prefBase)
{
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), "00:00:00:00:00");
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), 0);
}

////////////////////////////////////////////////
// PovHatBinding
////////////////////////////////////////////////

CInputBindingManager::CPovHatBinding::CPovHatBinding()
{

}

CInputBindingManager::CPovHatBinding::CPovHatBinding(const BINDINGINFO& bind, uint32 refValue)
	: m_binding(bind)
	, m_refValue(refValue)
{

}

CInputBindingManager::CPovHatBinding::~CPovHatBinding()
{

}

CInputBindingManager::BINDINGTYPE CInputBindingManager::CPovHatBinding::GetBindingType() const
{
	return BINDING_POVHAT;
}

const char* CInputBindingManager::CPovHatBinding::GetBindingTypeName() const
{
	return CONFIG_POVHATBINDING_PREFIX;
}

void CInputBindingManager::CPovHatBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), string_format("%02x:%02x:%02x:%02x:%02x:%02x", m_binding.device[0], m_binding.device[1], m_binding.device[2], m_binding.device[3], m_binding.device[4], m_binding.device[5]).c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), m_binding.id);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), m_binding.type);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), m_refValue);
}

void CInputBindingManager::CPovHatBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	CGamePadUtils::ParseMAC(m_binding.device, config.GetPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
	m_binding.id = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
	m_binding.type = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str());
	m_refValue = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str());
}

void CInputBindingManager::CPovHatBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	std::string prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), "00:00:00:00:00");
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_TYPE).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), -1);
}

void CInputBindingManager::CPovHatBinding::ProcessEvent(std::array<uint32, 6> device, uint32 id, uint32 value)
{
	if(id != m_binding.id) return;
	if(device != m_binding.device) return;
	m_value = value;
}

std::string CInputBindingManager::CPovHatBinding::GetDescription() const
{
	std::string key("Key: ");
	if(m_binding.type == 0)
	{
		return key + QKeySequence(m_binding.id).toString().toStdString();
	}
	else
	{
		const char* buttonname = libevdev_event_code_get_name(m_binding.type, m_binding.id);
		if (buttonname != NULL)
		{
			return  key + buttonname;
		}
		else
		{
			return  key + string_format("%d", m_binding.id);
		}
	}
}

uint32 CInputBindingManager::CPovHatBinding::GetValue() const
{
	if(m_value != m_refValue) return 0;
	int32 normalizedRefValue = m_refValue / 100;
	int32 normalizedValue = m_value / 100;
	if(GetShortestDistanceBetweenAngles(normalizedValue, normalizedRefValue) <= 45)
	{
		return m_refValue;
	}
	else
	{
		return 0;
	}
}

void CInputBindingManager::CPovHatBinding::SetValue(uint32 value)
{
	m_value = value;
}

int32 CInputBindingManager::CPovHatBinding::GetShortestDistanceBetweenAngles(int32 angle1, int32 angle2)
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
