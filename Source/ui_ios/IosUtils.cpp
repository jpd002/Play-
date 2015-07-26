#include "IosUtils.h"

bool IosUtils::IsLoadableExecutableFileName(NSString* fileName)
{
	NSString* extension = [fileName pathExtension];
	if([extension caseInsensitiveCompare: @"elf"] == NSOrderedSame)
	{
		return true;
	}
	return false;
}

bool IosUtils::IsLoadableDiskImageFileName(NSString* fileName)
{
	NSString* extension = [fileName pathExtension];
	if(
		([extension caseInsensitiveCompare: @"iso"] == NSOrderedSame) ||
		([extension caseInsensitiveCompare: @"isz"] == NSOrderedSame) ||
		([extension caseInsensitiveCompare: @"cso"] == NSOrderedSame) ||
		([extension caseInsensitiveCompare: @"bin"] == NSOrderedSame)
	)
	{
		return true;
	}
	return false;
}
