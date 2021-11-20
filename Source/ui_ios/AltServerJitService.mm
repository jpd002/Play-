#import "AltServerJitService.h"
#import "AltKit-Swift.h"
#include "../AppConfig.h"
#import "PreferenceDefs.h"

@implementation AltServerJitService

- (id)init
{
	if(self = [super init])
	{
		[self registerPreferences];
	}
	return self;
}

+ (AltServerJitService*)sharedAltServerJitService
{
	static AltServerJitService* sharedInstance = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
	  sharedInstance = [[self alloc] init];
	});
	return sharedInstance;
}

- (void)registerPreferences
{
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREFERENCE_ALTSTORE_JIT_ENABLED, false);
}

- (void)startProcess
{
	//Don't start the process if it's not enabled
	if(!CAppConfig::GetInstance().GetPreferenceBoolean(PREFERENCE_ALTSTORE_JIT_ENABLED))
	{
		return;
	}

	//Don't start the process if we've already started it
	if(self.processStarted)
	{
		return;
	}

	self.processStarted = YES;

	[[ALTServerManager sharedManager] startDiscovering];

	[[ALTServerManager sharedManager] autoconnectWithCompletionHandler:^(ALTServerConnection* connection, NSError* error) {
	  if(error)
	  {
		  return NSLog(@"Could not auto-connect to server. %@", error);
	  }

	  [connection enableUnsignedCodeExecutionWithCompletionHandler:^(BOOL success, NSError* error) {
		if(success)
		{
			NSLog(@"Successfully enabled JIT compilation!");
			[[ALTServerManager sharedManager] stopDiscovering];
		}
		else
		{
			NSLog(@"Could not enable JIT compilation. %@", error);
		}

		[connection disconnect];
	  }];
	}];
}

@end
