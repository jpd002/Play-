#import "ApplicationDelegate.h"
#include "../../Source/PsfLoader.h"
#include "../../Source/SH_OpenAL.h"

using namespace std;

string GetTagValue(const CPsfBase::TagMap& tags, const char* tagName)
{
	string result;
	CPsfBase::TagMap::const_iterator tagIterator(tags.find(tagName));
	if(tagIterator != tags.end())
	{
		result = tagIterator->second;
	}
	return result;
}

@implementation ApplicationDelegate

-(id)init
{
	m_virtualMachine = new CPsfVm();
	m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
	return [super init];
}

-(void)OnFileOpen: (id)sender
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObjects:@"psf", @"psf2", @"minipsf", @"minipsf2", nil];
	if([openPanel runModalForTypes:fileTypes] != NSOKButton)
	{
		return;
	}
	NSString* fileName = [openPanel filename];
	[self LoadPsf: fileName];
}

-(void)LoadPsf : (NSString*)fileName
{	
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	try
	{
		CPsfBase::TagMap tags;
		CPsfLoader::LoadPsf(*m_virtualMachine, [fileName fileSystemRepresentation], "", &tags);
		NSString* game  = [[NSString alloc] initWithUTF8String: GetTagValue(tags, "game").c_str()];
		NSString* title = [[NSString alloc] initWithUTF8String: GetTagValue(tags, "title").c_str()]; 
		NSString* length = [[NSString alloc] initWithUTF8String: GetTagValue(tags, "length").c_str()];
		PlaylistItem* item = [[PlaylistItem alloc] init: fileName game: game title: title length: length];
		[game release];
		[title release];
		[length release];
		[m_playlist addItem: item];
		[m_playListView reloadData];		
		m_virtualMachine->Resume();
	}
	catch(const exception& excep)
	{
		NSString* errorMessage = [[NSString alloc] initWithCString:excep.what()];
		NSRunCriticalAlertPanel(@"PSF load error:", errorMessage, NULL, NULL, NULL);
	}
}
						
@end
