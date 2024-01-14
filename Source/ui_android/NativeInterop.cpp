#include <jni.h>
#include <cassert>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "android/AssetManager.h"
#include "android/ContentResolver.h"
#include "android/JavaVM.h"
#include "android/android_database_Cursor.h"
#include "android/android_net_Uri.h"
#include "android/android_os_ParcelFileDescriptor.h"
#include "android/java_security_MessageDigest.h"
#include "android/javax_crypto_Mac.h"
#include "android/javax_crypto_spec_SecretKeySpec.h"
#include "PathUtils.h"
#include "../AppConfig.h"
#include "../DiskUtils.h"
#include "../PH_Generic.h"
#include "../PS2VM.h"
#include "../PS2VM_Preferences.h"
#include "../gs/GSH_Null.h"
#include "NativeShared.h"
#include "GSH_OpenGLAndroid.h"
#include "GSH_VulkanAndroid.h"
#include "SH_OpenSL.h"
#include "ui_shared/StatsManager.h"
#include "com_virtualapplications_play_Bootable.h"
#include "http/java_io_InputStream.h"
#include "http/java_io_OutputStream.h"
#include "http/java_net_HttpURLConnection.h"
#include "http/java_net_URL.h"

CPS2VM* g_virtualMachine = nullptr;
CPS2VM::NewFrameEvent::Connection g_OnNewFrameConnection;
CGSHandler::NewFrameEvent::Connection g_OnGsNewFrameConnection;
int g_currentGsHandlerId = -1;

#define PREF_VIDEO_GS_HANDLER ("video.gshandler")
#define PREF_AUDIO_ENABLEOUTPUT ("audio.enableoutput")

#define PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL 0
#define PREFERENCE_VALUE_VIDEO_GS_HANDLER_VULKAN 1

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
	g_virtualMachine->ReloadSpuBlockCount();
	g_virtualMachine->ReloadFrameRateLimit();
	SetupSoundHandler();
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
	Framework::CJavaVM::SetJavaVM(vm);
	java::net::URL_ClassInfo::GetInstance().PrepareClassInfo();
	java::net::HttpURLConnection_ClassInfo::GetInstance().PrepareClassInfo();
	java::io::InputStream_ClassInfo::GetInstance().PrepareClassInfo();
	java::io::OutputStream_ClassInfo::GetInstance().PrepareClassInfo();
	java::security::MessageDigest_ClassInfo::GetInstance().PrepareClassInfo();
	javax::crypto::Mac_ClassInfo::GetInstance().PrepareClassInfo();
	javax::crypto::spec::SecretKeySpec_ClassInfo::GetInstance().PrepareClassInfo();
	android::content::ContentResolver_ClassInfo::GetInstance().PrepareClassInfo();
	android::database::Cursor_ClassInfo::GetInstance().PrepareClassInfo();
	android::net::Uri_ClassInfo::GetInstance().PrepareClassInfo();
	android::os::ParcelFileDescriptor_ClassInfo::GetInstance().PrepareClassInfo();
	com::virtualapplications::play::Bootable_ClassInfo::GetInstance().PrepareClassInfo();
	return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setFilesDirPath(JNIEnv* env, jobject obj, jstring dirPathString)
{
	auto dirPath = env->GetStringUTFChars(dirPathString, 0);
	Framework::PathUtils::SetFilesDirPath(dirPath);
	env->ReleaseStringUTFChars(dirPathString, dirPath);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setCacheDirPath(JNIEnv* env, jobject obj, jstring dirPathString)
{
	auto dirPath = env->GetStringUTFChars(dirPathString, 0);
	Framework::PathUtils::SetCacheDirPath(dirPath);
	env->ReleaseStringUTFChars(dirPathString, dirPath);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setAssetManager(JNIEnv* env, jobject obj, jobject assetManagerJava)
{
	auto assetManager = AAssetManager_fromJava(env, assetManagerJava);
	assert(assetManager != nullptr);
	Framework::Android::CAssetManager::GetInstance().SetAssetManager(assetManager);
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_setContentResolver(JNIEnv* env, jclass clazz, jobject contentResolverJava)
{
	auto contentResolver = Framework::CJavaObject::CastTo<android::content::ContentResolver>(contentResolverJava);
	Framework::Android::CContentResolver::GetInstance().SetContentResolver(std::move(contentResolver));
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_createVirtualMachine(JNIEnv* env, jobject obj)
{
	assert(g_virtualMachine == nullptr);
	g_virtualMachine = new CPS2VM();
	g_virtualMachine->Initialize();
	g_virtualMachine->CreatePadHandler(CPH_Generic::GetFactoryFunction());
#ifdef PROFILE
	g_OnNewFrameConnection = g_virtualMachine->OnNewFrame.Connect(std::bind(&CStatsManager::OnNewFrame, &CStatsManager::GetInstance(), g_virtualMachine));
#endif
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_VIDEO_GS_HANDLER, PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_AUDIO_ENABLEOUTPUT, true);
	CGSH_OpenGL::RegisterPreferences();
	g_currentGsHandlerId = -1;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_NativeInterop_isVirtualMachineCreated(JNIEnv* env, jobject obj)
{
	return (g_virtualMachine != nullptr);
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
	int gsHandlerId = CAppConfig::GetInstance().GetPreferenceInteger(PREF_VIDEO_GS_HANDLER);
	bool recreateNeeded = (gsHandler == nullptr) || (g_currentGsHandlerId != gsHandlerId);
	if(recreateNeeded)
	{
		switch(gsHandlerId)
		{
		default:
		case PREFERENCE_VALUE_VIDEO_GS_HANDLER_OPENGL:
			g_virtualMachine->CreateGSHandler(CGSH_OpenGLAndroid::GetFactoryFunction(nativeWindow));
			break;
		case PREFERENCE_VALUE_VIDEO_GS_HANDLER_VULKAN:
			g_virtualMachine->CreateGSHandler(CGSH_VulkanAndroid::GetFactoryFunction(nativeWindow));
			break;
		}
		g_currentGsHandlerId = gsHandlerId;
		g_OnGsNewFrameConnection = g_virtualMachine->m_ee->m_gs->OnNewFrame.Connect(
		    std::bind(&CStatsManager::OnGsNewFrame, &CStatsManager::GetInstance(), std::placeholders::_1));
	}
	else
	{
		if(auto windowUpdateListener = dynamic_cast<INativeWindowUpdateListener*>(gsHandler))
		{
			windowUpdateListener->SetWindow(nativeWindow);
		}
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_NativeInterop_notifyPreferencesChanged(JNIEnv* env, jobject obj)
{
	SetupSoundHandler();
	g_virtualMachine->ReloadSpuBlockCount();
	g_virtualMachine->ReloadFrameRateLimit();
	auto gsHandler = g_virtualMachine->GetGSHandler();
	if(gsHandler != nullptr)
	{
		gsHandler->NotifyPreferencesChanged();
	}
}
