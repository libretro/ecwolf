/*
** sndseq.h
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

#ifndef __SNDSEQ_H__
#define __SNDSEQ_H__

#include "wl_def.h"
#include "gamemap.h"
#include "tarray.h"
#include "name.h"

class SoundSequence;
struct SndSeqInstruction;

enum SequenceType
{
	SEQ_OpenNormal,
	SEQ_CloseNormal,
	SEQ_OpenBlazing,
	SEQ_CloseBlazing,

	NUM_SEQ_TYPES
};

class SndSeqTable
{
public:
	void Init();

	const SoundSequence &operator() (FName sequence, SequenceType type) const;
protected:
	void ParseSoundSequence(int lumpnum);

private:
	TMap<FName, SoundSequence> Sequences;
};
extern SndSeqTable SoundSeq;

class SndSeqPlayer
{
public:
	SndSeqPlayer(const SoundSequence &sequence, MapSpot source);
	~SndSeqPlayer();

	bool IsPlaying() const { return Playing; }
	void Tick();
	void Stop();

private:
	const SoundSequence &Sequence;
	const SndSeqInstruction *Current;
	MapSpot Source;

	unsigned int Delay;
	bool Playing;
};

#endif
