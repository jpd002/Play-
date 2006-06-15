#ifndef _PH_DIRECTINPUT_H_
#define _PH_DIRECTINPUT_H_

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "PadHandler.h"

class CPH_DirectInput : public CPadHandler
{
public:
							CPH_DirectInput(HWND);
							~CPH_DirectInput();
	void					Update();

	static void				CreatePadHandler(HWND);

private:
	static CPadHandler*		PadHandlerFactory(void*);
	void					Initialize();
	bool					TranslateKey(uint32, CPadListener::BUTTON*);

	LPDIRECTINPUT8			m_pDI;
	LPDIRECTINPUTDEVICE8	m_pKeyboard;
	HWND					m_hWnd;
};

#endif
