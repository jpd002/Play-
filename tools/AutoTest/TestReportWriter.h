#pragma once

#include <string>
#include <memory>

struct LINEDIFF
{
	std::string expected;
	std::string result;
};

struct TESTRESULT
{
	typedef std::vector<LINEDIFF> LineDiffArray;

	bool succeeded = false;
	LineDiffArray lineDiffs;
};

class CTestReportWriter
{
public:
	virtual			~CTestReportWriter() {}
	
	virtual void	ReportTestEntry(const std::string& name, const TESTRESULT&) = 0;
	virtual void	Write(const boost::filesystem::path& reportPath) = 0;
};

typedef std::shared_ptr<CTestReportWriter> TestReportWriterPtr;
