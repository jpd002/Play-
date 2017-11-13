#include <jni.h>
#include <cassert>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "android/AssetManager.h"
#include "android/JavaVM.h"
#include "PathUtils.h"
#include "../AppConfig.h"
#include "../DiskUtils.h"
#include "../PH_Generic.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../gs/GSH_Null.h"
#include "NativeShared.h"
#include "GSH_OpenGLAndroid.h"
#include "SH_OpenSL.h"
#include "StatsManager.h"
#include "com_virtualapplications_play_Bootable.h"

CPS2VM* g_virtualMachine = nullptr;

#define PREF_AUDIO_ENABLEOUTPUT ("audio.enableoutput")

static void SetupSoundHandler()
{
	assert(g_virtualMachine != nullptr);
	auto audioEnabled = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_AUDIO_ENABLEOUTPUT);
	if(audioEnabled)
	{
		g_virtualMachine->CreateSoundHandler(&CSH_OpenSL::HandlerFactory);
	}
	else
	{
		g_virtualMachine->DestroySoundHandler();
	}
}

static void ResetVirtualMachine()
{
	assert(g_virtualMachine != nullptr);
	g_virtualMachine->Pause();
	g_virtualMachine->Reset();
	SetupSoundHandler();
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
	Framework::CJavaVM::SetJavaVM(vm);
	com::virtualapplications::play::Bootable_ClassInfo::GetInstance().PrepareClassInfo();
	return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setFilesDirPath(JNIEnv* env, jobject obj, jstring dirPathString)
{
	auto dirPath = env->GetStringUTFChars(dirPathString, 0);
	Framework::PathUtils::SetFilesDirPath(dirPath);
	env->ReleaseStringUTFChars(dirPathString, dirPath);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setAssetManager(JNIEnv* env, jobject obj, jobject assetManagerJava)
{
	auto assetManager = AAssetManager_fromJava(env, assetManagerJava);
	assert(assetManager != nullptr);
	Framework::Android::CAssetManager::GetInstance().SetAssetManager(assetManager);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_createVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine == nullptr);
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());
#ifdef PROFILE
	g_virtualMachine->ProfileFrameDone.connect(boost::bind(&CStatsManager::OnProfileFrameDone, &CStatsManager::GetInstance(), _1));
#endif
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_AUDIO_ENABLEOUTPUT, true);
	CGSH_OpenGL::RegisterPreferences();
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

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_loadState(JNIEnv* env, jobject obj, jint slot)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	auto stateFilePath = g_virtualMachine->GenerateStatePath(slot);
	auto resultFuture = g_virtualMachine->LoadState(stateFilePath);
	if(!resultFuture.get())
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, "LoadState failed.");
		return;
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_saveState(JNIEnv* env, jobject obj, jint slot)
{
	assert(g_virtualMachine != nullptr);
	if(g_virtualMachine == nullptr) return;
	Framework::PathUtils::EnsurePathExists(CPS2VM::GetStateDirectoryPath());
	auto stateFilePath = g_virtualMachine->GenerateStatePath(slot);
	auto resultFuture = g_virtualMachine->SaveState(stateFilePath);
	if(!resultFuture.get())
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, "SaveState failed.");
		return;
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_loadElf(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	ResetVirtualMachine();
	try
	{
		g_virtualMachine->m_ee->m_os->BootFromFile(GetStringFromJstring(env, selectedFilePath));
	}
	catch(const std::exception& exception)
	{
		jclass exceptionClass = env->FindClass("java/lang/Exception");
		env->ThrowNew(exceptionClass, exception.what());
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_bootDiskImage(JNIEnv* env, jobject obj, jstring selectedFilePath)
{
	assert(g_virtualMachine != nullptr);
	CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, GetStringFromJstring(env, selectedFilePath));
	ResetVirtualMachine();
	try
	{
		g_virtualMachine->m_ee->m_os->BootFromCDROM();
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
		g_virtualMachine->m_ee->m_gs->OnNewFrame.connect(
		    boost::bind(&CStatsManager::OnNewFrame, &CStatsManager::GetInstance(), _1));
	}
	else
	{
		static_cast<CGSH_OpenGLAndroid*>(gsHandler)->SetWindow(nativeWindow);
	}
}

extern "C" JNIEXPORT jstring JNICALL Java_com_virtualapplications_play_NativeInterop_getDiskId(JNIEnv* env, jobject obj, jstring diskImagePath)
{
	std::string diskId;
	bool succeeded = DiskUtils::TryGetDiskId(GetStringFromJstring(env, diskImagePath).c_str(), &diskId);
	if(!succeeded)
	{
		return NULL;
	}
	jstring result = env->NewStringUTF(diskId.c_str());
	return result;
}
