#pragma once

#include <boost/filesystem.hpp>
#include "xml/Node.h"
#include "TestReportWriter.h"

class CJUnitTestReportWriter : public CTestReportWriter
{
public:
								CJUnitTestReportWriter();
	virtual						~CJUnitTestReportWriter();

	void						ReportTestEntry(const std::string&, const TESTRESULT&) override;
	void						Write(const boost::filesystem::path&) override;

private:
	typedef std::unique_ptr<Framework::Xml::CNode> NodePtr;

	NodePtr						m_reportNode;
	Framework::Xml::CNode*		m_testSuiteNode = nullptr;
	unsigned int				m_testCount = 0;
};
