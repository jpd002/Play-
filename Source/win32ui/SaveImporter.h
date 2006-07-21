#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include <iostream>
#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>
#include "Types.h"

class CSaveImporter
{
public:
	enum OVERWRITE_PROMPT_RETURN
	{
		OVERWRITE_YES,
		OVERWRITE_YESTOALL,
		OVERWRITE_NO,
	};

	typedef boost::function< OVERWRITE_PROMPT_RETURN (const std::string&) > OverwritePromptFunctionType;

	static void						ImportSave(std::istream&, const char*, OverwritePromptFunctionType);

private:

									CSaveImporter(OverwritePromptFunctionType);
									~CSaveImporter();

	bool							CanExtractFile(const boost::filesystem::path&);

	void							Import(std::istream&, const char*);
	void							XPS_Import(std::istream&, const char*);
	void							XPS_ExtractFiles(std::istream&, const char*, uint32);
	void							PSU_Import(std::istream&, const char*);

	OverwritePromptFunctionType		m_OverwritePromptFunction;
	bool							m_nOverwriteAll;
};

#endif
