#pragma once

#include <string>
#include <memory>

class CTestReportWriter
{
public:
	virtual			~CTestReportWriter() {}
	
	virtual void	ReportTestEntry(const std::string& name, bool succeeded) = 0;
};

typedef std::shared_ptr<CTestReportWriter> TestReportWriterPtr;
