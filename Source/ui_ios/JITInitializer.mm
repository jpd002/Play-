#import "JITInitializer.h"
#include "MemoryUtil.h"
#import <Foundation/Foundation.h>

@implementation JITInitializer

+ (BOOL)deviceHasTXM
{
	// Detect TXM (Trusted Execution Monitor) presence
	// Based on StikDebug implementation
	// Checks for: /System/Volumes/Preboot/<36 chars>/boot/<96 chars>/usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4

	NSFileManager* fileManager = [NSFileManager defaultManager];
	NSError* error = nil;

	// Primary path
	NSArray<NSString*>* prebootContents = [fileManager contentsOfDirectoryAtPath:@"/System/Volumes/Preboot" error:&error];
	if(prebootContents)
	{
		for(NSString* uuid in prebootContents)
		{
			if(uuid.length == 36)
			{
				NSString* bootPath = [NSString stringWithFormat:@"/System/Volumes/Preboot/%@/boot", uuid];
				NSArray<NSString*>* bootContents = [fileManager contentsOfDirectoryAtPath:bootPath error:nil];
				if(bootContents)
				{
					for(NSString* hash in bootContents)
					{
						if(hash.length == 96)
						{
							NSString* txmPath = [NSString stringWithFormat:@"%@/%@/usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4", bootPath, hash];
							if([fileManager fileExistsAtPath:txmPath])
							{
								return YES;
							}
						}
					}
				}
			}
		}
	}

	// Fallback path
	NSArray<NSString*>* privatePrebootContents = [fileManager contentsOfDirectoryAtPath:@"/private/preboot" error:nil];
	if(privatePrebootContents)
	{
		for(NSString* hash in privatePrebootContents)
		{
			if(hash.length == 96)
			{
				NSString* txmPath = [NSString stringWithFormat:@"/private/preboot/%@/usr/standalone/firmware/FUD/Ap,TrustedExecutionMonitor.img4", hash];
				if([fileManager fileExistsAtPath:txmPath])
				{
					return YES;
				}
			}
		}
	}

	return NO;
}

+ (void)initializeJITSystem
{
	NSLog(@"[JITInitializer] Initializing CodeGen JIT system...");

	// Detect iOS version and TXM status
	BOOL hasTXM = NO;
	CodeGen::JitType jitType;

	if(@available(iOS 26, *))
	{
		// iOS 26+
		hasTXM = [self deviceHasTXM];

		if(hasTXM)
		{
			// iOS 26+ with TXM: LuckTXM mode (most performant)
			NSLog(@"[JITInitializer] Configuring JIT: LuckTXM mode (iOS 26+ with TXM)");
			jitType = CodeGen::JitType::LuckTXM;
		}
		else
		{
			// iOS 26+ without TXM: LuckNoTXM mode
			NSLog(@"[JITInitializer] Configuring JIT: LuckNoTXM mode (iOS 26+ without TXM)");
			jitType = CodeGen::JitType::LuckNoTXM;
		}
	}
	else
	{
		// iOS < 26: Legacy mode
		NSLog(@"[JITInitializer] Configuring JIT: Legacy mode (iOS < 26)");
		jitType = CodeGen::JitType::Legacy;
	}

	// Configure CodeGen with detected mode
	CodeGen::SetJitType(jitType);

	// Pre-allocate executable memory region if using LuckTXM
	if(jitType == CodeGen::JitType::LuckTXM)
	{
		NSLog(@"[JITInitializer] Pre-allocating 512MB executable memory region...");
		CodeGen::AllocateExecutableMemoryRegion();
		NSLog(@"[JITInitializer] Memory region allocated successfully");
	}

	NSLog(@"[JITInitializer] CodeGen JIT system initialized successfully");
}

@end
