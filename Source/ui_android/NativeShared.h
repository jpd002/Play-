#pragma once

#include <string>

void Log_Print(const char* fmt, ...);
std::string GetStringFromJstring(JNIEnv*, jstring);
