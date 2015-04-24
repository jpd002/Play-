#include <jni.h>
#include <cassert>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "PathUtils.h"
#include "../AppConfig.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
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
	assert(g_virtualMachine == nullptr);
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_NativeInterop_isVirtualMachineCreated(JNIEnv* env, jobject obj)
{
	return (g_virtualMachine != nullptr);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_NativeInterop_isVirtualMachineRunning(JNIEnv* env, jobject obj)
{
	if(g_virtualMachine == nullptr) return false;
	return g_virtualMachine->GetStatus() == CVirtualMachine::RUNNING;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_resumeVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	g_virtualMachine->Resume();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_pauseVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	g_virtualMachine->Pause();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_loadElf(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	g_virtualMachine->Reset();
	g_virtualMachine->m_ee->m_os->BootFromFile(GetStringFromJstring(env, selectedFilePath).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_bootDiskImage(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, GetStringFromJstring(env, selectedFilePath).c_str());
	g_virtualMachine->Pause();
	g_virtualMachine->Reset();
	try
	{
		g_virtualMachine->m_ee->m_os->BootFromCDROM(CPS2OS::ArgumentList());
	}
	catch(const std::exception& exception)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, exception.what());
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setupGsHandler(JNIEnv* env, jobject obj, jobject surface)
{
	auto nativeWindow = ANativeWindow_fromSurface(env, surface);
	auto gsHandler = g_virtualMachine->GetGSHandler();
	if(gsHandler == nullptr)
	{
		g_virtualMachine->CreateGSHandler(CGSH_OpenGLAndroid::GetFactoryFunction(nativeWindow));
	}
	else
	{
		static_cast<CGSH_OpenGLAndroid*>(gsHandler)->SetWindow(nativeWindow);
	}
}
