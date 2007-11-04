/*
    File:		TNib.cp

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

#include "TNib.h"

// Our base class for creating a Mac OS nib.

//-----------------------------------------------------------------------------------
//	TNib constructors
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//	TNib private constructor
//-----------------------------------------------------------------------------------
//
TNib::TNib()
{
	fNib = NULL;
}

//-----------------------------------------------------------------------------------
//	TNib Nib creation constructor
//-----------------------------------------------------------------------------------
//
TNib::TNib(
	CFStringRef			inNibName )
{
	fNib = NULL;
	verify_noerr( Init( inNibName ) );
}

//-----------------------------------------------------------------------------------
//	TNib Nib creation constructor
//-----------------------------------------------------------------------------------
//
TNib::TNib(
	IBNibRef			inNib )
{
	fNib = NULL;
	verify_noerr( Init( inNib ) );
}

//-----------------------------------------------------------------------------------
//	TNib destructor
//-----------------------------------------------------------------------------------
//
TNib::~TNib()
{
	if ( fNib != NULL )
		DisposeNibReference( fNib );
}

//-----------------------------------------------------------------------------------
//	Init
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::Init(
	IBNibRef			inNib )
{
	fNib = inNib;

	return noErr;
}

//-----------------------------------------------------------------------------------
//	Init
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::Init(
	CFStringRef			inNibName )
{
	IBNibRef			nibRef;
	OSStatus			err;

	err = CreateNibReference( inNibName, &nibRef );
	require_noerr( err, CantCreateNibRef );

	err = Init( nibRef );

CantCreateNibRef:
	return err;
}

//-----------------------------------------------------------------------------------
//	CreateWindow
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::CreateWindow(
	CFStringRef			inName,
	WindowRef*			outWindow )
{
	return CreateWindowFromNib( fNib, inName, outWindow );
}

//-----------------------------------------------------------------------------------
//	CreateMenu
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::CreateMenu(
	CFStringRef			inMenuName,
	MenuRef*			outMenu )
{
	return CreateMenuFromNib( fNib, inMenuName, outMenu );
}

//-----------------------------------------------------------------------------------
//	CreateMenu
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::CreateMenuBar(
	CFStringRef			inMenuBarName,
	MenuBarHandle*		outMenuBar )
{
	return CreateMenuBarFromNib( fNib, inMenuBarName, outMenuBar );
}

//-----------------------------------------------------------------------------------
//	SetMenuBar
//-----------------------------------------------------------------------------------
//
OSStatus
TNib::SetMenuBar(
	CFStringRef			inMenuBarName )
{
	return SetMenuBarFromNib( fNib, inMenuBarName );
}

//-----------------------------------------------------------------------------------
//	operator IBNibRef
//-----------------------------------------------------------------------------------
//
TNib::operator
IBNibRef() const
{
	return fNib;
}
