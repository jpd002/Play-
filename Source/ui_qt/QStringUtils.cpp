#include "QStringUtils.h"
#include <QDir>

template <typename Type>
static Type CvtToNativePath(const QString& str);

template <typename Type>
static QString CvtToString(const Type& str);

template <>
std::string CvtToNativePath(const QString& str)
{
	return str.toStdString();
}

template <>
std::wstring CvtToNativePath(const QString& str)
{
	return str.toStdWString();
}

template <>
QString CvtToString(const std::string& str)
{
	return QString::fromStdString(str);
}

template <>
QString CvtToString(const std::wstring& str)
{
	return QString::fromStdWString(str);
}

fs::path QStringToPath(const QString& str)
{
	auto nativeStr = QDir::toNativeSeparators(str);
	auto result = CvtToNativePath<fs::path::string_type>(nativeStr);
	return fs::path(result);
}

QString PathToQString(const fs::path& path)
{
	return CvtToString(path.native());
}
