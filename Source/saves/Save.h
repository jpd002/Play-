#ifndef _SAVE_H_
#define _SAVE_H_

#include "filesystem_def.h"
#include <string>
#include <ctime>
#include "Types.h"
#include "Stream.h"

class CSave
{
public:
	enum ICONTYPE
	{
		ICON_NORMAL,
		ICON_DELETING,
		ICON_COPYING,
	};

	CSave(const fs::path&);
	virtual ~CSave() = default;

	const wchar_t* GetName() const;
	const char* GetId() const;
	unsigned int GetSize() const;

	fs::path GetPath() const;
	fs::path GetIconPath(const ICONTYPE&) const;
	fs::path GetNormalIconPath() const;
	fs::path GetDeletingIconPath() const;
	fs::path GetCopyingIconPath() const;

	size_t GetSecondLineStartPosition() const;
	time_t GetLastModificationTime() const;

private:
	void ReadName(Framework::CStream&);

	fs::path m_basePath;
	std::wstring m_sName;
	std::string m_sId;
	std::string m_sNormalIconFileName;
	std::string m_sDeletingIconFileName;
	std::string m_sCopyingIconFileName;
	uint16 m_nSecondLineStartPosition;
	time_t m_nLastModificationTime;
};

#endif
