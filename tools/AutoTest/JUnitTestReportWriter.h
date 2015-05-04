#pragma once

#include <boost/filesystem.hpp>
#include "xml/Node.h"
#include "TestReportWriter.h"

class CJUnitTestReportWriter : public CTestReportWriter
{
public:
								CJUnitTestReportWriter(const boost::filesystem::path&);
	virtual						~CJUnitTestReportWriter();

	void						ReportTestEntry(const std::string&, bool) override;

private:
	typedef std::unique_ptr<Framework::Xml::CNode> NodePtr;

	NodePtr						m_reportNode;
	Framework::Xml::CNode*		m_testSuiteNode = nullptr;
	boost::filesystem::path		m_reportPath;
	unsigned int				m_testCount = 0;
};
