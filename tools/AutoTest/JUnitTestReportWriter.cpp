#include "JUnitTestReportWriter.h"
#include "StdStreamUtils.h"
#include "xml/Writer.h"
#include "string_format.h"

CJUnitTestReportWriter::CJUnitTestReportWriter()
{
	m_reportNode = std::make_unique<Framework::Xml::CNode>("", true);
	auto testSuitesNode = new Framework::Xml::CNode("testsuites", true);
	m_reportNode->InsertNode(testSuitesNode);

	m_testSuiteNode = new Framework::Xml::CNode("testsuite", true);
	testSuitesNode->InsertNode(m_testSuiteNode);
}

CJUnitTestReportWriter::~CJUnitTestReportWriter()
{

}

void CJUnitTestReportWriter::ReportTestEntry(const std::string& testName, bool succeeded)
{
	auto testCaseNode = new Framework::Xml::CNode("testcase", true);
	testCaseNode->InsertAttribute("name", testName.c_str());

	if(!succeeded)
	{
		auto resultNode = new Framework::Xml::CNode("failure", true);
		testCaseNode->InsertNode(resultNode);
	}

	m_testSuiteNode->InsertNode(testCaseNode);

	m_testCount++;
}

void CJUnitTestReportWriter::Write(const boost::filesystem::path& reportPath)
{
	m_testSuiteNode->InsertAttribute("tests", string_format("%d", m_testCount).c_str());
	auto testOutputFileStream = Framework::CreateOutputStdStream(reportPath.native());
	Framework::Xml::CWriter::WriteDocument(testOutputFileStream, m_reportNode.get());
}
