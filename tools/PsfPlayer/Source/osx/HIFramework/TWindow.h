/*
	File:		TWindow.h

    Version:	Mac OS X

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
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

#ifndef TWindow_H_
#define TWindow_H_

#include <Carbon/Carbon.h>

#include "HIFramework.h"
#include "TEventHandler.h"

class TWindow
	:	public TEventHandler
{
public:
						TWindow(
							CFStringRef				inName,
							CFStringRef				inNibName = CFSTR( "main" ) );
						TWindow(
							WindowRef				inWindowRef = NULL );
	virtual				~TWindow();

	operator			WindowRef() const
							{ return GetWindowRef(); }
	WindowRef			GetWindowRef() const
							{ return fWindow; }

	CGrafPtr			GetPort() const;

	void				SetTitle(
							CFStringRef				inTitle );
	CFStringRef			CopyTitle() const;

	void				SetAlternateTitle(
							CFStringRef				inTitle );
	CFStringRef			CopyAlternateTitle() const;

	virtual void		Show();
	virtual void		Hide();
	bool				IsVisible() const;

	virtual void		Close();
	void				Select();

	void				Activate() const
							{ ::ActivateWindow( GetWindowRef(), true ); }
	void				Deactivate() const
							{ ::ActivateWindow( GetWindowRef(), false ); }
	void				BringToFront() const
							{ ::BringToFront( GetWindowRef() ); }
	
	void				SetBounds(
							const Rect&				inBounds );

	void				InvalidateArea(
							RgnHandle				inRegion );
	void				InvalidateArea(
							const Rect&				inRect );
	void				ValidateArea(
							RgnHandle				inRegion );
	void				ValidateArea(
							const Rect&				inRect );

	void				SetDefaultButton(
							HIViewRef				inView );
	void				SetCancelButton(
							HIViewRef				inView );

	HIViewRef			GetView(
							ControlID				inID );
	HIViewRef			GetView(
							OSType					inSignature,
							SInt32					inID = 0 );

	OSStatus			EnableView(
							ControlID				inID );
	OSStatus			DisableView(
							ControlID				inID );
	OSStatus			EnableView(
							OSType					inSignature,
							SInt32					inID = 0 );
	OSStatus			DisableView(
							OSType					inSignature,
							SInt32					inID = 0 );

	OSStatus			ShowView(
							ControlID				inID );
	OSStatus			HideView(
							ControlID				inID );
	OSStatus			ShowView(
							OSType					inSignature,
							SInt32					inID = 0 );
	OSStatus			HideView(
							OSType					inSignature,
							SInt32					inID = 0 );
	static TWindow*		GetTWindowFromWindowRef(
							WindowRef				inWindowRef );

protected:
	virtual void		Draw();

	virtual void		Activated();
	virtual void		Deactivated();

	virtual void		Moved();
	virtual void		Resized();

	virtual Point		GetIdealSize();
	virtual	Point		GetMinimumSize();
	virtual Point		GetMaximumSize();

	virtual Boolean		UpdateCommandStatus(
							const
							HICommandExtended&		inCommand );
	virtual Boolean		HandleCommand(
							const
							HICommandExtended&		inCommand );

	virtual OSStatus	HandleEvent(
							EventHandlerCallRef		inCallRef,
							TCarbonEvent&			inEvent );

	void				SourceWindowDispose();

	virtual
	EventTargetRef		GetEventTarget();

private:
	OSStatus			Init(
							CFStringRef				inName,
							CFStringRef				inNibName = CFSTR( "main" ) );
	OSStatus			Init(
							WindowClass				inClass,
							WindowAttributes		inAttributes,
							const Rect&				inBounds );
	OSStatus			Init(
							WindowRef				inWindow );

	WindowRef			fWindow;
};

#endif TWindow_H_