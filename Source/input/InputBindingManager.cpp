#include "InputBindingManager.h"
#include "AppConfig.h"
#include "string_format.h"

#define CONFIG_PREFIX ("input")
#define CONFIG_BINDING_TYPE ("bindingtype")

#define CONFIG_BINDINGTARGET_PROVIDERID ("providerId")
#define CONFIG_BINDINGTARGET_DEVICEID ("deviceId")
#define CONFIG_BINDINGTARGET_KEYID ("keyId")
#define CONFIG_BINDINGTARGET_KEYTYPE ("keyType")

#define CONFIG_BINDINGTARGET1 ("bindingtarget1")
#define CONFIG_BINDINGTARGET2 ("bindingtarget2")

#define CONFIG_POVHATBINDING_REFVALUE ("povhatbinding.refvalue")

// clang-format off
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

const char* CInputBindingManager::m_padPreferenceName[] =
{
	"pad1",
	"pad2"
};

// clang-format on

static bool TryParseDeviceId(const char* input, DeviceIdType& out)
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
	bool parseResult = TryParseDeviceId(deviceIdString, result.deviceId);
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

CInputBindingManager::CInputBindingManager()
	: m_config(CAppConfig::GetInstance())
{
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			auto prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, m_padPreferenceName[pad], PS2::CControllerInfo::m_buttonName[button]);
			m_config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), 0);
			RegisterBindingTargetPreference(m_config, Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGTARGET1).c_str());
			if(PS2::CControllerInfo::IsAxis(static_cast<PS2::CControllerInfo::BUTTON>(button)))
			{
				RegisterBindingTargetPreference(m_config, Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGTARGET2).c_str());
			}
			CPovHatBinding::RegisterPreferences(m_config, prefBase.c_str());
		}
	}
	Load();
}

bool CInputBindingManager::HasBindings() const
{
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			const auto& binding = m_bindings[pad][button];
			if(binding) return true;
		}
	}
	return false;
}

void CInputBindingManager::RegisterInputProvider(const ProviderPtr& provider)
{
	provider->OnInput = [this] (auto target, auto value) { this->OnInputEventReceived(target, value); };
	m_providers.insert(std::make_pair(provider->GetId(), provider));
}

void CInputBindingManager::OverrideInputEventHandler(const InputEventFunction& inputEventHandler)
{
	InputEventFunction actualHandler = inputEventHandler;
	if(!actualHandler)
	{
		actualHandler = [this] (auto target, auto value) { this->OnInputEventReceived(target, value); };
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
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			auto binding = m_bindings[pad][button];
			if(!binding) continue;
			binding->ProcessEvent(target, value);
		}
	}
}

void CInputBindingManager::Load()
{
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			BINDINGTYPE bindingType = BINDING_UNBOUND;
			auto prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, m_padPreferenceName[pad], PS2::CControllerInfo::m_buttonName[button]);
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
			m_bindings[pad][button] = binding;
		}
	}
	ResetBindingValues();
}

void CInputBindingManager::Save()
{
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			const auto& binding = m_bindings[pad][button];
			if(!binding) continue;
			auto prefBase = Framework::CConfig::MakePreferenceName(CONFIG_PREFIX, m_padPreferenceName[pad], PS2::CControllerInfo::m_buttonName[button]);
			m_config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), binding->GetBindingType());
			binding->Save(m_config, prefBase.c_str());
		}
	}
	m_config.Save();
}

const CInputBindingManager::CBinding* CInputBindingManager::GetBinding(uint32 pad, PS2::CControllerInfo::BUTTON button) const
{
	if((pad >= MAX_PADS) || (button >= PS2::CControllerInfo::MAX_BUTTONS))
	{
		throw std::exception();
	}
	return m_bindings[pad][button].get();
}

uint32 CInputBindingManager::GetBindingValue(uint32 pad, PS2::CControllerInfo::BUTTON button) const
{
	assert(pad < MAX_PADS);
	assert(button < PS2::CControllerInfo::MAX_BUTTONS);
	const auto& binding = m_bindings[pad][button];
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
	for(unsigned int pad = 0; pad < MAX_PADS; pad++)
	{
		for(unsigned int button = 0; button < PS2::CControllerInfo::MAX_BUTTONS; button++)
		{
			const auto& binding = m_bindings[pad][button];
			if(!binding) continue;
			binding->SetValue(m_buttonDefaultValue[button]);
		}
	}
}

void CInputBindingManager::SetSimpleBinding(uint32 pad, PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding)
{
	if((pad >= MAX_PADS) || (button >= PS2::CControllerInfo::MAX_BUTTONS))
	{
		throw std::exception();
	}
	m_bindings[pad][button] = std::make_shared<CSimpleBinding>(binding);
}

void CInputBindingManager::SetPovHatBinding(uint32 pad, PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding, uint32 refValue)
{
	if((pad >= MAX_PADS) || (button >= PS2::CControllerInfo::MAX_BUTTONS))
	{
		throw std::exception();
	}
	m_bindings[pad][button] = std::make_shared<CPovHatBinding>(binding, refValue);
}

void CInputBindingManager::SetSimulatedAxisBinding(uint32 pad, PS2::CControllerInfo::BUTTON button, const BINDINGTARGET& binding1, const BINDINGTARGET& binding2)
{
	if((pad >= MAX_PADS) || (button >= PS2::CControllerInfo::MAX_BUTTONS))
	{
		throw std::exception();
	}
	m_bindings[pad][button] = std::make_shared<CSimulatedAxisBinding>(binding1, binding2);
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
	return "simplebinding";
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
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
	SaveBindingTargetPreference(config, prefBase.c_str(), m_binding);
}

void CInputBindingManager::CSimpleBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
	m_binding = LoadBindingTargetPreference(config, prefBase.c_str());
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
	return "povhatbinding";
}

void CInputBindingManager::CPovHatBinding::RegisterPreferences(Framework::CConfig& config, const char* buttonBase)
{
	config.RegisterPreferenceInteger(Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), -1);
}

void CInputBindingManager::CPovHatBinding::Save(Framework::CConfig& config, const char* buttonBase) const
{
	{
		auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
		SaveBindingTargetPreference(config, prefBase.c_str(), m_binding);
	}
	config.SetPreferenceInteger(Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_REFVALUE).c_str(), m_refValue);
}

void CInputBindingManager::CPovHatBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	{
		auto prefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
		m_binding = LoadBindingTargetPreference(config, prefBase.c_str());
	}
	m_refValue = config.GetPreferenceInteger(Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_POVHATBINDING_REFVALUE).c_str());
}

void CInputBindingManager::CPovHatBinding::ProcessEvent(const BINDINGTARGET& target, uint32 value)
{
	if(m_binding != target) return;
	m_value = value;
}

std::string CInputBindingManager::CPovHatBinding::GetDescription(CInputBindingManager* inputBindingManager) const
{
	return string_format("%s - %d",
						 inputBindingManager->GetTargetDescription(m_binding).c_str(),
						 m_refValue);
}

uint32 CInputBindingManager::CPovHatBinding::GetValue() const
{
	if(m_value >= BINDINGTARGET::POVHAT_MAX) return 0;
	int32 normalizedRefValue = (m_refValue * 360) / BINDINGTARGET::POVHAT_MAX;
	int32 normalizedValue = (m_value * 360) / BINDINGTARGET::POVHAT_MAX;
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
	return "simulatedaxisbinding";
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
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET2);
	SaveBindingTargetPreference(config, key1PrefBase.c_str(), m_key1Binding);
	SaveBindingTargetPreference(config, key2PrefBase.c_str(), m_key2Binding);
}

void CInputBindingManager::CSimulatedAxisBinding::Load(Framework::CConfig& config, const char* buttonBase)
{
	auto key1PrefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET1);
	auto key2PrefBase = Framework::CConfig::MakePreferenceName(buttonBase, CONFIG_BINDINGTARGET2);
	m_key1Binding = LoadBindingTargetPreference(config, key1PrefBase.c_str());
	m_key2Binding = LoadBindingTargetPreference(config, key2PrefBase.c_str());
}
