#include <jni.h>
#include <cassert>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "PathUtils.h"
#include "../PS2VM.h"
#include "../GSH_Null.h"
#include "GSH_OpenGLAndroid.h"

#define LOG_NAME "Play!"

static CPS2VM* g_virtualMachine = nullptr;

void Log_Print(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	__android_log_vprint(ANDROID_LOG_INFO, LOG_NAME, fmt, ap);
	va_end(ap);
}

std::string GetStringFromJstring(JNIEnv* env, jstring javaString)
{
	auto nativeString = env->GetStringUTFChars(javaString, JNI_FALSE);
	std::string result(nativeString);
	env->ReleaseStringUTFChars(javaString, nativeString);
	return result;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setFilesDirPath(JNIEnv* env, jobject obj, jstring dirPathString)
{
	auto dirPath = env->GetStringUTFChars(dirPathString, 0);
	Framework::PathUtils::SetFilesDirPath(dirPath);
	env->ReleaseStringUTFChars(dirPathString, dirPath);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_createVirtualMachine(JNIEnv* env, jobject obj)
{
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_start(JNIEnv* env, jobject obj, 
	jobject surface, jstring selectedFilePath)
{
	auto nativeWindow = ANativeWindow_fromSurface(env, surface);
	g_virtualMachine->CreateGSHandler(CGSH_OpenGLAndroid::GetFactoryFunction(nativeWindow));
	g_virtualMachine->Reset();
	g_virtualMachine->m_ee->m_os->BootFromFile(GetStringFromJstring(env, selectedFilePath).c_str());
	Log_Print("Before step.");
	g_virtualMachine->StepEe();
	Log_Print("Stepping.");
}
