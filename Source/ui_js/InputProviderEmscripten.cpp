#include "InputProviderEmscripten.h"
#include "string_format.h"

#define PROVIDER_ID 'EmSc'

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
	INPUT_KEY_S,
	INPUT_KEY_Z,
	INPUT_KEY_X,
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
	if(!strcmp(code, "ArrowUp")) keyCode = INPUT_ARROW_UP;
	else if(!strcmp(code, "ArrowDown")) keyCode = INPUT_ARROW_DOWN;
	else if(!strcmp(code, "ArrowLeft")) keyCode = INPUT_ARROW_LEFT;
	else if(!strcmp(code, "ArrowRight")) keyCode = INPUT_ARROW_RIGHT;
	else if(!strcmp(code, "Enter")) keyCode = INPUT_ENTER; 
	else if(!strcmp(code, "ShiftRight")) keyCode = INPUT_SHIFT_RIGHT;
	else if(!strcmp(code, "Backspace")) keyCode = INPUT_BACKSPACE;
	else if(!strcmp(code, "KeyA")) keyCode = INPUT_KEY_A;
	else if(!strcmp(code, "KeyS")) keyCode = INPUT_KEY_S;
	else if(!strcmp(code, "KeyZ")) keyCode = INPUT_KEY_Z;
	else if(!strcmp(code, "KeyX")) keyCode = INPUT_KEY_X;
	else keyCode = code[0];
	return BINDINGTARGET(PROVIDER_ID, DeviceIdType{{0}}, keyCode, BINDINGTARGET::KEYTYPE::BUTTON);
}

void CInputProviderEmscripten::OnKeyDown(const EM_UTF8* code)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(code), 1);
}

void CInputProviderEmscripten::OnKeyUp(const EM_UTF8* code)
{
	if(!OnInput) return;
	OnInput(MakeBindingTarget(code), 0);
}
