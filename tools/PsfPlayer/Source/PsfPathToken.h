#pragma once

#include <string>

class CPsfPathToken
{
public:
						CPsfPathToken();
						CPsfPathToken(const std::wstring&);
						CPsfPathToken(const std::string&);

	static std::string	NarrowString(const std::wstring&);
	static std::wstring	WidenString(const std::string&);

	std::string			GetNarrowPath() const;
	std::wstring		GetWidePath() const;

	std::string			GetExtension() const;

private:
	std::wstring		m_path;
};
