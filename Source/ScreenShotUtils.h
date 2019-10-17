#pragma once

#include "PS2VM.h"
#include "gs/GSHandler.h"

class CScreenShotUtils
{
public:
	typedef std::function<void(int, const char*)> Callback;
	typedef CGSHandler::FlipCompleteEvent::Connection Connection;

	static Connection TriggerGetScreenshot(CPS2VM*, Callback);

private:
	static fs::path GetScreenShotDirectoryPath();
	static fs::path GenerateScreenShotPath(const char* gameID);
};
