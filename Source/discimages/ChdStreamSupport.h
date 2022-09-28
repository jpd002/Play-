#pragma once

namespace Framework
{
	class CStream;
};
typedef struct chd_core_file core_file;

namespace ChdStreamSupport
{
	core_file* CreateFileFromStream(Framework::CStream*);
}
