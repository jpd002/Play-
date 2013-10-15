#import "NSStringUtils.h"

NSString* stringWithWchar(const std::wstring& input)
{
	NSString* result = [[NSString alloc] initWithBytes: input.data()
												length: input.length() * sizeof(wchar_t)
											  encoding: NSUTF32LittleEndianStringEncoding];
	[result autorelease];
	return result;
}
