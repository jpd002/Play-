#ifndef _STRUCTCOLLECTIONSTATEFILE_H_
#define _STRUCTCOLLECTIONSTATEFILE_H_

#include <map>
#include "zip/ZipFile.h"
#include "StructFile.h"
#include "uint128.h"

class CStructCollectionStateFile : public CZipFile
{
public:
    typedef std::map<std::string, CStructFile> StructMap;	
    typedef StructMap::const_iterator StructIterator;	

                        CStructCollectionStateFile(const char*);
                        CStructCollectionStateFile(Framework::CStream&);
    virtual             ~CStructCollectionStateFile();

    void                InsertStruct(const char*, const CStructFile&);
    void                Read(Framework::CStream&);
    virtual void        Write(Framework::CStream&);

    StructIterator      GetStructBegin() const;
    StructIterator      GetStructEnd() const;

private:
    StructMap           m_structs;
};

#endif
