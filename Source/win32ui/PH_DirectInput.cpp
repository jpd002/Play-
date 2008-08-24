#include "PH_DirectInput.h"
#include "ControllerSettingsWnd.h"
#include "InputConfig.h"

using namespace Framework;
using namespace PS2;
using namespace std::tr1;
using namespace std::tr1::placeholders;

CPH_DirectInput::CPH_DirectInput(HWND hWnd) :
m_hWnd(hWnd),
m_manager(NULL)
{
	m_hWnd = hWnd;
	Initialize();
}

CPH_DirectInput::~CPH_DirectInput()
{
    delete m_manager;
}

CPadHandler::FactoryFunction CPH_DirectInput::GetFactoryFunction(HWND hWnd)
{
    return bind(&CPH_DirectInput::PadHandlerFactory, hWnd);
}

CPadHandler* CPH_DirectInput::PadHandlerFactory(HWND hWnd)
{
	return new CPH_DirectInput(hWnd);
}

void CPH_DirectInput::Initialize()
{
    m_manager = new DirectInput::CManager();
    m_manager->CreateKeyboard(m_hWnd);
    m_manager->CreateJoysticks(m_hWnd);
}

DirectInput::CManager* CPH_DirectInput::GetManager() const
{
    return m_manager;
}

void CPH_DirectInput::Update(uint8* ram)
{
    CInputConfig::InputEventHandler eventHandler(bind(&CPH_DirectInput::ProcessEvents, this, _1, _2, ram));
    m_manager->ProcessEvents(
        bind(&CInputConfig::TranslateInputEvent, &CInputConfig::GetInstance(), _1, _2, _3, std::tr1::cref(eventHandler)));
}

void CPH_DirectInput::ProcessEvents(CControllerInfo::BUTTON button, uint32 value, uint8* ram)
{
    for(ListenerList::iterator listenerIterator(m_listeners.begin()); 
        listenerIterator != m_listeners.end(); listenerIterator++)
    {
        CPadListener* pListener(*listenerIterator);
        if(CControllerInfo::IsAxis(button))
        {
            pListener->SetAxisState(0, button, static_cast<uint8>((value & 0xFFFF) >> 8), ram);
        }
        else
        {
            pListener->SetButtonState(0, button, value ? true : false, ram);
        }
    }
}

Win32::CModalWindow* CPH_DirectInput::CreateSettingsDialog(HWND parent)
{
    return new CControllerSettingsWnd(parent, m_manager);
}

void CPH_DirectInput::OnSettingsDialogDestroyed()
{

}
