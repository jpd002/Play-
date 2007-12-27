#include "PathUtils.h"
#include <boost/filesystem/operations.hpp>

using namespace boost;
using namespace std;
/*
namespace Framework
{
	namespace PathUtils
	{
		void EnsurePathExists(const filesystem::path& path)
		{
			filesystem::path buildPath;
			for(filesystem::path::iterator pathIterator(path.begin());
				pathIterator != path.end(); pathIterator++)
			{
				buildPath /= (*pathIterator);
				if(!filesystem::exists(buildPath))
				{
					filesystem::create_directory(buildPath);
				}
			}
		}
	}
}
*/