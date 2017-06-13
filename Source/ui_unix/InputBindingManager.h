#pragma once

#include "Config.h"
#include "ControllerInfo.h"
#include <memory>
#include <functional>
#include <qnamespace.h>

class CInputBindingManager
{
public:
	enum BINDINGTYPE
	{
		BINDING_UNBOUND = 0,
		BINDING_SIMPLE = 1,
		BINDING_SIMULATEDAXIS = 2,
		BINDING_POVHAT = 3,
	};

	struct BINDINGINFO
	{
		BINDINGINFO() = default;
		BINDINGINFO(std::array<uint32, 6> device, uint32 id, uint32 type = 0) : device(device), id(id), type(type) { }
		std::array<uint32, 6>		device;
		uint32				id = 0;
		uint32				type = 0;
	};

	class CBinding
	{
	public:
		virtual					~CBinding() {}

		virtual void				ProcessEvent(std::array<uint32, 6>, uint32, uint32) = 0;

		virtual BINDINGTYPE			GetBindingType() const = 0;
		virtual const char*			GetBindingTypeName() const = 0;
		virtual uint32				GetValue() const = 0;
		virtual void				SetValue(uint32) = 0;
		virtual std::string			GetDescription() const = 0;

		virtual void				Save(Framework::CConfig&, const char*) const = 0;
		virtual void				Load(Framework::CConfig&, const char*) = 0;
	};

	class CSimpleBinding : public CBinding
	{
	public:
		CSimpleBinding(std::array<uint32, 6>, uint32, uint32 t = 0);
		CSimpleBinding(Qt::Key);
		CSimpleBinding();
		~CSimpleBinding() override;

		void					ProcessEvent(std::array<uint32, 6>, uint32, uint32) override;
		static void				RegisterPreferences(Framework::CConfig&, const char*);

		BINDINGTYPE				GetBindingType() const override;
		const char*				GetBindingTypeName() const override;
		uint32					GetValue() const override;
		void					SetValue(uint32) override;
		std::string				GetDescription() const override;

		void					Save(Framework::CConfig&, const char*) const override;
		void					Load(Framework::CConfig&, const char*) override;


	private:
		uint32					m_keyCode;
		uint32					m_state;
		std::array<uint32, 6>			m_device;
		uint32					m_type;

	};

	class CPovHatBinding : public CBinding
	{
	public:
		CPovHatBinding(const BINDINGINFO&, uint32 = -1);
		CPovHatBinding();
		virtual					~CPovHatBinding();

		static void				RegisterPreferences(Framework::CConfig&, const char*);

		BINDINGTYPE				GetBindingType() const override;
		const char*				GetBindingTypeName() const override;
		void					ProcessEvent(std::array<uint32, 6>, uint32, uint32) override;

		uint32					GetValue() const override;
		void					SetValue(uint32) override;
		std::string				GetDescription() const override;


		void					Save(Framework::CConfig&, const char*) const override;
		void					Load(Framework::CConfig&, const char*) override;

	private:
				static int32				GetShortestDistanceBetweenAngles(int32, int32);

		BINDINGINFO				m_binding;
		uint32					m_refValue = 0;
		uint32					m_value = 0;
	};

	class CSimulatedAxisBinding : public CBinding
	{
	public:
		CSimulatedAxisBinding(const BINDINGINFO&, const BINDINGINFO&);
		CSimulatedAxisBinding(int, int);
		CSimulatedAxisBinding();
		~CSimulatedAxisBinding();

		void					ProcessEvent(std::array<uint32, 6>, uint32, uint32) override;
		static void				RegisterPreferences(Framework::CConfig&, const char*);


		BINDINGTYPE				GetBindingType() const override;
		const char*				GetBindingTypeName() const override;
		uint32					GetValue() const override;
		void					SetValue(uint32);
		std::string				GetDescription() const override;

		void					Save(Framework::CConfig&, const char*) const override;
		void					Load(Framework::CConfig&, const char*) override;

	private:
		static void				RegisterKeyBindingPreferences(Framework::CConfig&, const char*);
		void					SaveKeyBinding(Framework::CConfig&, const char*, const BINDINGINFO&) const;
		void					LoadKeyBinding(Framework::CConfig&, const char*, BINDINGINFO&);

		uint32					m_key1State;
		uint32					m_key2State;

		BINDINGINFO				m_key1Binding;
		BINDINGINFO				m_key2Binding;
	};

		CInputBindingManager(Framework::CConfig&);
		virtual						~CInputBindingManager();

		uint32						GetBindingValue(PS2::CControllerInfo::BUTTON) const;
		void						ResetBindingValues();

		const CBinding*				GetBinding(PS2::CControllerInfo::BUTTON) const;
		void						SetSimpleBinding(PS2::CControllerInfo::BUTTON, const BINDINGINFO&);
		void						SetPovHatBinding(PS2::CControllerInfo::BUTTON, const BINDINGINFO&, uint32);
		void						SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON, const BINDINGINFO&, const BINDINGINFO&);

		void						Load();
		void						Save();
		void						AutoConfigureKeyboard();
		void						OnInputEventReceived(std::array<uint32, 6>, uint32, uint32);

private:
		typedef std::shared_ptr<CBinding>	BindingPtr;

		BindingPtr							m_bindings[PS2::CControllerInfo::MAX_BUTTONS];
		static uint32						m_buttonDefaultValue[PS2::CControllerInfo::MAX_BUTTONS];

		Framework::CConfig&					m_config;
};
