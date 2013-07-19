/*
** filesys.mm
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "filesys.h"
#include "zstring.h"

#include <Cocoa/Cocoa.h>

namespace FileSys {

// Kind of hacking the ESpecialDirectory, but this function isn't really supposed to be widely used.
FString OSX_FindFolder(ESpecialDirectory dir)
{
	static const NSSearchPathDirectory DirToNS[NUM_SPECIAL_DIRECTORIES] =
	{
		NSApplicationDirectory,
		NSLibraryDirectory,
		NSDocumentDirectory,
		NSApplicationSupportDirectory,
		NSDocumentDirectory,
		NSDocumentDirectory
	};

	NSURL *url = [[NSFileManager defaultManager] URLForDirectory:DirToNS[dir] inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
	if(url == nil)
		return FString();
	if(dir == DIR_Configuration)
	{
		NSString *preferences = [[url path] stringByAppendingPathComponent:@"/Preferences"];
		return [preferences UTF8String];
	}
	return [[url path] UTF8String];
}

}
