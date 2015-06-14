#include <jni.h>
#include "StatsManager.h"

void CStatsManager::OnNewFrame(uint32 drawCalls)
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames++;
	m_drawCalls += drawCalls;
}

uint32 CStatsManager::GetFrames()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	return m_frames;
}

uint32 CStatsManager::GetDrawCalls()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	return m_drawCalls;
}

void CStatsManager::ClearStats()
{
	std::lock_guard<std::mutex> statsLock(m_statsMutex);
	m_frames = 0;
	m_drawCalls = 0;
}

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
