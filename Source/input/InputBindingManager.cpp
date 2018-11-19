#include "InputBindingManager.h"
#include "AppConfig.h"
#include "string_format.h"

#define CONFIG_PREFIX ("input")
#define CONFIG_BINDING_TYPE ("bindingtype")

#define CONFIG_BINDINGTARGET_PROVIDERID ("providerId")
#define CONFIG_BINDINGTARGET_DEVICEID ("deviceId")
#define CONFIG_BINDINGTARGET_KEYID ("keyId")
#define CONFIG_BINDINGTARGET_KEYTYPE ("keyType")

#define CONFIG_SIMPLEBINDING_PREFIX ("simplebinding")

#define CONFIG_POVHATBINDING_PREFIX ("povhatbinding")
#define CONFIG_POVHATBINDING_REFVALUE ("refvalue")

#define CONFIG_SIMULATEDAXISBINDING_PREFIX ("simulatedaxisbinding")
#define CONFIG_SIMULATEDAXISBINDING_KEY1 ("key1")
#define CONFIG_SIMULATEDAXISBINDING_KEY2 ("key2")

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
        0};

CInputBindingManager::CInputBindingManager()
	: m_config(CAppConfig::GetInstance())
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

bool CInputBindingManager::HasBindings() const
{
	for(auto& binding : m_bindings)
	{
		if(binding) return true;
	}
	return false;
}

void CInputBindingManager::RegisterInputProvider(const ProviderPtr& provider)
{
	provider->OnInput = [this] (auto target, auto value) { OnInputEventReceived(target, value); };
	m_providers.insert(std::make_pair(provider->GetId(), provider));
}

void CInputBindingManager::OverrideInputEventHandler(const InputEventFunction& inputEventHandler)
{
	InputEventFunction actualHandler = inputEventHandler;
	if(!actualHandler)
	{
		actualHandler = [this] (auto target, auto value) { OnInputEventReceived(target, value); };
	}
	for(auto& providerPair : m_providers)
	{
		auto& provider = providerPair.second;
		provider->OnInput = actualHandler;
	}
}

std::string CInputBindingManager::GetTargetDescription(const BINDINGTARGET& target) const
{
	auto providerIterator = m_providers.find(target.providerId);
	if(providerIterator == std::end(m_providers)) throw std::exception();
	auto provider = providerIterator->second;
	return provider->GetTargetDescription(target);
}

void CInputBindingManager::OnInputEventReceived(const BINDINGTARGET& target, uint32 value)
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		auto binding = m_bindings[i];
		if(!binding) continue;
		binding->ProcessEvent(target, value);
	}
}

void CInputBindingManager::Load()
{
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
			binding = std::make_shared<CSimpleBinding>();
			break;
		case BINDING_POVHAT:
			binding = std::make_shared<CPovHatBinding>();
			break;
		case BINDING_SIMULATEDAXIS:
			binding = std::make_shared<CSimulatedAxisBinding>();
			break;
		}
		if(binding)
		{
			binding->Load(m_config, prefBase.c_str());
		}
		m_bindings[i] = binding;
	}
	ResetBindingValues();
}

void CInputBindingManager::Save()
{
	for(unsigned int i = 0; i < PS2::CControllerInfo::MAX_BUTTONS; i++)
	{
		BindingPtr& binding = m_bindings[i];
		if(!binding) continue;
		std::string prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, PS2::CControllerInfo::m_buttonName[i]);
		m_config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), binding->GetBindingType());
		binding->Save(m_config, prefBase.c_str());
	}
	m_config.Save();
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

void CInputBindingManager::SetSimpleBinding(PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button] = std::make_shared<CSimpleBinding>(binding);
}

void CInputBindingManager::SetPovHatBinding(PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding, uint32 refValue)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button] = std::make_shared<CPovHatBinding>(binding, refValue);
}

void CInputBindingManager::SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding1, const BINDINGTARGET& binding2)
{
	if(button >= PS2::CControllerInfo::MAX_BUTTONS)
	{
		throw std::exception();
	}
	m_bindings[button] = std::make_shared<CSimulatedAxisBinding>(binding1, binding2);
}

static bool ParseMAC(std::array<uint32, 6>& out, const char* input)
{
	uint32 bytes[6] = {0};
	if(std::sscanf(input,
				   "%x:%x:%x:%x:%x:%x",
				   &bytes[0], &bytes[1], &bytes[2],
				   &bytes[3], &bytes[4], &bytes[5]) != 6)
	{
		return false;
	}
	for(int i = 0; i < 6; ++i)
	{
		out.at(i) = bytes[i];
	}
	return true;
}

static void RegisterBindingTargetPreference(Framework::CConfig& config, const char* base)
{
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_PROVIDERID).c_str(), 0);
	config.RegisterPreferenceString(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_DEVICEID).c_str(), "0:0:0:0:0:0");
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYID).c_str(), 0);
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYTYPE).c_str(), 0);
}

static BINDINGTARGET LoadBindingTargetPreference(Framework::CConfig& config, const char* base)
{
	BINDINGTARGET result;
	result.providerId = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_PROVIDERID).c_str());
	auto deviceIdString = config.GetPreferenceString(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_DEVICEID).c_str());
	bool parseResult = ParseMAC(result.deviceId, deviceIdString);
	assert(parseResult);
	result.keyId = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYID).c_str());
	result.keyType = static_cast<BINDINGTARGET::KEYTYPE>(config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYTYPE).c_str()));
	return result;
}

static void SaveBindingTargetPreference(Framework::CConfig& config, const char* base, const BINDINGTARGET& target)
{
	auto deviceIdString = string_format("%x:%x:%x:%x:%x:%x",
										target.deviceId[0], target.deviceId[1], target.deviceId[2],
										target.deviceId[3], target.deviceId[4], target.deviceId[5]);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_PROVIDERID).c_str(), target.providerId);
	config.SetPreferenceString(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_DEVICEID).c_str(), deviceIdString.c_str());
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYID).c_str(), target.keyId);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(base, CONFIG_BINDINGTARGET_KEYTYPE).c_str(), static_cast<int32>(target.keyType));
}

////////////////////////////////////////////////
// SimpleBinding
////////////////////////////////////////////////
CInputBindingManager::CSimpleBinding::CSimpleBinding(const BINDINGTARGET& binding)
: m_binding(binding)
, m_value(0)
{
}

void CInputBindingManager::CSimpleBinding::ProcessEvent(const BINDINGTARGET& target, uint32 value)
{
	if(m_binding != target) return;
	m_value = value;
}

CInputBindingManager::BINDINGTYPE CInputBindingManager::CSimpleBinding::GetBindingType() const
{
	return BINDING_SIMPLE;
}

const char* CInputBindingManager::CSimpleBinding::GetBindingTypeName() const
{
	return CONFIG_SIMPLEBINDING_PREFIX;
}

uint32 CInputBindingManager::CSimpleBinding::GetValue() const
{
	return m_value;
}

void CInputBindingManager::CSimpleBinding::SetValue(uint32 value)
{
	m_value = value;
}

std::string CInputBindingManager::CSimpleBinding::GetDescription(CInputBindingManager* bindingManager) const
{
	return bindingManager->GetTargetDescription(m_binding);
}

void CInputBindingManager::CSimpleBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	SaveBindingTargetPreference(config, prefBase.c_str(), m_binding);
}

void CInputBindingManager::CSimpleBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	m_binding = LoadBindingTargetPreference(config, prefBase.c_str());
}

void CInputBindingManager::CSimpleBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
	RegisterBindingTargetPreference(config, prefBase.c_str());
}

////////////////////////////////////////////////
// PovHatBinding
////////////////////////////////////////////////

CInputBindingManager::CPovHatBinding::CPovHatBinding(const BINDINGTARGET& binding, uint32 refValue)
: m_binding(binding)
, m_refValue(refValue)
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
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	SaveBindingTargetPreference(config, prefBase.c_str(), m_binding);
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), m_refValue);
}

void CInputBindingManager::CPovHatBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	m_binding = LoadBindingTargetPreference(config, prefBase.c_str());
	m_refValue = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str());
}

void CInputBindingManager::CPovHatBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_PREFIX);
	RegisterBindingTargetPreference(config, prefBase.c_str());
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), -1);
}

void CInputBindingManager::CPovHatBinding::ProcessEvent(const BINDINGTARGET& target, uint32 value)
{
	if(m_binding != target) return;
	m_value = value;
}

std::string CInputBindingManager::CPovHatBinding::GetDescription(CInputBindingManager*) const
{
	return "CPovHatBinding";
}

uint32 CInputBindingManager::CPovHatBinding::GetValue() const
{
	if(m_value >= 8) return 0;
	int32 normalizedRefValue = (m_refValue * 360) / 8;
	int32 normalizedValue = (m_value * 360) / 8;
	if(GetShortestDistanceBetweenAngles(normalizedValue, normalizedRefValue) <= 45)
	{
		return 1;
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
	if(angle1 > 180) angle1 -= 360;
	if(angle1 < -180) angle1 += 360;
	if(angle2 > 180) angle2 -= 360;
	if(angle2 < -180) angle2 += 360;
	int32 angle = abs(angle1 - angle2);
	if(angle > 180)
	{
		angle = 360 - angle;
	}
	return angle;
}

////////////////////////////////////////////////
// CSimulatedAxisBinding
////////////////////////////////////////////////
CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding(const BINDINGTARGET& binding1, const BINDINGTARGET& binding2)
: m_key1Binding(binding1)
, m_key2Binding(binding2)
{
}

void CInputBindingManager::CSimulatedAxisBinding::ProcessEvent(const BINDINGTARGET& target, uint32 state)
{
	if(m_key1Binding == target)
	{
		m_key1State = state;
	}
	else if(m_key2Binding == target)
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

std::string CInputBindingManager::CSimulatedAxisBinding::GetDescription(CInputBindingManager* inputBindingManager) const
{
	return string_format("(%s, %s)",
						 inputBindingManager->GetTargetDescription(m_key1Binding).c_str(),
						 inputBindingManager->GetTargetDescription(m_key2Binding).c_str());
}

void CInputBindingManager::CSimulatedAxisBinding::SetValue(uint32 state)
{
	m_key1State = 0;
	m_key2State = 0;
}

void CInputBindingManager::CSimulatedAxisBinding::CSimulatedAxisBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	SaveBindingTargetPreference(config, key1PrefBase.c_str(), m_key1Binding);
	SaveBindingTargetPreference(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputBindingManager::CSimulatedAxisBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	m_key1Binding = LoadBindingTargetPreference(config, key1PrefBase.c_str());
	m_key2Binding = LoadBindingTargetPreference(config, key2PrefBase.c_str());
}

void CInputBindingManager::CSimulatedAxisBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_SIMULATEDAXISBINDING_PREFIX);
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(prefBase, CONFIG_SIMULATEDAXISBINDING_KEY2);
	RegisterBindingTargetPreference(config, key1PrefBase.c_str());
	RegisterBindingTargetPreference(config, key2PrefBase.c_str());
}

