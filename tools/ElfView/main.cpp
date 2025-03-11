#include <QApplication>
#include "AppConfig.h"
#include "PathUtils.h"
#include "mainwindow.h"

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	QApplication a(argc, argv);

	MainWindow w;
	w.show();

	return a.exec();
}

fs::path CAppConfig::GetBasePath() const
{
	static const char* BASE_DATA_PATH = "ElfView Data Files";
	static const auto basePath =
	    []() {
		    auto result = Framework::PathUtils::GetPersonalDataPath() / BASE_DATA_PATH;
		    Framework::PathUtils::EnsurePathExists(result);
		    return result;
	    }();
	return basePath;
}
