#ifndef _INPUTCONFIG_H_
#define _INPUTCONFIG_H_

#include "directinput/Manager.h"
#include "tcharx.h"
#include "Types.h"
#include "Singleton.h"
#include "../AppConfig.h"
#include "../ControllerInfo.h"
#include <memory>
#include <functional>

class CInputConfig : public CDependantSingleton<CInputConfig, CAppConfig>
{
public:
    friend class CDependantSingleton<CInputConfig, CAppConfig>;

    typedef std::tr1::function<void (PS2::CControllerInfo::BUTTON, uint32)> InputEventHandler;

    enum BINDINGTYPE
    {
        BINDING_UNBOUND = 0,
        BINDING_SIMPLE = 1,
        BINDING_SIMULATEDAXIS = 2,
    };

    struct BINDINGINFO
    {
        BINDINGINFO(const GUID& device, uint32 id) : device(device), id(id) { }
        GUID        device;
        uint32      id;
    };

    class CBinding
    {
    public:
        virtual                 ~CBinding() {};
        virtual BINDINGTYPE     GetBindingType() const = 0;
        virtual void            ProcessEvent(const GUID&, uint32, uint32, PS2::CControllerInfo::BUTTON, const InputEventHandler&) = 0;
        virtual void            Save(Framework::CConfig&, const char*) const = 0;
        virtual void            Load(Framework::CConfig&, const char*) = 0;
        virtual std::tstring    GetDescription(DirectInput::CManager*) const = 0;
    };

    class CSimpleBinding : public CBinding, private BINDINGINFO
    {
    public:
                                CSimpleBinding(const GUID& = GUID(), uint32 = 0);
        virtual                 ~CSimpleBinding();

        static void             RegisterPreferences(Framework::CConfig&, const char*);

        virtual BINDINGTYPE     GetBindingType() const;
        virtual void            ProcessEvent(const GUID&, uint32, uint32, PS2::CControllerInfo::BUTTON, const InputEventHandler&);
        virtual void            Save(Framework::CConfig&, const char*) const;
        virtual void            Load(Framework::CConfig&, const char*);
        virtual std::tstring    GetDescription(DirectInput::CManager*) const;
    };

    class CSimulatedAxisBinding : public CBinding
    {
    public:
								CSimulatedAxisBinding(const BINDINGINFO&, const BINDINGINFO&);
		virtual					~CSimulatedAxisBinding();

        static void				RegisterPreferences(Framework::CConfig&, const char*);

        virtual BINDINGTYPE     GetBindingType() const;
        virtual void            ProcessEvent(const GUID&, uint32, uint32, PS2::CControllerInfo::BUTTON, const InputEventHandler&);
        virtual void            Save(Framework::CConfig&, const char*) const;
        virtual void            Load(Framework::CConfig&, const char*);
        virtual std::tstring    GetDescription(DirectInput::CManager*) const;

    private:
        BINDINGINFO				m_key1Binding;
        BINDINGINFO				m_key2Binding;

		uint32					m_key1State;
		uint32					m_key2State;
    };

    const CBinding*         GetBinding(PS2::CControllerInfo::BUTTON) const;
    void                    SetSimpleBinding(PS2::CControllerInfo::BUTTON, const BINDINGINFO&);
    void                    SetSimulatedAxisBinding(PS2::CControllerInfo::BUTTON, const BINDINGINFO&, const BINDINGINFO&);

    void                    Load();
    void                    Save();
	void					AutoConfigureKeyboard();
    void                    TranslateInputEvent(const GUID&, uint32, uint32, const InputEventHandler&);
    std::tstring            GetBindingDescription(DirectInput::CManager*, PS2::CControllerInfo::BUTTON) const;

private:
    typedef std::tr1::shared_ptr<CBinding> BindingPtr;

                            CInputConfig(CAppConfig&);
    virtual                 ~CInputConfig();

    static void             RegisterBindingPreference(Framework::CConfig&, const char*);

    BindingPtr              m_bindings[PS2::CControllerInfo::MAX_BUTTONS];
    static const char*      m_buttonName[PS2::CControllerInfo::MAX_BUTTONS];
};

#endif
