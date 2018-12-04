#include "QStringUtils.h"

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

boost::filesystem::path QStringToPath(const QString& str)
{
	auto result = CvtToNativePath<boost::filesystem::path::string_type>(str);
	return boost::filesystem::path(result);
}

QString PathToQString(const boost::filesystem::path& path)
{
	return CvtToString(path.native());
}
