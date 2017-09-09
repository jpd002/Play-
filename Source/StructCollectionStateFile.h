#pragma once

#include <map>
#include "zip/ZipFile.h"
#include "StructFile.h"
#include "uint128.h"

class CStructCollectionStateFile : public Framework::CZipFile
{
public:
	typedef std::map<std::string, CStructFile> StructMap;
	typedef StructMap::const_iterator StructIterator;

						CStructCollectionStateFile(const char*);
						CStructCollectionStateFile(Framework::CStream&);
	virtual				~CStructCollectionStateFile() = default;

	void				InsertStruct(const char*, const CStructFile&);
	void				Read(Framework::CStream&);
	void				Write(Framework::CStream&) override;

	StructIterator		GetStructBegin() const;
	StructIterator		GetStructEnd() const;

private:
	StructMap			m_structs;
};

