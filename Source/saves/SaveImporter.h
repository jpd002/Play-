#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include <functional>
#include <boost/filesystem/path.hpp>
#include "Types.h"
#include "Stream.h"

class CSaveImporter
{
public:
	enum OVERWRITE_PROMPT_RETURN
	{
		OVERWRITE_YES,
		OVERWRITE_YESTOALL,
		OVERWRITE_NO,
	};

	typedef std::tr1::function< OVERWRITE_PROMPT_RETURN (const std::string&) > OverwritePromptFunctionType;

	static void						ImportSave(Framework::CStream&, const boost::filesystem::path&, const OverwritePromptFunctionType&);

private:

									CSaveImporter(const OverwritePromptFunctionType&);
	virtual							~CSaveImporter();

	bool							CanExtractFile(const boost::filesystem::path&);

	void							Import(Framework::CStream&, const boost::filesystem::path&);
	void							XPS_Import(Framework::CStream&, const boost::filesystem::path&);
	void							XPS_ExtractFiles(Framework::CStream&, const boost::filesystem::path&, uint32);
	void							PSU_Import(Framework::CStream&, const boost::filesystem::path&);

	OverwritePromptFunctionType		m_OverwritePromptFunction;
	bool							m_nOverwriteAll;
};

#endif
