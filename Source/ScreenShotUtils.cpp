#include "AppConfig.h"
#include "bitmap/BMP.h"
#include "ScreenShotUtils.h"
#include "StdStream.h"
#include "PathUtils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

void CScreenShotUtils::TriggerGetScreenshot(CPS2VM* virtualMachine, Callback completionCallback)
{
	virtualMachine->m_ee->m_gs->OnFlipComplete.connect_extended(
		[=](const boost::signals2::connection &c)->void
		{
			c.disconnect();
			try
			{
				auto m_buffer = virtualMachine->m_ee->m_gs->GetScreenshot();
				auto name = GenerateScreenShotPath(virtualMachine->m_ee->m_os->GetExecutableName());
				Framework::CStdStream outputStream(name.string().c_str(), "wb");
				Framework::CBMP::WriteBitmap(m_buffer, outputStream);

				std::ostringstream oss;
				oss << "Screenshot saved as " << name.filename();
				auto msgstr = oss.str();
				completionCallback(0, msgstr.c_str());
			}
			catch (const std::exception& e)
			{
				completionCallback(-1, "Error occured while trying to save screenshot.");
			}
		}
	);
}

Framework::CConfig::PathType CScreenShotUtils::GetScreenShotDirectoryPath()
{
	auto screenshotpath(CAppConfig::GetBasePath() / boost::filesystem::path("screenshots"));
	Framework::PathUtils::EnsurePathExists(screenshotpath);
	return screenshotpath;
}

Framework::CConfig::PathType CScreenShotUtils::GenerateScreenShotPath(const char* gameID)
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() % 1000;

	std::ostringstream oss;
	oss << gameID << std::put_time(&tm, "_%d-%m-%Y_%H.%M.%S.") << ms << ".bmp";
	auto screenshotFileName = oss.str();

	return GetScreenShotDirectoryPath() / boost::filesystem::path(screenshotFileName);
}
