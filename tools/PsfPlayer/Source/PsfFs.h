#ifndef _PSFFS_H_
#define _PSFFS_H_

#include "PsfBase.h"
#include <list>

class CPsfFs
{
public:
				                    CPsfFs();
	virtual		                    ~CPsfFs();

	void							AppendArchive(const CPsfBase&);
    Framework::CStream*				GetFile(const char*);

private:
    struct NODE
    {
		NODE() :
			offset(0),
			size(0),
			blockSize(0)
		{
			name[0] = 0;
		}

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

    static void                     ReadFile(Framework::CStream&, FILE&);
    static void                     ReadDirectory(Framework::CStream&, DIRECTORY&);

    const FILE*                     GetFileDetail(const DIRECTORY&, const char*) const;
    static NODE*					GetFileFindNode(const DIRECTORY&, const char*);

    DIRECTORY                       m_root;
};

#endif
