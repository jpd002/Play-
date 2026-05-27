#include <QApplication>
#include "AppConfig.h"
#include "PathUtils.h"
#include "MainWindow.h"

int main(int argc, char** argv)
{
	QApplication a(argc, argv);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	CMainWindow w;
	w.show();
	return a.exec();
}

fs::path CAppConfig::GetBasePath() const
{
	static const char* BASE_DATA_PATH = "DiscChecker Data Files";
	static const auto basePath =
	    []() {
		    auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
		    Framework::PathUtils::EnsurePathExists(result);
		    return result;
	    }();
	return basePath;
}
