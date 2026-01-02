#include "InputProviderEmscripten.h"
#include "string_format.h"

constexpr uint32 PROVIDER_ID = 'EmSc';

enum
{
	INPUT_ARROW_UP = 0xFF80,
	INPUT_ARROW_DOWN,
	INPUT_ARROW_LEFT,
	INPUT_ARROW_RIGHT,
	INPUT_ENTER,
	INPUT_BACKSPACE,
	INPUT_SHIFT_RIGHT,

	INPUT_KEY_A,
	INPUT_KEY_F,
	INPUT_KEY_G,
	INPUT_KEY_H,
	INPUT_KEY_I,
	INPUT_KEY_J,
	INPUT_KEY_K,
	INPUT_KEY_L,
	INPUT_KEY_S,
	INPUT_KEY_T,
	INPUT_KEY_X,
	INPUT_KEY_Z,

	INPUT_KEY_0,
	INPUT_KEY_1,
	INPUT_KEY_2,
	INPUT_KEY_3,
	INPUT_KEY_8,
	INPUT_KEY_9,
};

uint32 CInputProviderEmscripten::GetId() const
{
	return PROVIDER_ID;
}

std::string CInputProviderEmscripten::GetTargetDescription(const BINDINGTARGET& target) const
{
	return std::string();
}

BINDINGTARGET CInputProviderEmscripten::MakeBindingTarget(const EM_UTF8* code)
{
	uint32 keyCode = 0;
	if(!strcmp(code, "ArrowUp"))
		keyCode = INPUT_ARROW_UP;
	else if(!strcmp(code, "ArrowDown"))
		keyCode = INPUT_ARROW_DOWN;
	else if(!strcmp(code, "ArrowLeft"))
		keyCode = INPUT_ARROW_LEFT;
	else if(!strcmp(code, "ArrowRight"))
		keyCode = INPUT_ARROW_RIGHT;
	else if(!strcmp(code, "Enter"))
		keyCode = INPUT_ENTER;
	else if(!strcmp(code, "ShiftRight"))
		keyCode = INPUT_SHIFT_RIGHT;
	else if(!strcmp(code, "Backspace"))
		keyCode = INPUT_BACKSPACE;
	else if(!strcmp(code, "KeyA"))
		keyCode = INPUT_KEY_A;
	else if(!strcmp(code, "KeyF"))
		keyCode = INPUT_KEY_F;
	else if(!strcmp(code, "KeyG"))
		keyCode = INPUT_KEY_G;
	else if(!strcmp(code, "KeyH"))
		keyCode = INPUT_KEY_H;
	else if(!strcmp(code, "KeyI"))
		keyCode = INPUT_KEY_I;
	else if(!strcmp(code, "KeyJ"))
		keyCode = INPUT_KEY_J;
	else if(!strcmp(code, "KeyK"))
		keyCode = INPUT_KEY_K;
	else if(!strcmp(code, "KeyL"))
		keyCode = INPUT_KEY_L;
	else if(!strcmp(code, "KeyS"))
		keyCode = INPUT_KEY_S;
	else if(!strcmp(code, "KeyT"))
		keyCode = INPUT_KEY_T;
	else if(!strcmp(code, "KeyX"))
		keyCode = INPUT_KEY_X;
	else if(!strcmp(code, "KeyZ"))
		keyCode = INPUT_KEY_Z;
	else if(!strcmp(code, "Key0"))
		keyCode = INPUT_KEY_0;
	else if(!strcmp(code, "Key1"))
		keyCode = INPUT_KEY_1;
	else if(!strcmp(code, "Key2"))
		keyCode = INPUT_KEY_2;
	else if(!strcmp(code, "Key3"))
		keyCode = INPUT_KEY_3;
	else if(!strcmp(code, "Key8"))
		keyCode = INPUT_KEY_8;
	else if(!strcmp(code, "Key9"))
		keyCode = INPUT_KEY_9;
	else
		keyCode = code[0];
	return BINDINGTARGET(PROVIDER_ID, DeviceIdType{{0}}, keyCode, BINDINGTARGET::KEYTYPE::BUTTON);
}

void CInputProviderEmscripten::OnKeyDown(const EM_UTF8* code)
{
	OnInput(MakeBindingTarget(code), 1);
}

void CInputProviderEmscripten::OnKeyUp(const EM_UTF8* code)
{
	OnInput(MakeBindingTarget(code), 0);
}
