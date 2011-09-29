#ifndef _STDSTREAMUTILS_H_
#define _STDSTREAMUTILS_H_

template <typename StringType>
static Framework::CStdStream* CreateInputStdStream(const StringType&);

template <typename StringType>
static Framework::CStdStream* CreateOutputStdStream(const StringType&);

template <>
static Framework::CStdStream* CreateInputStdStream(const std::wstring& path)
{
	return new Framework::CStdStream(path.c_str(), L"rb");
}

template <>
static Framework::CStdStream* CreateInputStdStream(const std::string& path)
{
	return new Framework::CStdStream(path.c_str(), "rb");
}

template <>
static Framework::CStdStream* CreateOutputStdStream(const std::wstring& path)
{
	return new Framework::CStdStream(path.c_str(), L"wb");
}

template <>
static Framework::CStdStream* CreateOutputStdStream(const std::string& path)
{
	return new Framework::CStdStream(path.c_str(), "wb");
}

#endif
