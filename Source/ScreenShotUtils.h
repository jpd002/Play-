#pragma once
#include "Config.h"
#include "PS2VM.h"

class CScreenShotUtils : public Framework::CConfig
{
public:
	typedef std::function<void(int, const char*)>		Callback;

	static void								TriggerGetScreenshot(CPS2VM*, Callback);

private:
	static CConfig::PathType				GetScreenShotDirectoryPath();
	static CConfig::PathType				GenerateScreenShotPath(const char* gameID);
};
