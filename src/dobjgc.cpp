/*
** dobjgc.cpp
** The garbage collector. Based largely on Lua's.
**
**---------------------------------------------------------------------------
** Copyright 2008 Randy Heit
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
*/
/******************************************************************************
* Copyright (C) 1994-2008 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

// HEADER FILES ------------------------------------------------------------

#include "actor.h"
#include "dobject.h"
#include "templates.h"
#include "m_alloc.h"
#include "thinker.h"
//#include "b_bot.h"
//#include "p_local.h"
//#include "g_game.h"
//#include "a_sharedglobal.h"
//#include "sbar.h"
//#include "stats.h"
//#include "c_dispatch.h"
//#include "p_acs.h"
//#include "s_sndseq.h"
//#include "r_data/r_interpolate.h"
//#include "doomstat.h"
//#include "m_argv.h"
//#include "po_man.h"
#include "v_video.h"
//#include "menu/menu.h"
//#include "intermission/intermission.h"
#include "wl_agent.h"
#include "wl_net.h"
#include "thingdef/thingdef.h"
#include "id_ca.h"

// MACROS ------------------------------------------------------------------

/*
@@ DEFAULT_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/
#define DEFAULT_GCPAUSE		150	// 150% (wait for memory to increase by half before next GC)

/*
@@ DEFAULT_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define DEFAULT_GCMUL		400 // GC runs 'quadruple the speed' of memory allocation

// Number of sectors to mark for each step.
#define SECTORSTEPSIZE	32
#define POLYSTEPSIZE 120
#define SIDEDEFSTEPSIZE 240

#define GCSTEPSIZE		1024u
#define GCSWEEPMAX		40
#define GCSWEEPCOST		10
#define GCFINALIZECOST	100

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

//extern DThinker *NextToThink;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

namespace GC
{
size_t AllocBytes;
size_t Threshold;
size_t Estimate;
DObject *Root;
DObject *SoftRoots;
int Pause = DEFAULT_GCPAUSE;
int StepMul = DEFAULT_GCMUL;
int StepCount;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// SetThreshold
//
// Sets the new threshold after a collection is finished.
//
//==========================================================================

void SetThreshold()
{
	Threshold = (Estimate / 100) * Pause;
}

//==========================================================================
//
// Reap
//
// Deterministic replacement for the old tri-color incremental collector.
// Walks the Root list and deletes every object that has been Destroy()ed
// (OF_EuthanizeMe).  Object lifetime is driven entirely by explicit Destroy()
// calls (and the thinker lists), not by reachability tracing, so there is no
// mark phase, no gray list, and no write barriers.  ~DObject() still unlinks
// the object from Root and nulls all inbound pointers via
// StaticPointerSubstitution(), so deleting here cannot leave dangling refs.
//
//==========================================================================

static void Reap()
{
	DObject *curr = Root;
	while (curr != NULL)
	{
		// Capture the successor before a possible delete: ~DObject() unlinks
		// curr from Root, so we must not read curr->ObjNext afterwards.
		DObject *next = curr->ObjNext;
		if ((curr->ObjectFlags & OF_EuthanizeMe) &&
		    !(curr->ObjectFlags & (OF_Fixed | OF_Rooted)))
		{
			curr->ObjectFlags |= OF_YesReallyDelete;
			delete curr;
		}
		curr = next;
	}
	Estimate = AllocBytes;
	SetThreshold();
}

//==========================================================================
//
// Step
//
// Formerly one incremental collection step.  Now simply reaps dead objects.
//
//==========================================================================

void Step()
{
	Reap();
	StepCount++;
}

//==========================================================================
//
// FullGC
//
// Collects everything in one fell swoop.
//
//==========================================================================

void FullGC()
{
	// Delete everything that has been Destroy()ed. Objects that are still
	// live (not euthanized) remain on the Root list until they are
	// Destroy()ed in the normal course of play or at level teardown.
	Reap();
}

void DelSoftRootHead()
{
	if (SoftRoots != NULL)
	{
		// Don't let the destructor print a warning message
		SoftRoots->ObjectFlags |= OF_YesReallyDelete;
		delete SoftRoots;
	}
	SoftRoots = NULL;
}

//==========================================================================
//
// AddSoftRoot
//
// Marks an object as a soft root. A soft root behaves exactly like a root
// in MarkRoot, except it can be added at run-time.
//
//==========================================================================

void AddSoftRoot(DObject *obj)
{
	DObject **probe;

	// Are there any soft roots yet?
	if (SoftRoots == NULL)
	{
		// Create a new object to root the soft roots off of, and stick
		// it at the end of the object list, so we know that anything
		// before it is not a soft root.
		SoftRoots = new DObject;
		SoftRoots->ObjectFlags |= OF_Fixed;
		probe = &Root;
		while (*probe != NULL)
		{
			probe = &(*probe)->ObjNext;
		}
		Root = SoftRoots->ObjNext;
		SoftRoots->ObjNext = NULL;
		*probe = SoftRoots;
	}
	// Mark this object as rooted and move it after the SoftRoots marker.
	probe = &Root;
	while (*probe != NULL && *probe != obj)
	{
		probe = &(*probe)->ObjNext;
	}
	*probe = (*probe)->ObjNext;
	obj->ObjNext = SoftRoots->ObjNext;
	SoftRoots->ObjNext = obj;
	obj->ObjectFlags |= OF_Rooted;
}

//==========================================================================
//
// DelSoftRoot
//
// Unroots an object so that it must be reachable or it will get collected.
//
//==========================================================================

void DelSoftRoot(DObject *obj)
{
	DObject **probe;

	if (!(obj->ObjectFlags & OF_Rooted))
	{ // Not rooted, so nothing to do.
		return;
	}
	obj->ObjectFlags &= ~OF_Rooted;
	// Move object out of the soft roots part of the list.
	probe = &SoftRoots;
	while (*probe != NULL && *probe != obj)
	{
		probe = &(*probe)->ObjNext;
	}
	if (*probe == obj)
	{
		*probe = obj->ObjNext;
		obj->ObjNext = Root;
		Root = obj;
	}
}

}

//==========================================================================
//
// STAT gc
//
// Provides information about the current garbage collector state.
//
//==========================================================================

