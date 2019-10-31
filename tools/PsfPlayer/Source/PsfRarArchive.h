#pragma once

#include "PsfArchive.h"

class CPsfRarArchive : public CPsfArchive
{
public:
	CPsfRarArchive();
	virtual ~CPsfRarArchive();

	virtual void Open(const fs::path&) override;
	virtual void ReadFileContents(const char*, void*, unsigned int) override;

private:
	void* m_archive;
};
