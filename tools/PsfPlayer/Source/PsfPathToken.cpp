#include <assert.h>
#include "PsfPathToken.h"

CPsfPathToken::CPsfPathToken()
{

}

CPsfPathToken::CPsfPathToken(const std::string& path)
: m_path(WidenString(path))
{

}

CPsfPathToken::CPsfPathToken(const std::wstring& path)
: m_path(path)
{

}

std::string CPsfPathToken::NarrowString(const std::wstring& input)
{
	std::string result;
	result.reserve(input.length());
	for(const auto& inputChar : input)
	{
		assert((inputChar & 0xFF00) == 0);
		result.push_back(static_cast<char>(inputChar));
	}
	return result;
}

std::wstring CPsfPathToken::WidenString(const std::string& input)
{
	std::wstring result;
	result.reserve(input.length());
	for(const auto& inputChar : input)
	{
		result.push_back(inputChar);
	}
	return result;
}

std::string CPsfPathToken::GetNarrowPath() const
{
	return NarrowString(m_path);
}

std::wstring CPsfPathToken::GetWidePath() const
{
	return m_path;
}

std::string CPsfPathToken::GetExtension() const
{
	auto dotPosition = m_path.find_last_of(L".");
	if(dotPosition == std::wstring::npos) return std::string();
	auto extensionToken = std::wstring(std::begin(m_path) + dotPosition + 1, std::end(m_path));
	auto extension = NarrowString(extensionToken);
	return extension;
}
