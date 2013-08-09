/*
** sndseq.cpp
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

#include "id_sd.h"
#include "m_random.h"
#include "scanner.h"
#include "sndseq.h"
#include "sndinfo.h"
#include "w_wad.h"
#include "wl_game.h"

enum ESndSeqFlag
{
	SSF_NoStopCutOff = 0x1
};

// We used a fixed size instruction. What the instruction does is determined
// by what flags are set, so a single instruction can play a sound and delay
// for example.
enum ESndSeqInstruction
{
	SSI_PlaySound = 0x1,
	SSI_Delay = 0x2,
	SSI_End = 0x4
};
struct SndSeqInstruction
{
public:
	unsigned int Instruction;
	FName Sound;
	unsigned int Argument;
	unsigned int ArgumentRand;
};

/* The SoundSequence class holds the set of instructions to execute for a given
 * sound sequence OR points to which sound sequence to play for a given event.
 *
 * After parsing all of this information should be static so we can just pass
 * a pointer to the first instruction to a player and only refer back to this
 * object for meta data.
 */
class SoundSequence
{
public:
	SoundSequence() : Flags(0)
	{
	}

	void AddInstruction(const SndSeqInstruction &instr)
	{
		Instructions.Push(instr);
	}

	void Clear()
	{
		for(unsigned int i = 0;i < NUM_SEQ_TYPES;++i)
			AltSequences[i] = NAME_None;
		Instructions.Clear();
	}

	const SoundSequence &GetSequence(SequenceType type) const
	{
		if(AltSequences[type] == NAME_None)
			return *this;
		return SoundSeq(AltSequences[type], type);
	}

	void SetFlag(ESndSeqFlag flag, bool set)
	{
		if(set)
			Flags |= flag;
		else
			Flags &= ~flag;
	}

	void SetSequence(SequenceType type, FName sequence)
	{
		AltSequences[type] = sequence;
	}

	const SndSeqInstruction *Start() const
	{
		if(Instructions.Size() == 0)
			return NULL;
		return &Instructions[0];
	}

private:
	TArray<SndSeqInstruction> Instructions;
	FName AltSequences[NUM_SEQ_TYPES];
	unsigned int Flags;
};

//------------------------------------------------------------------------------

SndSeqTable SoundSeq;

void SndSeqTable::Init()
{
	Printf("S_Init: Reading SNDSEQ defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("SNDSEQ", &lastLump)) != -1)
	{
		ParseSoundSequence(lump);
	}
}

void SndSeqTable::ParseSoundSequence(int lumpnum)
{
	FMemLump lump = Wads.ReadLump(lumpnum);
	Scanner sc((const char*)(lump.GetMem()), lump.GetSize());
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lumpnum));

	while(sc.TokensLeft())
	{
		if(sc.CheckToken('['))
		{
			sc.MustGetToken(TK_Identifier);
			SoundSequence &seq = Sequences[sc->str];
			seq.Clear();

			while(!sc.CheckToken(']'))
			{
				sc.MustGetToken(TK_IntConst);
				SequenceType type = static_cast<SequenceType>(sc->number);

				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
				seq.SetSequence(type, sc->str);
			}
		}
		else
		{
			sc.MustGetToken(':');
			sc.MustGetToken(TK_Identifier);
			SoundSequence &seq = Sequences[sc->str];
			seq.Clear();

			do
			{
				sc.MustGetToken(TK_Identifier);

				if(sc->str.CompareNoCase("end") == 0)
				{
					SndSeqInstruction instr;
					instr.Instruction = SSI_End;
					seq.AddInstruction(instr);
					break;
				}
				else if(sc->str.CompareNoCase("delay") == 0)
				{
					SndSeqInstruction instr;
					instr.Instruction = SSI_Delay;
					instr.ArgumentRand = 0;

					sc.MustGetToken(TK_IntConst);
					instr.Argument = sc->number;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("delayrand") == 0)
				{
					SndSeqInstruction instr;
					instr.Instruction = SSI_Delay;

					sc.MustGetToken(TK_IntConst);
					instr.Argument = sc->number;

					sc.MustGetToken(TK_IntConst);
					instr.ArgumentRand = sc->number - instr.Argument;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("play") == 0)
				{
					SndSeqInstruction instr;
					instr.Instruction = SSI_PlaySound;

					if(!sc.GetNextString())
						sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
					instr.Sound = sc->str;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("nostopcutoff") == 0)
				{
					seq.SetFlag(SSF_NoStopCutOff, true);
				}
				else
				{
					sc.ScriptMessage(Scanner::ERROR, "Unknown sound sequence command '%s'.", sc->str.GetChars());
				}
			}
			while(sc.TokensLeft());
		}
	}
}

const SoundSequence &SndSeqTable::operator() (FName sequence, SequenceType type) const
{
	static const SoundSequence NullSequence;

	const SoundSequence *seq = Sequences.CheckKey(sequence);
	if(seq)
		return seq->GetSequence(type);
	return NullSequence;
}

//------------------------------------------------------------------------------

SndSeqPlayer::SndSeqPlayer(const SoundSequence &sequence, MapSpot Source) :
	Sequence(sequence), Source(Source), Delay(0), Playing(true)
{
	Current = Sequence.Start();
	if(Current == NULL)
		Playing = false;
}

SndSeqPlayer::~SndSeqPlayer()
{
	if(Playing)
		Stop();
}

void SndSeqPlayer::Tick()
{
	if(!Playing || (Delay != 0 && --Delay > 0))
		return;

	do
	{
		if(Current->Instruction & SSI_PlaySound)
		{
			PlaySoundLocMapSpot(Current->Sound, Source);
		}

		if(Current->Instruction & SSI_Delay)
		{
			Delay = Current->Argument + (Current->ArgumentRand ? (M_Random.GenRand32() % Current->ArgumentRand) : 0);
		}

		if(Current->Instruction & SSI_End)
		{
			Playing = false;
		}

		++Current;
	}
	while(Delay == 0 && Playing);
}

void SndSeqPlayer::Stop()
{
	Playing = false;

	// Unfortunately due to limitations of the sound code we can't determine
	// what sound is playing much less stop the sound.
}
