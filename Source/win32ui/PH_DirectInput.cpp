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
/*
	DIDEVICEOBJECTDATA d[DIBUFFERSIZE];

	DWORD nElements = DIBUFFERSIZE;
	HRESULT hRet = m_pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), d, &nElements, 0);
	if(FAILED(hRet))
	{
		m_pKeyboard->Acquire();
		return;
	}

    int8 analogX = 0;
    int8 analogY = 0;
    static bool rightPressed = false;
    static bool leftPressed = false;

	for(DWORD i = 0; i < nElements; i++)
	{
	    CPadListener::BUTTON nButton;
        if(TranslateKey(d[i].dwOfs, &nButton))
		{
            for(ListenerList::iterator listenerIterator(m_listeners.begin()); 
                listenerIterator != m_listeners.end(); listenerIterator++)
			{
	            CPadListener* pListener(*listenerIterator);
				pListener->SetButtonState(0, nButton, (d[i].dwData & 0x80) ? true : false, ram);
			}
		}

        //REMOVE
        if(d[i].dwOfs == DIK_LEFT)
        {
            leftPressed = (d[i].dwData & 0x80) ? true : false;
        }
        if(d[i].dwOfs == DIK_RIGHT)
        {
            rightPressed = (d[i].dwData & 0x80) ? true : false;
        }
        //REMOVE
    }

    if(rightPressed)
    {
        analogX += 0x7F;
    }
    if(leftPressed)
    {
        analogX -= 0x7F;
    }

    for(ListenerList::iterator listenerIterator(m_listeners.begin()); 
        listenerIterator != m_listeners.end(); listenerIterator++)
	{
        CPadListener* pListener(*listenerIterator);
        pListener->SetAnalogStickState(0, 0, rand() % 0x7F, rand() % 0x7F, ram);
	}
*/
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
