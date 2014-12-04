#include <jni.h>
#include "../PS2VM.h"

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_start(JNIEnv* env, jobject obj)
{
	CPS2VM virtualMachine;
	virtualMachine.Reset();
}
