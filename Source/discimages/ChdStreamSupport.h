#pragma once

namespace Framework
{
	class CStream;
};
typedef struct _core_file core_file;

namespace ChdStreamSupport
{
	core_file* CreateFileFromStream(Framework::CStream*);
}
