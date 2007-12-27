#ifndef _PATHUTILS_H_
#define _PATHUTILS_H_

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace Framework
{
	namespace PathUtils
	{
		template<typename String, typename Traits>
		void EnsurePathExists(const boost::filesystem::basic_path<String, Traits>& path)
		{
			typedef boost::filesystem::basic_path<String, Traits> PathType;
			PathType buildPath;
			for(typename PathType::iterator pathIterator(path.begin());
				pathIterator != path.end(); pathIterator++)
			{
				buildPath /= (*pathIterator);
				if(!boost::filesystem::exists(buildPath))
				{
					boost::filesystem::create_directory(buildPath);
				}
			}		
		}
	}
}

#endif
