#include <jni.h>
#include "InputManager.h"
#include "PH_Android.h"
#include "../PS2VM.h"

extern CPS2VM* g_virtualMachine;

void CInputManager::SetButtonState(int buttonId, bool pressed)
{
	auto padHandler = g_virtualMachine->GetPadHandler();
	if(padHandler)
	{
		static_cast<CPH_Android*>(padHandler)->SetButtonState(buttonId, pressed);
	}
}

void CInputManager::SetAxisState(int buttonId, float value)
{
	auto padHandler = g_virtualMachine->GetPadHandler();
	if(padHandler)
	{
		static_cast<CPH_Android*>(padHandler)->SetAxisState(buttonId, value);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_InputManager_setButtonState(JNIEnv* env, jobject obj, jint buttonId, jboolean pressed)
{
	CInputManager::GetInstance().SetButtonState(buttonId, (pressed == JNI_TRUE));
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_InputManager_setAxisState(JNIEnv* env, jobject obj, jint buttonId, jfloat value)
{
	CInputManager::GetInstance().SetAxisState(buttonId, value);
}
