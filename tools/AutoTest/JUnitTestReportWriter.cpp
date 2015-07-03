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

void CJUnitTestReportWriter::ReportTestEntry(const std::string& testName, const TESTRESULT& result)
{
	auto testCaseNode = new Framework::Xml::CNode("testcase", true);
	testCaseNode->InsertAttribute("name", testName.c_str());

	if(!result.succeeded)
	{
		std::string failureDetails;
		for(const auto& lineDiff : result.lineDiffs)
		{
			auto failureLine = string_format(
				"result   : %s\r\n"
				"expected : %s\r\n"
				"\r\n",
				lineDiff.result.c_str(), lineDiff.expected.c_str());
			failureDetails += failureLine;
		}
		auto resultNode = new Framework::Xml::CNode("failure", true);
		resultNode->InsertTextNode(failureDetails.c_str());
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
