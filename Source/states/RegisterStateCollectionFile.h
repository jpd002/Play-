#pragma once

#include <map>
#include "zip/ZipFile.h"
#include "RegisterState.h"

class CRegisterStateCollectionFile : public Framework::CZipFile
{
public:
	typedef std::map<std::string, CRegisterState> RegisterStateMap;
	typedef RegisterStateMap::const_iterator RegisterStateIterator;

	CRegisterStateCollectionFile(const char*);
	CRegisterStateCollectionFile(Framework::CStream&);
	virtual ~CRegisterStateCollectionFile() = default;

	void InsertRegisterState(const char*, CRegisterState);
	void Read(Framework::CStream&);
	void Write(Framework::CStream&) override;

	RegisterStateIterator begin() const;
	RegisterStateIterator end() const;

private:
	RegisterStateMap m_registerStates;
};
