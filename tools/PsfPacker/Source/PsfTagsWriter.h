#ifndef _PSFTAGSWRITER_H_
#define _PSFTAGSWRITER_H_

#include "Stream.h"
#include "PsfTagsDescription.h"

namespace PsfTags
{
	class CWriter
	{
	public:
		static void					Write(Framework::CStream&, const TagList&);

	private:

	};
};

#endif
