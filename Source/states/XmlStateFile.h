#pragma once

#include <memory>
#include "zip/ZipFile.h"
#include "xml/Node.h"

class CXmlStateFile : public Framework::CZipFile
{
public:
	CXmlStateFile(const char*, const char*);
	CXmlStateFile(Framework::CStream&);

	Framework::Xml::CNode* GetRoot() const;

	void Read(Framework::CStream&);
	void Write(Framework::CStream&) override;

private:
	std::unique_ptr<Framework::Xml::CNode> m_root;
};
