/*
 *  Copyright (C) 2020 Google
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include "wl_main.h"
#include "m_classes.h"
#include "g_conversation.h"
#include "id_in.h"
#include "sndinfo.h"
#include "r_sprites.h"

typedef enum {
	      START,

	      BEFORE_NON_SHAREWARE,
	      NON_SHAREWARE_WAIT,
	      AFTER_NON_SHAREWARE,

	      BEFORE_PG13,
	      PG13_WAIT,
	      AFTER_PG13,

	      START_DEMO_INTERMISSION,
	      PLAY_INTERMISSION,
	      AFTER_DEMO_INTERMISSION,

	      START_GAME, // 10
	      GAME_LOAD_MAP,
	      PLAY_START,
	      PLAY_STEP_A,
	      PLAY_STEP_B,
	      GAME_END_MAP, // 15
	      GAME_DRAW_PLAY_SCREEN,
	      
	      MAIN_MENU_PREPARE,
	      MENU_PREPARE,
	      MENU_RUN,
	      MENU_EXITING_ENTER_1, //20
	      MENU_EXITING_ENTER_2,
	      MENU_EXITING_ESCAPE_1,
	      MENU_EXITING_ESCAPE_2,

	      MAP_CHANGE_1,
	      MAP_CHANGE_2,
	      MAP_CHANGE_3,
	      MAP_CHANGE_4,
	      MAP_CHANGE_5,
	      END_SEQUENCE_1,

	      LEVEL_INTERMISSION, // 30
	      LEVEL_INTERMISSION_CONTINUE, 
	      LEVEL_INTERMISSION_COUNT_KR,
	      LEVEL_INTERMISSION_COUNT_2,
	      LEVEL_INTERMISSION_COUNT_3,
	      LEVEL_INTERMISSION_WAIT,
	      LEVEL_INTERMISSION_COUNT_BONUS,

	      PSYCH_1,
	      PSYCH_2,
	      PSYCH_3,
	      PSYCH_4, // 40

	      LEVEL_ENTER_TEXT,
	      LEVEL_TRANSITION_INTERMISSION_START,

	      DIED1,
	      DIED2,
	      DEATH_ROTATE,
	      DEATH_WEAPON_DROP,
	      DIED3,
	      DIED4,
	      DIED5,
	      DEATH_FIZZLE, // 50
	      DIED6,
	      VICTORY_ZOOMER_START,
	      VICTORY_ZOOMER_STEP,
	      END_SEQUENCE_2,
	      END_SEQUENCE_3, // 55
	      END_SEQUENCE_4,
	      END_SEQUENCE_5,
	      END_SEQUENCE_6,
	      TEXT_READER_STEP, //59
} wl_stage_t;

class IntermissionInfo;

#define MAX_MENU_STACK 10

struct IntermissionGState
{
	int32_t step;
	uint32_t demoMode;
	FName intermission_name;
	const IntermissionInfo *intermission;
	bool gototitle;
	bool finishing;
	bool finished;
	bool ret_value;
	bool image_ready;
	bool fade_in;
	bool fade_out;
	uint32_t fade_steps;
	uint32_t wait;
};

enum StateMenuType
{
	MAIN_MENU,
	EPISODE_MENU,
	SKILL_MENU,
};

#define MIX_CHANNELS 8

enum SampleFormat {
	FORMAT_8BIT_LINEAR_UNSIGNED,
	FORMAT_8BIT_LINEAR_SIGNED,
	FORMAT_16BIT_LINEAR_SIGNED
};

class Mix_Chunk
{
public:
	virtual void MixInto(int16_t *samples, int output_rate, size_t size, int start_ticks,
			     fixed leftmul, fixed rightmul) = 0;
	virtual int GetLengthTicks() = 0;
};
	
class Mix_Chunk_Sampled : public Mix_Chunk
{
protected:
	int rate;
	int sample_count;
	void *chunk_samples;
	SampleFormat sample_format;
	bool isLooping;
	void MixSamples(int16_t *samples, int output_rate, size_t size, int start_ticks,
			fixed leftmul, fixed rightmul);
};


class Mix_Chunk_Digital : public Mix_Chunk_Sampled
{
public:
	Mix_Chunk_Digital(int rate, void *samples, int sample_count, SampleFormat sample_format,
			  bool isLooping)
	{
		this->rate = rate;
		this->sample_count = sample_count;
		this->chunk_samples = samples;
		this->sample_format = sample_format;
		this->isLooping = isLooping;
	}

	int GetLengthTicks() {
		return sample_count * TICRATE / rate + 1;
	}

	void MixInto(int16_t *samples, int output_rate, size_t size, int start_ticks,
		     fixed leftmul, fixed rightmul) {
		MixSamples(samples, output_rate, size, start_ticks, leftmul, rightmul);
	}
};

namespace DBOPL {
	class Chip;
}

class Mix_Chunk_IMF : public Mix_Chunk_Sampled
{
public:
        Mix_Chunk_IMF(int rate, const byte *imf, size_t imf_size,
		      bool isLooping);
        ~Mix_Chunk_IMF() {
		free (imf);
	}

	int GetLengthTicks() {
		return sample_count * TICRATE / rate + 1;
	}

	void MixInto(int16_t *samples, int output_rate, size_t size, int start_ticks,
		     fixed leftmul, fixed rightmul) {
		EnsureSynthesis(start_ticks + size * TICRATE / output_rate + 1);
		MixSamples(samples, output_rate, size, start_ticks, leftmul, rightmul);
	}

private:
	size_t samples_allocated;
	byte *imf;
	size_t imfptr, imfsize;
	void EnsureSynthesis(int tics);
	void EnsureSpace(int samples);
};


Mix_Chunk *GetSoundDataType(const SoundData &which, SoundData::Type type);

struct SoundChannelState
{
	FString sound;
	uint64_t startTick;
	uint64_t skipTicks;
	int64_t stopTicks;
	Mix_Chunk *sample;
	bool isMusic;
	uint32_t leftPos;
	uint32_t rightPos;
	SoundData::Type type;

	void Serialize(FArchive &arc);

	bool isPlaying(long long currentTick) {
		return sample != NULL
			&& startTick <= currentTick && (stopTicks == -1 || currentTick < stopTicks);
	}
};

typedef struct wl_state_s {
	wl_stage_t stage;
	wl_stage_t stageAfterIntermission;
	bool isFading;
	bool isInWait;
	bool waitCanBeAcked;
	bool waitCanTimeout;
	bool wasAcked;
	longword ackTimeout;
	fixed fadeStep;
	fixed fadeStart;
	fixed fadeEnd;
	fixed fadeCur;
	uint32_t fadeRed;
	uint32_t fadeGreen;
	uint32_t fadeBlue;
	bool died;
	bool dointermission;
	bool playing_title_music;
	bool level_bonus;

	uint32_t episode_num;
	uint32_t skill_num;
	uint32_t menuLevel;
	StateMenuType menuStack[MAX_MENU_STACK];
	SoundChannelState channels[MIX_CHANNELS];
	SoundChannelState musicChannel;

	uint32_t frame_counter;
	uint64_t frame_tic;
	uint32_t tic_rest;
	uint64_t usec;
	FString nextMap;

	bool isCounting;
	bool isCountingRatio;
	FString prevCount;
	int32_t countCurrent;
	int32_t countEnd;
	int32_t countStep;
	uint32_t countX;
	uint32_t countY;	
	int32_t countFrame;
	bool bonusFont;
	uint32_t intermissionSndFreq;
	FString intermissionSound;
	FString transitionSlideshow;
	bool isInQuiz;
	Dialog::QuizMenu *quizMenu;
	const Dialog::Page *quizPage;
	AActor *quizNpc;
	angle_t iangle;
	uint32_t rndval;
	TObjPtr<SpriteZoomer> zoomer;
	bool firstpage;
	bool newpage;
	FString article;
	int32_t textposition;
	int32_t pagenum;
	int32_t numpages;
	uint32_t rowon;
	byte    fontcolor;
	int32_t     picx;
	int32_t     picy;
	int32_t     picdelay;
	bool    layoutdone;

	struct IntermissionGState intermission;
	Menu *currentMenu();
} wl_state_t;

typedef struct wl_input_state_s
{
	// Raw input
	int button_mask;
	int lsx;
	int lsy;
	int rsx;

	// Interpreted input
	Direction menuDir;
	bool menuBack;
	bool menuEnter;
	bool screenAcked;
	bool pauseToggled;
} wl_input_state_t;

void State_FadeOut(wl_state_t *state, int start = 0, int end = 255, int steps = 30);
bool GameMapStart (wl_state_t *state);
bool GameLoopInit (wl_state_t *state);
bool GameMapEnd (wl_state_t *state);

void AdvanceIntermission(wl_state_t *wlstate, IntermissionGState *state);
void InitIntermission(IntermissionGState *state, const FName &intermission, bool demoMode);
bool TopLoopStep(wl_state_t *state, const wl_input_state_t *input);
void InitThinkerList();
void InitGame();
void UninitGame();
unsigned int I_MakeRNGSeed();
extern bool param_nowait;
void CallTerminateFunctions();
void TransformPlayInputs(const wl_input_state_t *input);
void PlayLoopA (void);
bool PlayLoopB (wl_state_t *state);
void PlayInit (void);
void State_Fade(wl_state_t *state, int start, int end, int red, int green, int blue, int steps);
void State_FadeIn(wl_state_t *state, int start = 0, int end = 255, int steps = 30);
void State_Ack(wl_state_t *state);
bool MapChange1(wl_state_t *state);
bool MapChange2(wl_state_t *state);
bool MapChange3(wl_state_t *state);
bool MapChange4(wl_state_t *state);
bool MapChange5(wl_state_t *state);
void PrepareLevelCompleted (void);
bool LevelCompletedState1 (wl_state_t *state);
bool LevelIntermission (wl_state_t *state);
bool LevelIntermissionContinue (wl_state_t *state, bool isAcked);
bool LevelIntermissionCount1(wl_state_t *state, bool isAcked);
bool LevelIntermissionCount2(wl_state_t *state, bool isAcked);
bool LevelIntermissionCount3(wl_state_t *state, bool isAcked);
bool LevelIntermissionCountBonus(wl_state_t *state, bool isAcked);
bool LevelIntermissionWait(wl_state_t *state, bool isAcked);
void State_UserInput(wl_state_t *state, longword delay);
bool updatePsych1(wl_state_t *state);
bool updatePsych2(wl_state_t *state);
bool updatePsych3(wl_state_t *state);
bool updatePsych4(wl_state_t *state);
bool LevelEnterText(wl_state_t *state);
bool LevelEnterIntermissionStart(wl_state_t *state);
bool TransitionText (wl_state_t *stage, int exitClusterNum, int enterClusterNum);
namespace Dialog {
bool quizHandle(wl_state_t *state, const wl_input_state_t *input);
void quizSerialize(wl_state_t *state, FArchive &arc);
}
void State_Delay(wl_state_t *state, longword delay);
void SetSoundPriorities(const char *prio);
extern wl_state_t g_state;
bool Died1(wl_state_t *state);
bool Died2(wl_state_t *state);
bool Died3(wl_state_t *state);
bool Died4(wl_state_t *state);
bool Died5(wl_state_t *state);
bool Died6(wl_state_t *state);
bool DeathRotate(wl_state_t *state);
bool DeathDropWeapon(wl_state_t *state);
bool DeathFizzle(wl_state_t *state);
bool VictoryZoomerStart(wl_state_t *state);
bool VictoryZoomerStep(wl_state_t *state);
bool TextReaderStep(wl_state_t *state, const wl_input_state_t *input);

extern int32_t damagecount, bonuscount;
extern bool palshifted;
void DrawVictory (bool fromIntermission);
extern bool store_files_in_memory;

Mix_Chunk *SynthesizeAdlibIMF(const byte *dataRaw, size_t size);
void    SD_Startup_Adlib(void);
Mix_Chunk *SynthesizeAdlib(const byte *dataRaw);
#endif
