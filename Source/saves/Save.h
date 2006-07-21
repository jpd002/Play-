#ifndef _SAVE_H_
#define _SAVE_H_

#include <boost/filesystem/path.hpp>
#include <string>
#include <ctime>
#include "Types.h"

class CSave
{
public:
								CSave(boost::filesystem::path&);
								~CSave();
	const wchar_t*				GetName() const;
	const char*					GetId() const;
	unsigned int				GetSize() const;
	boost::filesystem::path		GetPath() const;
	boost::filesystem::path		GetNormalIconPath() const;
	size_t						GetSecondLineStartPosition() const;
	time_t						GetLastModificationTime() const;

private:
	void						ReadName(std::istream&);

	boost::filesystem::path		m_BasePath;
	std::wstring				m_sName;
	std::string					m_sId;
	std::string					m_sNormalIconFileName;
	uint16						m_nSecondLineStartPosition;
	time_t						m_nLastModificationTime;
};

#endif
