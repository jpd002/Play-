#include <jni.h>
#include "ui_shared/StatsManager.h"

extern "C" JNIEXPORT jint JNICALL Java_com_virtualapplications_play_StatsManager_getFrames(JNIEnv* env, jobject obj)
{
	return CStatsManager::GetInstance().GetFrames();
}

extern "C" JNIEXPORT jint JNICALL Java_com_virtualapplications_play_StatsManager_getDrawCalls(JNIEnv* env, jobject obj)
{
	return CStatsManager::GetInstance().GetDrawCalls();
}

extern "C" JNIEXPORT void JNICALL Java_com_virtualapplications_play_StatsManager_clearStats(JNIEnv* env, jobject obj)
{
	CStatsManager::GetInstance().ClearStats();
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_virtualapplications_play_StatsManager_isProfiling(JNIEnv* env, jobject obj)
{
#ifdef PROFILE
	return JNI_TRUE;
#else
	return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jstring JNICALL Java_com_virtualapplications_play_StatsManager_getProfilingInfo(JNIEnv* env, jobject obj)
{
	std::string info;
#ifdef PROFILE
	info = CStatsManager::GetInstance().GetProfilingInfo();
#endif
	jstring result = env->NewStringUTF(info.c_str());
	return result;
}
