#ifndef _PSFFS_H_
#define _PSFFS_H_

#include <string>
#include <list>
#include "Stream.h"

class CPsfFs
{
public:
    struct NODE
    {
        virtual ~NODE()
        {

        }

        bool IsDirectory() const
        {
            return (size == 0 && blockSize == 0 && offset != 0);
        }

        char name[37];
        uint32 offset;
        uint32 size;
        uint32 blockSize;
    };

    struct FILE : public NODE
    {
        FILE(const NODE& baseNode) :
        NODE(baseNode)
        {
            data = new uint8[baseNode.size];
        }

        virtual ~FILE()
        {
            delete [] data;
        }

        uint8* data;
    };

    struct DIRECTORY : public NODE
    {
        typedef std::list<NODE*> FileListType;

        DIRECTORY()
        {

        }

        DIRECTORY(const NODE& baseNode) :
        NODE(baseNode)
        {

        }

        virtual ~DIRECTORY()
        {
            for(FileListType::const_iterator nodeIterator(fileList.begin());
                fileList.end() != nodeIterator; nodeIterator++)
            {
                delete (*nodeIterator);
            }
        }

        FileListType fileList;
    };

                    CPsfFs(const char*);
    virtual         ~CPsfFs();
    const FILE*     GetFile(const char*) const;

private:

    void            ReadFile(Framework::CStream&, FILE&);
    void            ReadDirectory(Framework::CStream&, DIRECTORY&);
    const FILE*     GetFileDetail(const DIRECTORY&, const char*) const;
    const NODE*     GetFileFindNode(const DIRECTORY&, const char*) const;
    std::string     GetPsfLibAttributeName(unsigned int);

    std::string     m_tag;
    DIRECTORY       m_root;
};

#endif
