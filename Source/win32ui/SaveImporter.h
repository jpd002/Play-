#ifndef _SAVEIMPORTER_H_
#define _SAVEIMPORTER_H_

#include <iostream>
#include "Types.h"

class CSaveImporter
{
public:
	static void			ImportSave(std::istream&, const char*);

private:
	static void			XPS_Import(std::istream&, const char*);
	static void			XPS_ExtractFiles(std::istream&, const char*, uint32);
	static void			PSU_Import(std::istream&, const char*);
};

#endif
