#pragma once

#include "Config.h"
#include "ControllerInfo.h"
#include "InputProvider.h"
#include <array>
#include <memory>
#include <functional>

class CInputBindingManager
{
public:
	typedef std::shared_ptr<CInputProvider> ProviderPtr;
	
	enum
	{
		MAX_PADS = 2,
	};
	
	enum BINDINGTYPE
	{
		BINDING_UNBOUND = 0,
		BINDING_SIMPLE = 1,
		BINDING_SIMULATEDAXIS = 2,
		BINDING_POVHAT = 3,
	};

	class CBinding
	{
	public:
		virtual ~CBinding() = default;

		virtual void ProcessEvent(const BINDINGTARGET&, uint32) = 0;

		virtual BINDINGTYPE GetBindingType() const = 0;
		virtual const char* GetBindingTypeName() const = 0;
		virtual uint32 GetValue() const = 0;
		virtual void SetValue(uint32) = 0;
		virtual std::string GetDescription(CInputBindingManager*) const = 0;

		virtual void Save(Framework::CConfig&, const char*) const = 0;
		virtual void Load(Framework::CConfig&, const char*) = 0;
	};
	
	CInputBindingManager();
	virtual ~CInputBindingManager() = default;

	bool HasBindings() const;
	
	void RegisterInputProvider(const ProviderPtr&);
	void OverrideInputEventHandler(const InputEventFunction&);
	
	std::string GetTargetDescription(const BINDINGTARGET&) const;
	
	uint32 GetBindingValue(uint32, PS2::CControllerInfo::BUTTON) const;
	void ResetBindingValues();

	const CBinding* GetBinding(uint32, PS2::CControllerInfo::BUTTON) const;
	void SetSimpleBinding(uint32, PS2::CControllerInfo::BUTTON, const BINDINGTARGET&);
	void SetPovHatBinding(uint32, PS2::CControllerInfo::BUTTON, const BINDINGTARGET&, uint32);
	void SetSimulatedAxisBinding(uint32, PS2::CControllerInfo::BUTTON, const BINDINGTARGET&, const BINDINGTARGET&);

	void Load();
	void Save();

private:
	class CSimpleBinding : public CBinding
	{
	public:
		CSimpleBinding() = default;
		CSimpleBinding(const BINDINGTARGET&);
		
		void ProcessEvent(const BINDINGTARGET&, uint32) override;
		
		BINDINGTYPE GetBindingType() const override;
		const char* GetBindingTypeName() const override;
		std::string GetDescription(CInputBindingManager*) const override;
		
		uint32 GetValue() const override;
		void SetValue(uint32) override;
		
		void Save(Framework::CConfig&, const char*) const override;
		void Load(Framework::CConfig&, const char*) override;
		
	private:
		BINDINGTARGET m_binding;
		uint32 m_value = 0;
	};
	
	class CPovHatBinding : public CBinding
	{
	public:
		CPovHatBinding() = default;
		CPovHatBinding(const BINDINGTARGET&, uint32 = -1);
		
		void ProcessEvent(const BINDINGTARGET&, uint32) override;
		
		BINDINGTYPE GetBindingType() const override;
		const char* GetBindingTypeName() const override;
		std::string GetDescription(CInputBindingManager*) const override;
		
		uint32 GetValue() const override;
		void SetValue(uint32) override;
		
		static void RegisterPreferences(Framework::CConfig&, const char*);
		void Save(Framework::CConfig&, const char*) const override;
		void Load(Framework::CConfig&, const char*) override;
		
	private:
		static int32 GetShortestDistanceBetweenAngles(int32, int32);
		
		BINDINGTARGET m_binding;
		uint32 m_refValue = 0;
		uint32 m_value = 0;
	};
	
	class CSimulatedAxisBinding : public CBinding
	{
	public:
		CSimulatedAxisBinding() = default;
		CSimulatedAxisBinding(const BINDINGTARGET&, const BINDINGTARGET&);
		
		void ProcessEvent(const BINDINGTARGET&, uint32) override;
		
		BINDINGTYPE GetBindingType() const override;
		const char* GetBindingTypeName() const override;
		std::string GetDescription(CInputBindingManager*) const override;
		
		uint32 GetValue() const override;
		void SetValue(uint32) override;
		
		void Save(Framework::CConfig&, const char*) const override;
		void Load(Framework::CConfig&, const char*) override;
		
	private:
		BINDINGTARGET m_key1Binding;
		BINDINGTARGET m_key2Binding;
		
		uint32 m_key1State = 0;
		uint32 m_key2State = 0;
	};

	typedef std::shared_ptr<CBinding> BindingPtr;
	typedef std::map<uint32, ProviderPtr> ProviderMap;

	void OnInputEventReceived(const BINDINGTARGET&, uint32);

	BindingPtr m_bindings[MAX_PADS][PS2::CControllerInfo::MAX_BUTTONS];
	static uint32 m_buttonDefaultValue[PS2::CControllerInfo::MAX_BUTTONS];
	static const char* m_padPreferenceName[MAX_PADS];
	
	Framework::CConfig& m_config;
	ProviderMap m_providers;
};
