/*
    File:		TApplication.cp

    Version:	Mac OS X

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 2000-2005 Apple Computer, Inc., All Rights Reserved
*/

#include "TApplication.h"
#include "TNib.h"

// -----------------------------------------------------------------------------
//	constants
// -----------------------------------------------------------------------------
//

// Our base class for creating a Mac OS application which is be Carbon Event-savvy.

// -----------------------------------------------------------------------------
//	TApplication Nib creation constructor
// -----------------------------------------------------------------------------
//
TApplication::TApplication(
	CFStringRef				inMenuBarName,
	CFStringRef				inNibName )
{
	Init( inMenuBarName, inNibName );
}

// -----------------------------------------------------------------------------
//	Init
// -----------------------------------------------------------------------------
//
OSStatus
TApplication::Init(
	CFStringRef			inMenuBarName,
	CFStringRef			inNibName )
{
	OSStatus			err;
	TNib				nib;

	static const
	EventTypeSpec		kEvents[] =
							{
								{ kEventClassCommand, kEventProcessCommand },
							};

	err = RegisterForEvents( GetEventTypeCount( kEvents ), kEvents );
	require_noerr( err, CantInit );

	err = nib.Init( inNibName );
	require_noerr( err, CantInit );

	err = nib.SetMenuBar( inMenuBarName );
	require_noerr( err, CantInit );

CantInit:
	return err;
}

// -----------------------------------------------------------------------------
//	TApplication destructor
// -----------------------------------------------------------------------------
//
TApplication::~TApplication()
{
}

// -----------------------------------------------------------------------------
//	GetEventTarget
// -----------------------------------------------------------------------------
//
EventTargetRef
TApplication::GetEventTarget()
{
	return GetApplicationEventTarget();
}

// -----------------------------------------------------------------------------
//	Run
// -----------------------------------------------------------------------------
//
void
TApplication::Run()
{
	RunApplicationEventLoop();
}

// -----------------------------------------------------------------------------
//	Activated
// -----------------------------------------------------------------------------
//
void
TApplication::Activated()
{
}

// -----------------------------------------------------------------------------
//	Deactivated
// -----------------------------------------------------------------------------
//
void
TApplication::Deactivated()
{
}

// -----------------------------------------------------------------------------
//	UpdateCommandStatus
// -----------------------------------------------------------------------------
//
Boolean
TApplication::UpdateCommandStatus(
	const HICommandExtended&	inCommand )
{
	return false; // not handled
}

// -----------------------------------------------------------------------------
//	HandleCommand
// -----------------------------------------------------------------------------
//
Boolean
TApplication::HandleCommand(
	const HICommandExtended&	inCommand )
{
	return false; // not handled
}

// -----------------------------------------------------------------------------
//	HandleEvent
// -----------------------------------------------------------------------------
//
OSStatus
TApplication::HandleEvent(
	EventHandlerCallRef		inRef,
	TCarbonEvent&			inEvent )
{
	OSStatus				result = eventNotHandledErr;
	HICommandExtended		command;

	switch ( inEvent.GetClass() )
	{
		case kEventClassCommand:
			{
				inEvent.GetParameter( kEventParamDirectObject, &command );

				switch ( inEvent.GetKind() )
				{
					case kEventCommandProcess:
						if ( this->HandleCommand( command ) )
							result = noErr;
						break;

					case kEventCommandUpdateStatus:
						if ( this->UpdateCommandStatus( command ) )
							result = noErr;
						break;
				}
			}
			break;
	};

	return result;
}
