#include "NativeShared.h"
#include <android/log.h>

#define LOG_NAME "Play!"

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
