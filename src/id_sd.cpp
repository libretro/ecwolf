//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//
#include "wl_def.h"
#include <SDL_mixer.h>
#include "w_wad.h"
#include "zstring.h"
#include "sndinfo.h"
#ifdef USE_GPL
#include "dosbox/dbopl.h"
#else
#include "mame/fmopl.h"
#endif
#include "wl_main.h"
#include "id_sd.h"

globalsoundpos channelSoundPos[MIX_CHANNELS];

//      Global variables
bool	AdLibPresent,
		SoundBlasterPresent,SBProPresent,
		SoundPositioned;
SDMode	SoundMode;
SMMode	MusicMode;
SDSMode	DigiMode;
int		AdlibVolume=MAX_VOLUME;
int		MusicVolume=MAX_VOLUME;
int		SoundVolume=MAX_VOLUME;

//      Internal variables
static  bool					SD_Started;
static  bool					nextsoundpos;
static  FString                 SoundPlaying;
static  word                    SoundPriority;
static  word                    DigiPriority;
static  int                     LeftPosition;
static  int                     RightPosition;

static  bool					DigiPlaying;

//      PC Sound variables
static  volatile byte           pcLastSample;
static  byte * volatile         pcSound;
static  longword                pcLengthLeft;

//      AdLib variables
static  byte * volatile         alSound;
static  byte                    alBlock;
static  longword                alLengthLeft;
static  longword                alTimeCount;
static  Instrument              alZeroInst;

//      Sequencer variables
static  volatile bool			sqActive;
static  word                   *sqHack;
static	word					*sqHackFreeable=NULL;
static  word                   *sqHackPtr;
static  int                     sqHackLen;
static  int                     sqHackSeqLen;
static  longword                sqHackTime;

#ifdef USE_GPL

DBOPL::Chip oplChip;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	oplChip.Setup(rate);
	return false;
}

static inline void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume)
{
	which.SetVolume(volume);
	which.WriteReg(reg, val);
}

static inline void YM3812UpdateOne(DBOPL::Chip &which, int16_t *stream, int length)
{
	Bit32s buffer[512 * 2];
	int i;

	// length is at maximum samplesPerMusicTick = param_samplerate / 700
	// so 512 is sufficient for a sample rate of 358.4 kHz (default 44.1 kHz)
	if(length > 512)
		length = 512;

	if(which.opl3Active)
	{
		which.GenerateBlock3(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit stereo samples
		// so we only need to convert them to 16-bit samples
		for(i = 0; i < length * 2; i++)  // * 2 for left/right channel
		{
			// Multiply by 4 to match loudness of MAME emulator.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = sample;
		}
	}
	else
	{
		which.GenerateBlock2(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit mono samples
		// so we need to convert them to 32-bit stereo samples
		for(i = 0; i < length; i++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			// Then upconvert to stereo.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i * 2] = stream[i * 2 + 1] = (int16_t) sample;
		}
	}
}

#else

static const int oplChip = 0;

#endif

static void SDL_SoundFinished(void)
{
	SoundPlaying = FString();
	SoundPriority = 0;
}


#ifdef NOTYET

void SDL_turnOnPCSpeaker(word timerval);
#pragma aux SDL_turnOnPCSpeaker = \
		"mov    al,0b6h" \
		"out    43h,al" \
		"mov    al,bl" \
		"out    42h,al" \
		"mov    al,bh" \
		"out    42h,al" \
		"in     al,61h" \
		"or     al,3"   \
		"out    61h,al" \
		parm [bx] \
		modify exact [al]

void SDL_turnOffPCSpeaker();
#pragma aux SDL_turnOffPCSpeaker = \
		"in     al,61h" \
		"and    al,0fch" \
		"out    61h,al" \
		modify exact [al]

void SDL_setPCSpeaker(byte val);
#pragma aux SDL_setPCSpeaker = \
		"in     al,61h" \
		"and    al,0fch" \
		"or     al,ah" \
		"out    61h,al" \
		parm [ah] \
		modify exact [al]

void inline SDL_DoFX()
{
		if(pcSound)
		{
				if(*pcSound!=pcLastSample)
				{
						pcLastSample=*pcSound;

						if(pcLastSample)
								SDL_turnOnPCSpeaker(pcLastSample*60);
						else
								SDL_turnOffPCSpeaker();
				}
				pcSound++;
				pcLengthLeft--;
				if(!pcLengthLeft)
				{
						pcSound=0;
						SoundPlaying = FString();
						SoundPriority=0;
						SDL_turnOffPCSpeaker();
				}
		}

		// [adlib sound stuff removed...]
}

static void SDL_DigitizedDoneInIRQ(void);

void inline SDL_DoFast()
{
		count_fx++;
		if(count_fx>=5)
		{
				count_fx=0;

				SDL_DoFX();

				count_time++;
				if(count_time>=2)
				{
						TimeCount++;
						count_time=0;
				}
		}

		// [adlib music and soundsource stuff removed...]

		TimerCount+=TimerDivisor;
		if(*((word *)&TimerCount+1))
		{
				*((word *)&TimerCount+1)=0;
				t0OldService();
		}
		else
		{
				outp(0x20,0x20);
		}
}

// Timer 0 ISR for 7000Hz interrupts
void __interrupt SDL_t0ExtremeAsmService(void)
{
		if(pcindicate)
		{
				if(pcSound)
				{
						SDL_setPCSpeaker(((*pcSound++)&0x80)>>6);
						pcLengthLeft--;
						if(!pcLengthLeft)
						{
								pcSound=0;
								SDL_turnOffPCSpeaker();
								SDL_DigitizedDoneInIRQ();
						}
				}
		}
		extreme++;
		if(extreme>=10)
		{
				extreme=0;
				SDL_DoFast();
		}
		else
				outp(0x20,0x20);
}

// Timer 0 ISR for 700Hz interrupts
void __interrupt SDL_t0FastAsmService(void)
{
		SDL_DoFast();
}

// Timer 0 ISR for 140Hz interrupts
void __interrupt SDL_t0SlowAsmService(void)
{
		count_time++;
		if(count_time>=2)
		{
				TimeCount++;
				count_time=0;
		}

		SDL_DoFX();

		TimerCount+=TimerDivisor;
		if(*((word *)&TimerCount+1))
		{
				*((word *)&TimerCount+1)=0;
				t0OldService();
		}
		else
				outp(0x20,0x20);
}

void SDL_IndicatePC(bool ind)
{
		pcindicate=ind;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetTimer0() - Sets system timer 0 to the specified speed
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetTimer0(word speed)
{
#ifndef TPROF   // If using Borland's profiling, don't screw with the timer
//      _asm pushfd
		_asm cli

		outp(0x43,0x36);                                // Change timer 0
		outp(0x40,(byte)speed);
		outp(0x40,speed >> 8);
		// Kludge to handle special case for digitized PC sounds
		if (TimerDivisor == (1192030 / (TickBase * 100)))
				TimerDivisor = (1192030 / (TickBase * 10));
		else
				TimerDivisor = speed;

//      _asm popfd
		_asm    sti
#else
		TimerDivisor = 0x10000;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetIntsPerSec() - Uses SDL_SetTimer0() to set the number of
//              interrupts generated by system timer 0 per second
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetIntsPerSec(word ints)
{
		TimerRate = ints;
		SDL_SetTimer0(1192030 / ints);
}

static void
SDL_SetTimerSpeed(void)
{
		word    rate;
		void (_interrupt *isr)(void);

		if ((DigiMode == sds_PC) && DigiPlaying)
		{
				rate = TickBase * 100;
				isr = SDL_t0ExtremeAsmService;
		}
		else if ((MusicMode == smm_AdLib) || ((DigiMode == sds_SoundSource) && DigiPlaying)     )
		{
				rate = TickBase * 10;
				isr = SDL_t0FastAsmService;
		}
		else
		{
				rate = TickBase * 2;
				isr = SDL_t0SlowAsmService;
		}

		if (rate != TimerRate)
		{
				_dos_setvect(8,isr);
				SDL_SetIntsPerSec(rate);
				TimerRate = rate;
		}
}

//
//      PC Sound code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySample() - Plays the specified sample on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySample(byte *data,longword len,bool inIRQ)
{
		if(!inIRQ)
		{
//              _asm    pushfd
				_asm    cli
		}

		SDL_IndicatePC(true);

		pcLengthLeft = len;
		pcSound = (volatile byte *)data;

		if(!inIRQ)
		{
//              _asm    popfd
				_asm    sti
		}
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSample() - Stops a sample playing on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSampleInIRQ(void)
{
		pcSound = 0;

		SDL_IndicatePC(false);

		_asm    in      al,0x61                 // Turn the speaker off
		_asm    and     al,0xfd                 // ~2
		_asm    out     0x61,al
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySound() - Plays the specified sound on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySound(PCSound *sound)
{
//      _asm    pushfd
		_asm    cli

		pcLastSample = -1;
		pcLengthLeft = sound->common.length;
		pcSound = sound->data;

//      _asm    popfd
		_asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSound() - Stops the current sound playing on the PC Speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSound(void)
{
//      _asm    pushfd
		_asm    cli

		pcSound = 0;

		_asm    in      al,0x61                 // Turn the speaker off
		_asm    and     al,0xfd                 // ~2
		_asm    out     0x61,al

//      _asm    popfd
		_asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPC() - Turns off the pc speaker
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutPC(void)
{
//      _asm    pushfd
		_asm    cli

		pcSound = 0;

		_asm    in      al,0x61                 // Turn the speaker & gate off
		_asm    and     al,0xfc                 // ~3
		_asm    out     0x61,al

//      _asm    popfd
		_asm    sti
}

#endif

void
SD_StopDigitized(void)
{
	DigiPlaying = false;
	DigiPriority = 0;
	SoundPositioned = false;
	if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
		SDL_SoundFinished();

	switch (DigiMode)
	{
		default:
			break;
		case sds_PC:
//            SDL_PCStopSampleInIRQ();
			break;
		case sds_SoundBlaster:
//            SDL_SBStopSampleInIRQ();
			Mix_HaltChannel(-1);
			break;
	}
}

void SD_SetPosition(int channel, int leftpos, int rightpos)
{
	if((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
			|| ((leftpos == 15) && (rightpos == 15)))
		Quit("SD_SetPosition: Illegal position");

	switch (DigiMode)
	{
		default:
			break;
		case sds_SoundBlaster:
//            SDL_PositionSBP(leftpos,rightpos);
			Mix_SetPanning(channel, ((15 - leftpos) << 4) + 15,
				((15 - rightpos) << 4) + 15);
			break;
	}
}

byte* SD_PrepareSound(int which)
{
	int size = Wads.LumpLength(which);
	if(size == 0)
		return NULL;

	FMemLump soundLump = Wads.ReadLump(which);

	byte* out = reinterpret_cast<byte*> (Mix_LoadWAV_RW(SDL_RWFromMem(soundLump.GetMem(), size), 1));
	if(!out)
		return NULL;

	// TEMPORARY WORK AROUND FOR MEMORY ERROR
	byte* nout = new byte[sizeof(Mix_Chunk)];
	memcpy(nout, out, sizeof(Mix_Chunk));
	return nout;
}

int SD_PlayDigitized(const SoundData &which,int leftpos,int rightpos,SoundChannel chan)
{
	if (!DigiMode)
		return 0;

	int channel = chan;
	if(chan == SD_GENERIC)
	{
		channel = Mix_GroupAvailable(1);
		if(channel == -1) channel = Mix_GroupOldest(1);
		if(channel == -1)           // All sounds stopped in the meantime?
			channel = Mix_GroupAvailable(1);
	}
	SD_SetPosition(channel, leftpos,rightpos);

	DigiPlaying = true;

	Mix_Chunk *sample = reinterpret_cast<Mix_Chunk*> (which.GetData(SoundData::DIGITAL));
	if(sample == NULL)
		return 0;

	Mix_Volume(channel, static_cast<int> (ceil(128.0*MULTIPLY_VOLUME(SoundVolume))));
	if(Mix_PlayChannel(channel, sample, 0) == -1)
	{
		printf("Unable to play sound: %s\n", Mix_GetError());
		return 0;
	}

	return channel;
}

void SD_ChannelFinished(int channel)
{
	channelSoundPos[channel].valid = 0;
}

void
SD_SetDigiDevice(SDSMode mode)
{
	bool devicenotpresent;

	if (mode == DigiMode)
		return;

	SD_StopDigitized();

	devicenotpresent = false;
	switch (mode)
	{
		default:
			break;
		case sds_SoundBlaster:
			if (!SoundBlasterPresent)
				devicenotpresent = true;
			break;
	}

	if (!devicenotpresent)
	{
		DigiMode = mode;

#ifdef NOTYET
		SDL_SetTimerSpeed();
#endif
	}
}

//      AdLib Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALStopSound(void)
{
	alSound = 0;
	alOut(alFreqH + 0, 0);
}

static void
SDL_AlSetFXInst(Instrument *inst)
{
	byte c,m;

	m = 0;      // modulator cell for channel 0
	c = 3;      // carrier cell for channel 0
	alOut(m + alChar,inst->mChar);
	alOut(m + alScale,inst->mScale);
	alOut(m + alAttack,inst->mAttack);
	alOut(m + alSus,inst->mSus);
	alOut(m + alWave,inst->mWave);
	alOut(c + alChar,inst->cChar);
	alOut(c + alScale,inst->cScale);
	alOut(c + alAttack,inst->cAttack);
	alOut(c + alSus,inst->cSus);
	alOut(c + alWave,inst->cWave);

	// Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);
	alOut(alFeedCon,0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALPlaySound(AdLibSound *sound)
{
	Instrument      *inst;
	byte            *data;

	SDL_ALStopSound();

	alLengthLeft = sound->common.length;
	data = sound->data;
	alBlock = ((sound->block & 7) << 2) | 0x20;
	inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		Quit("SDL_ALPlaySound() - Bad instrument");
	}

	SDL_AlSetFXInst(inst);
	alSound = (byte *)data;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutAL() - Shuts down the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutAL(void)
{
	alSound = 0;
	alOut(alEffects,0);
	alOut(alFreqH + 0,0);
	SDL_AlSetFXInst(&alZeroInst);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanAL() - Totally shuts down the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanAL(void)
{
	int     i;

	alOut(alEffects,0);
	for (i = 1; i < 0xf5; i++)
		alOut(i, 0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
	alOut(alEffects, 0);
	SDL_AlSetFXInst(&alZeroInst);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_DetectAdLib() - Determines if there's an AdLib (or SoundBlaster
//              emulating an AdLib) present
//
///////////////////////////////////////////////////////////////////////////
static bool SDL_DetectAdLib(void)
{
	for (int i = 1; i <= 0xf5; i++)       // Zero all the registers
		alOut(i, 0);

	alOut(1, 0x20);             // Set WSE=1
//    alOut(8, 0);                // Set CSM=0 & SEL=0

	return true;
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
//            SDL_ShutPC();
			break;
		case sdm_AdLib:
			SDL_ShutAL();
			break;
	}
	SoundMode = sdm_Off;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanDevice() - totally shuts down all sound devices
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanDevice(void)
{
	if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
		SDL_CleanAL();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
	switch (SoundMode)
	{
		default:
			break;
		case sdm_AdLib:
			SDL_StartAL();
			break;
	}
	SoundPlaying = FString();
	SoundPriority = 0;
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
bool SD_SetSoundMode(SDMode mode)
{
	bool result = false;

	SD_StopSound();

	if ((mode == sdm_AdLib) && !AdLibPresent)
		mode = sdm_PC;

	switch (mode)
	{
		case sdm_Off:
		case sdm_PC:
			result = true;
			break;
		case sdm_AdLib:
			if (AdLibPresent)
				result = true;
			break;
		default:
			Quit("SD_SetSoundMode: Invalid sound mode %i", mode);
			return false;
	}

	if (result && (mode != SoundMode))
	{
		SDL_ShutDevice();
		SoundMode = mode;
		SDL_StartDevice();
	}

	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
bool SD_SetMusicMode(SMMode mode)
{
	bool result = false;

	SD_FadeOutMusic();
	while (SD_MusicPlaying())
		SDL_Delay(5);

	switch (mode)
	{
		case smm_Off:
			result = true;
			break;
		case smm_AdLib:
			if (AdLibPresent)
				result = true;
			break;
	}

	if (result)
		MusicMode = mode;

//    SDL_SetTimerSpeed();

	return(result);
}

int numreadysamples = 0;
byte *curAlSound = 0;
byte *curAlSoundPtr = 0;
longword curAlLengthLeft = 0;
int soundTimeCounter = 5;
int samplesPerMusicTick;

void SDL_IMFMusicPlayer(void *udata, Uint8 *stream, int len)
{
	int stereolen = len>>1;
	int sampleslen = stereolen>>1;
	int16_t *stream16 = (int16_t *) (void *) stream;    // expect correct alignment

	while(1)
	{
		if(numreadysamples)
		{
			if(numreadysamples<sampleslen)
			{
				YM3812UpdateOne(oplChip, stream16, numreadysamples);
				stream16 += numreadysamples*2;
				sampleslen -= numreadysamples;
			}
			else
			{
				YM3812UpdateOne(oplChip, stream16, sampleslen);
				numreadysamples -= sampleslen;
				return;
			}
		}
		soundTimeCounter--;
		if(!soundTimeCounter)
		{
			soundTimeCounter = 5;
			if(curAlSound != alSound)
			{
				curAlSound = curAlSoundPtr = alSound;
				curAlLengthLeft = alLengthLeft;
			}
			if(curAlSound)
			{
				if(*curAlSoundPtr)
				{
					alOut(alFreqL, *curAlSoundPtr);
					alOut(alFreqH, alBlock);
				}
				else alOut(alFreqH, 0);
				curAlSoundPtr++;
				curAlLengthLeft--;
				if(!curAlLengthLeft)
				{
					curAlSound = alSound = 0;
					SoundPlaying = FString();
					SoundPriority = 0;
					alOut(alFreqH, 0);
				}
			}
		}
		if(sqActive)
		{
			do
			{
				if(sqHackTime > alTimeCount) break;
				sqHackTime = alTimeCount + *(sqHackPtr+1);
				alOutMusic(*(byte *) sqHackPtr, *(((byte *) sqHackPtr)+1));
				sqHackPtr += 2;
				sqHackLen -= 4;
			}
			while(sqHackLen>0);
			alTimeCount++;
			if(!sqHackLen)
			{
				sqHackPtr = sqHack;
				sqHackLen = sqHackSeqLen;
				sqHackTime = 0;
				alTimeCount = 0;
			}
		}
		numreadysamples = samplesPerMusicTick;
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
	int     i;

	if (SD_Started)
		return;

	if(Mix_OpenAudio(param_samplerate, AUDIO_S16, 2, param_audiobuffer))
	{
		printf("Unable to open audio: %s\n", Mix_GetError());
		return;
	}

	Mix_ReserveChannels(2);  // reserve player and boss weapon channels
	Mix_GroupChannels(2, MIX_CHANNELS-1, 1); // group remaining channels

	// Init music

	samplesPerMusicTick = param_samplerate / 700;    // SDL_t0FastAsmService played at 700Hz

	if(YM3812Init(1,3579545,param_samplerate))
	{
		printf("Unable to create virtual OPL!!\n");
	}

	for(i=1;i<0xf6;i++)
		YM3812Write(oplChip,i,0,MAX_VOLUME);

	YM3812Write(oplChip,1,0x20,MAX_VOLUME); // Set WSE=1
//    YM3812Write(0,8,0); // Set CSM=0 & SEL=0		 // already set in for statement

	Mix_HookMusic(SDL_IMFMusicPlayer, 0);
	Mix_ChannelFinished(SD_ChannelFinished);
	AdLibPresent = true;
	SoundBlasterPresent = true;

	alTimeCount = 0;

	SD_SetSoundMode(sdm_Off);
	SD_SetMusicMode(smm_Off);

	SD_Started = true;

	SoundInfo.Init();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
	if (!SD_Started)
		return;

	SD_MusicOff();
	SD_StopSound();

	SD_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
	LeftPosition = leftvol;
	RightPosition = rightvol;
	nextsoundpos = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//
///////////////////////////////////////////////////////////////////////////
bool SD_PlaySound(const char* sound, SoundChannel chan)
{
	bool            ispos;
	int             lp,rp;

	lp = LeftPosition;
	rp = RightPosition;
	LeftPosition = 0;
	RightPosition = 0;

	ispos = nextsoundpos;
	nextsoundpos = false;

	const SoundData &sindex = SoundInfo[sound];

	if ((SoundMode != sdm_Off) && sindex.IsNull())
		return 0;

	if ((DigiMode != sds_Off) && sindex.HasType(SoundData::DIGITAL))
	{
		if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
		{
#ifdef NOTYET
			if (s->priority < SoundPriority)
				return 0;

			SDL_PCStopSound();

			SD_PlayDigitized(sindex,lp,rp);
			SoundPositioned = ispos;
			SoundPriority = s->priority;
#else
			return 0;
#endif
		}
		else
		{
#ifdef NOTYET
			if (s->priority < DigiPriority)
				return(false);
#endif

			int channel = SD_PlayDigitized(sindex, lp, rp, chan);
			SoundPositioned = ispos;
			DigiPriority = sindex.GetPriority();
			SoundPlaying = sound;
			return channel + 1;
		}

		return(true);
	}

	if (SoundMode == sdm_Off)
		return 0;

//    if (!s->length)
//        Quit("SD_PlaySound() - Zero length sound");
	if (sindex.GetPriority() < SoundPriority)
		return 0;

	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
//            SDL_PCPlaySound((PCSound *)s);
			break;
		case sdm_AdLib:
			if(sindex.HasType(SoundData::ADLIB))
				SDL_ALPlaySound((AdLibSound *)sindex.GetData(SoundData::ADLIB));
			break;
	}

	SoundPriority = sindex.GetPriority();
	SoundPlaying = sound;

	return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
bool SD_SoundPlaying(void)
{
	bool result = false;

	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
			result = pcSound? true : false;
			break;
		case sdm_AdLib:
			result = alSound? true : false;
			break;
	}

	if (result)
		return SoundPlaying.IsNotEmpty();
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
	if (DigiPlaying)
		SD_StopDigitized();

	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
//            SDL_PCStopSound();
			break;
		case sdm_AdLib:
			SDL_ALStopSound();
			break;
	}

	SoundPositioned = false;

	SDL_SoundFinished();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
	while (SD_SoundPlaying())
		SDL_Delay(5);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
	sqActive = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
	word    i;

	sqActive = false;
	switch (MusicMode)
	{
		default:
			break;
		case smm_AdLib:
			alOut(alEffects, 0);
			for (i = 0;i < sqMaxTracks;i++)
				alOut(alFreqH + i + 1, 0);
			break;
	}

	return (int) (sqHackPtr-sqHack);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(const char* chunk)
{
	SD_MusicOff();

	if (MusicMode == smm_AdLib)
	{
		int lumpNum = Wads.CheckNumForName(chunk, ns_music);
		if(lumpNum == -1)
			return;
		FWadLump lump = Wads.OpenLumpNum(lumpNum);
		if(sqHackFreeable != NULL)
			delete[] sqHackFreeable;
		sqHack = new word[(Wads.LumpLength(lumpNum)/2)+1]; //+1 is just safety
		sqHackFreeable = sqHack;
		lump.Read(sqHack, Wads.LumpLength(lumpNum));
		if(*sqHack == 0) sqHackLen = sqHackSeqLen = Wads.LumpLength(lumpNum);
		else sqHackLen = sqHackSeqLen = *sqHack++;
		sqHackPtr = sqHack;
		sqHackTime = 0;
		alTimeCount = 0;
		SD_MusicOn();
	}
}

void
SD_ContinueMusic(const char* chunk, int startoffs)
{
	SD_MusicOff();

	if (MusicMode == smm_AdLib)
	{
		{ // We need this scope to "delete" the lump before modifying the sqHack pointers.
			int lumpNum = Wads.CheckNumForName(chunk, ns_music);
			if(lumpNum == -1)
				return;
			FWadLump lump = Wads.OpenLumpNum(lumpNum);
			if(sqHackFreeable != NULL)
				delete[] sqHackFreeable;
			sqHack = new word[(Wads.LumpLength(lumpNum)/2)+1]; //+1 is just safety
			sqHackFreeable = sqHack;
			lump.Read(sqHack, Wads.LumpLength(lumpNum));
			if(*sqHack == 0) sqHackLen = sqHackSeqLen = Wads.LumpLength(lumpNum);
			else sqHackLen = sqHackSeqLen = *sqHack++;
			sqHackPtr = sqHack;
		}

		if(startoffs >= sqHackLen)
		{
			Quit("SD_StartMusic: Illegal startoffs provided!");
		}

		// fast forward to correct position
		// (needed to reconstruct the instruments)

		for(int i = 0; i < startoffs; i += 2)
		{
			byte reg = *(byte *)sqHackPtr;
			byte val = *(((byte *)sqHackPtr) + 1);
			if(reg >= 0xb1 && reg <= 0xb8) val &= 0xdf;           // disable play note flag
			else if(reg == 0xbd) val &= 0xe0;                     // disable drum flags

			alOut(reg,val);
			sqHackPtr += 2;
			sqHackLen -= 4;
		}
		sqHackTime = 0;
		alTimeCount = 0;

		SD_MusicOn();
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
	switch (MusicMode)
	{
		default:
			break;
		case smm_AdLib:
			// DEBUG - quick hack to turn the music off
			SD_MusicOff();
			break;
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
bool SD_MusicPlaying(void)
{
	bool result;

	switch (MusicMode)
	{
		case smm_AdLib:
			result = sqActive;
			break;
		default:
			result = false;
			break;
	}

	return(result);
}
