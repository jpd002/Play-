#pragma once

#include <jni.h>
#include <string>

void Log_Print(const char* fmt, ...);
std::string GetStringFromJstring(JNIEnv*, jstring);
