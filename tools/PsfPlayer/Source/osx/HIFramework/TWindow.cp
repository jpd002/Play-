/*
    File:		TWindow.cp

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

#include "TWindow.h"
#include "TNib.h"

// -----------------------------------------------------------------------------
//	constants
// -----------------------------------------------------------------------------
//

// Our base class for creating a Mac OS window which can be Carbon Event-savvy.

// -----------------------------------------------------------------------------
//	TWindow private constructor
// -----------------------------------------------------------------------------
//
TWindow::TWindow(
	CFStringRef			inName,
	CFStringRef			inNibName )
{
	fWindow = NULL;

	Init( inName, inNibName );
}

// -----------------------------------------------------------------------------
//	TWindow private constructor
// -----------------------------------------------------------------------------
//
TWindow::TWindow(
	WindowRef			inWindow )
{
	fWindow = NULL;
}

// -----------------------------------------------------------------------------
//	TWindow destructor
// -----------------------------------------------------------------------------
//
TWindow::~TWindow()
{
	if ( fWindow != NULL )
		DisposeWindow( fWindow );
}

// -----------------------------------------------------------------------------
//	Init (with WindowRef)
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::Init(
	WindowRef			inWindow )
{
	OSStatus			err;
	static const
	EventTypeSpec		kEvents[] =
							{
								{ kEventClassWindow, kEventWindowClose },
								{ kEventClassWindow, kEventWindowActivated },
								{ kEventClassWindow, kEventWindowDeactivated },
								{ kEventClassWindow, kEventWindowDispose },
								{ kEventClassWindow, kEventWindowDrawContent },
								{ kEventClassWindow, kEventWindowBoundsChanged },
								{ kEventClassWindow, kEventWindowGetIdealSize },
								{ kEventClassWindow, kEventWindowGetMinimumSize },
								{ kEventClassWindow, kEventWindowGetMaximumSize },
								{ kEventClassCommand, kEventCommandProcess },
								{ kEventClassCommand, kEventCommandUpdateStatus },
							};
	TWindow*			tWindow;

	require_action( inWindow != NULL, InvalidWindow, err = paramErr );

	fWindow = inWindow;

	err = RegisterForEvents( GetEventTypeCount( kEvents ), kEvents );
	require_noerr( err, CantRegisterForEvents );
	
	tWindow = this;
	err = SetWindowProperty( inWindow, 'hifr', 'twin', sizeof( TWindow* ), &tWindow );

CantRegisterForEvents:
InvalidWindow:
	return err;
}

// -----------------------------------------------------------------------------
//	Init (from Nib)
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::Init(
	CFStringRef			inWindowName,
	CFStringRef			inNibName )
{
	OSStatus			err;
	TNib				nib;
	WindowRef			window;

	err = nib.Init( inNibName );
	require_noerr( err, CantInitNib );

	err = nib.CreateWindow( inWindowName, &window );
	require_noerr( err, CantCreateWindow );

	err = Init( window );

CantCreateWindow:
CantInitNib:
	return err;
}

// -----------------------------------------------------------------------------
//	Init
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::Init(
	WindowClass			inClass,
	WindowAttributes	inAttributes,
	const Rect&			inBounds )
{
	OSStatus			err;
	WindowRef			window;

	err = CreateNewWindow( inClass, inAttributes, &inBounds, &window );
	require_noerr( err, CantCreateWindow );

	err = Init( window );

CantCreateWindow:
	return err;
}

// -----------------------------------------------------------------------------
//	GetEventTarget
// -----------------------------------------------------------------------------
//
EventTargetRef
TWindow::GetEventTarget()
{
	return GetWindowEventTarget( GetWindowRef() );
}

// -----------------------------------------------------------------------------
//	SourceWindowDispose
// -----------------------------------------------------------------------------
//
void
TWindow::SourceWindowDispose()
{
	fWindow = NULL;
}

// -----------------------------------------------------------------------------
//	Close
// -----------------------------------------------------------------------------
//
void
TWindow::Close()
{
	Hide();
	delete this;
}

// -----------------------------------------------------------------------------
//	GetPort
// -----------------------------------------------------------------------------
//
CGrafPtr
TWindow::GetPort() const
{
	return GetWindowPort( GetWindowRef() );
}

// -----------------------------------------------------------------------------
//	SetTitle
// -----------------------------------------------------------------------------
//
void
TWindow::SetTitle(
	CFStringRef			inTitle )
{
	SetWindowTitleWithCFString( GetWindowRef(), inTitle );
}

// -----------------------------------------------------------------------------
//	CopyTitle
// -----------------------------------------------------------------------------
//
CFStringRef
TWindow::CopyTitle() const
{
	CFStringRef			outTitle;

	CopyWindowTitleAsCFString( GetWindowRef(), &outTitle );

	return outTitle;
}

// -----------------------------------------------------------------------------
//	SetAlternateTitle
// -----------------------------------------------------------------------------
//
void
TWindow::SetAlternateTitle(
	CFStringRef			inTitle )
{
	SetWindowAlternateTitle( GetWindowRef(), inTitle );
}

// -----------------------------------------------------------------------------
//	CopyAlternateTitle
// -----------------------------------------------------------------------------
//
CFStringRef
TWindow::CopyAlternateTitle() const
{
	CFStringRef	outTitle;

	CopyWindowAlternateTitle( GetWindowRef(), &outTitle );

	return outTitle;
}

// -----------------------------------------------------------------------------
//	Show
// -----------------------------------------------------------------------------
//
void
TWindow::Show()
{
	ShowWindow( GetWindowRef() );

	if ( GetWindowRef() == ActiveNonFloatingWindow() )
	{
		WindowActivationScope		scope;

		GetWindowActivationScope( GetWindowRef(), &scope );
		if ( scope == kWindowActivationScopeAll )
			AdvanceKeyboardFocus( GetWindowRef() );
	}
}

// -----------------------------------------------------------------------------
//	Hide
// -----------------------------------------------------------------------------
//
void
TWindow::Hide()
{
	HideWindow( GetWindowRef() );
}

// -----------------------------------------------------------------------------
//	IsVisible
// -----------------------------------------------------------------------------
//
bool
TWindow::IsVisible() const
{
	return IsWindowVisible( GetWindowRef() );
}

// -----------------------------------------------------------------------------
//	Select
// -----------------------------------------------------------------------------
//
void
TWindow::Select()
{
	SelectWindow( GetWindowRef() );
}

// -----------------------------------------------------------------------------
//	Draw
// -----------------------------------------------------------------------------
//
void
TWindow::Draw()
{
}

// -----------------------------------------------------------------------------
//	Activated
// -----------------------------------------------------------------------------
//
void
TWindow::Activated()
{
}

// -----------------------------------------------------------------------------
//	Deactivated
// -----------------------------------------------------------------------------
//
void
TWindow::Deactivated()
{
}

// -----------------------------------------------------------------------------
//	Moved
// -----------------------------------------------------------------------------
//
void
TWindow::Moved()
{
}

// -----------------------------------------------------------------------------
//	Resized
// -----------------------------------------------------------------------------
//
void
TWindow::Resized()
{
}

// -----------------------------------------------------------------------------
//	GetIdealSize
// -----------------------------------------------------------------------------
//
Point
TWindow::GetIdealSize()
{
	Point idealSize = { 0, 0 };

	return idealSize;
}

// -----------------------------------------------------------------------------
//	GetMinimumSize
// -----------------------------------------------------------------------------
//
Point
TWindow::GetMinimumSize()
{
	Point minSize = { 0, 0 };

	return minSize;
}

// -----------------------------------------------------------------------------
//	GetMaximumSize
// -----------------------------------------------------------------------------
//
Point
TWindow::GetMaximumSize()
{
	Point maxSize = { 0, 0 };

	return maxSize;
}

// -----------------------------------------------------------------------------
//	SetBounds
// -----------------------------------------------------------------------------
//
void
TWindow::SetBounds(
	const Rect&			inBounds )
{
	SetWindowBounds( GetWindowRef(), kWindowStructureRgn, &inBounds );
}

// -----------------------------------------------------------------------------
//	InvalidateArea
// -----------------------------------------------------------------------------
//
void
TWindow::InvalidateArea(
	RgnHandle			inRegion )
{
	InvalWindowRgn( GetWindowRef(), inRegion );
}

// -----------------------------------------------------------------------------
//	InvalidateArea
// -----------------------------------------------------------------------------
//
void
TWindow::InvalidateArea(
	const Rect&			inRect )
{
	InvalWindowRect( GetWindowRef(), &inRect );
}

// -----------------------------------------------------------------------------
//	ValidateArea
// -----------------------------------------------------------------------------
//
void
TWindow::ValidateArea(
	RgnHandle			inRegion )
{
	ValidWindowRgn( GetWindowRef(), inRegion );
}

// -----------------------------------------------------------------------------
//	ValidateArea
// -----------------------------------------------------------------------------
//
void
TWindow::ValidateArea(
	const Rect&			inRect )
{
	ValidWindowRect( GetWindowRef(), &inRect );
}

// -----------------------------------------------------------------------------
//	UpdateCommandStatus
// -----------------------------------------------------------------------------
//
Boolean
TWindow::UpdateCommandStatus(
	const HICommandExtended&	inCommand )
{
	if ( inCommand.commandID == kHICommandClose )
	{
		EnableMenuCommand( NULL, inCommand.commandID );
		return true;
	}

	return false; // not handled
}

// -----------------------------------------------------------------------------
//	HandleCommand
// -----------------------------------------------------------------------------
//
Boolean
TWindow::HandleCommand(
	const HICommandExtended&		inCommand )
{
	return false; // not handled
}

// -----------------------------------------------------------------------------
//	SetDefaultButton
// -----------------------------------------------------------------------------
//
void
TWindow::SetDefaultButton(
	HIViewRef			inView )
{
	SetWindowDefaultButton( GetWindowRef(), inView );
}

// -----------------------------------------------------------------------------
//	SetCancelButton
// -----------------------------------------------------------------------------
//
void
TWindow::SetCancelButton(
	HIViewRef			inView )
{
	SetWindowCancelButton( GetWindowRef(), inView );
}

// -----------------------------------------------------------------------------
//	HandleEvent
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::HandleEvent(
	EventHandlerCallRef		inRef,
	TCarbonEvent&			inEvent )
{
	OSStatus				result = eventNotHandledErr;
	UInt32					attributes;
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

		case kEventClassWindow:
			switch ( inEvent.GetKind() )
			{
				case kEventWindowActivated:
					CallNextEventHandler( inRef, inEvent.GetEventRef() );
					this->Activated();
					result = noErr;
					break;

				case kEventWindowBoundsChanged:
					if ( inEvent.GetParameter( kEventParamAttributes, &attributes ) )
					{
						if ( attributes & kWindowBoundsChangeSizeChanged )
						{
							this->Resized();
							result = noErr;
						}
						else if ( attributes & kWindowBoundsChangeOriginChanged )
						{
							this->Moved();
							result = noErr;
						}
					}
					break;

				case kEventWindowClose:
					this->Close();
					result = noErr;
					break;

				case kEventWindowDeactivated:
					CallNextEventHandler( inRef, inEvent.GetEventRef() );
					this->Deactivated();
					result = noErr;
					break;

				case kEventWindowDispose:
					CallNextEventHandler( inRef, inEvent.GetEventRef() );
					this->SourceWindowDispose();
					result = noErr;
					break;

				case kEventWindowDrawContent:
					CallNextEventHandler( inRef, inEvent.GetEventRef() );
					this->Draw();
					result = noErr;
					break;

				case kEventWindowGetIdealSize:
					{
						Point	size = this->GetIdealSize();

						if ( (size.h != 0) && (size.v != 0) )
						{
							inEvent.SetParameter( kEventParamDimensions, size );
							result = noErr;
						}
					}
					break;

				case kEventWindowGetMinimumSize:
					{
						Point	size = this->GetMinimumSize();

						if ( (size.h != 0) && (size.v != 0) )
						{
							inEvent.SetParameter( kEventParamDimensions, size );
							result = noErr;
						}
					}
					break;

				case kEventWindowGetMaximumSize:
					{
						Point	size = this->GetIdealSize();

						if ( (size.h != 0) && (size.v != 0) )
						{
							inEvent.SetParameter( kEventParamDimensions, size );
							result = noErr;
						}
					}
					break;
			}
	};

	return result;
}

// -----------------------------------------------------------------------------
//	GetView
// -----------------------------------------------------------------------------
//
HIViewRef
TWindow::GetView(
	ControlID				inID )
{
	HIViewRef				view;

	if ( GetControlByID( GetWindowRef(), &inID, &view ) != noErr )
		view = NULL;

	return view;
}

// -----------------------------------------------------------------------------
//	GetView
// -----------------------------------------------------------------------------
//
HIViewRef
TWindow::GetView(
	OSType					inSignature,
	SInt32					inID  )
{
	HIViewRef				view;
	ControlID				viewID = { inSignature, inID };

	view = GetView( viewID );

	return view;
}

// -----------------------------------------------------------------------------
//	EnableView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::EnableView(
	ControlID				inID )
{
	OSStatus				err = noErr;
	HIViewRef				view;

	view = GetView( inID );
	require_action( view != NULL, CantGetView, err = errUnknownControl );

	EnableControl( view );

CantGetView:
	return err;
}

// -----------------------------------------------------------------------------
//	EnableView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::EnableView(
	OSType					inSignature,
	SInt32					inID  )
{
	OSStatus				err;
	ControlID				viewID = { inSignature, inID };

	err = EnableView( viewID );

	return err;
}

// -----------------------------------------------------------------------------
//	DisableView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::DisableView(
	ControlID				inID )
{
	OSStatus				err = noErr;
	HIViewRef				view;

	view = GetView( inID );
	require_action( view != NULL, CantGetView, err = errUnknownControl );

	DisableControl( view );

CantGetView:
	return err;
}

// -----------------------------------------------------------------------------
//	DisableView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::DisableView(
	OSType					inSignature,
	SInt32					inID  )
{
	OSStatus				err;
	ControlID				viewID = { inSignature, inID };

	err = DisableView( viewID );

	return err;
}

// -----------------------------------------------------------------------------
//	ShowView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::ShowView(
	ControlID				inID )
{
	OSStatus				err;
	HIViewRef				view;

	view = GetView( inID );
	require_action( view != NULL, CantGetView, err = errUnknownControl );

	ShowControl( view );

CantGetView:
	return err;
}

// -----------------------------------------------------------------------------
//	ShowView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::ShowView(
	OSType					inSignature,
	SInt32					inID  )
{
	OSStatus				err;
	ControlID				viewID = { inSignature, inID };

	err = ShowView( viewID );

	return err;
}

// -----------------------------------------------------------------------------
//	HideView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::HideView(
	ControlID				inID )
{
	HIViewRef				view;
	OSStatus				err;

	view = GetView( inID );
	require_action( view != NULL, CantGetView, err = errUnknownControl );

	HideControl( view );

CantGetView:
	return err;
}

// -----------------------------------------------------------------------------
//	HideView
// -----------------------------------------------------------------------------
//
OSStatus
TWindow::HideView(
	OSType					inSignature,
	SInt32					inID  )
{
	OSStatus				err;
	ControlID				viewID = { inSignature, inID };

	err = HideView( viewID );

	return err;
}

// --------------------------------------------------------------------------------
//	GetTWindowFromWindowRef
// --------------------------------------------------------------------------------
//
TWindow*
TWindow::GetTWindowFromWindowRef(
	WindowRef				inWindowRef )
{
	TWindow*				tWindow = NULL;
	verify_noerr( GetWindowProperty( inWindowRef, 'hifr', 'twin', sizeof( TWindow* ), NULL, &tWindow ) );
	return tWindow;
}
