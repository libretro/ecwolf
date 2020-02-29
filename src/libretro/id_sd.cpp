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
#include "w_wad.h"
#include "zstring.h"
#include "sndinfo.h"
#include "sndseq.h"
#include "wl_main.h"
#include "id_sd.h"
#include "state_machine.h"
#include "wl_play.h"
#ifdef USE_GPL
#include "dosbox/dbopl.h"
#else
#include "mame/fmopl.h"
#endif

static  bool					SD_Started;
static  bool					nextsoundpos;
static  int                     LeftPosition;
static  int                     RightPosition;
static const int synthesisRate = 44100;
#define MUSIC_RATE 700	// Must be a multiple of SOUND_RATE
#define SOUND_RATE 140	// Also affects PC Speaker sounds
static const int samplesPerSoundTick = synthesisRate / SOUND_RATE;
static const int samplesPerMusicTick = synthesisRate / MUSIC_RATE;
#undef alOut
#define alOut(n,b) 		YM3812Write(oplChip, n, b, 20)

Mix_Chunk *SynthesizeAdlibIMF(const byte *dataRaw, size_t size);
  
FString                 SoundPlaying;
globalsoundpos channelSoundPos[MIX_CHANNELS];
struct SynthCacheItem
{
	        TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> adlibSynth;
	        TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> pcSynth;
};

static TArray<SynthCacheItem> synthCache;

#define PC_BASE_TIMER 1193181

#ifdef USE_GPL

DBOPL::Chip oplChip;
DBOPL::Chip musicOpl;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	static int inited = false;
	if (inited)
		return false;
	inited = true;
	oplChip.Setup(rate);
	musicOpl.Setup(rate);
	return false;
}

static inline void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume)
{
	which.WriteReg(reg, val);
}

static inline void YM3812UpdateOneMono(DBOPL::Chip &which, int16_t *stream, int length)
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
		for(i = 0; i < length; i ++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			Bit32s sample = buffer[i * 2] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = LittleShort(sample);
		}
	}
	else
	{
		which.GenerateBlock2(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit mono samples
		// so we need to convert them to 16-bit mono samples
		for(i = 0; i < length; i++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			// Then upconvert to stereo.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = (int16_t) LittleShort(sample);
		}
	}
}

#else

static const int oplChip = 0;

#endif

void    SD_Startup(void)
{
	if (SD_Started)
		return;

	SoundInfo.Init();
	SoundSeq.Init();
	SoundPlaying = FString();
	YM3812Init(1,3579545,synthesisRate);

	SD_Started = true;
}

void SD_Shutdown(void)
{
}

void
SD_PositionSound(int leftvol,int rightvol)
{
	LeftPosition = leftvol;
	RightPosition = rightvol;
	nextsoundpos = true;
}

void    SD_SetPosition(int channel, int leftvol,int rightvol) {
	g_state.channels[channel].leftPos = leftvol;
	g_state.channels[channel].rightPos = rightvol;
}


struct SoundPriorities
{
	int numPriorities;
	SoundData::Type priorities[4];
};

static SoundPriorities DigAdlibPC() {
	SoundPriorities ret;
	ret.numPriorities = 3;
	ret.priorities[0] = SoundData::DIGITAL;
	ret.priorities[1] = SoundData::ADLIB;
	ret.priorities[2] = SoundData::PCSPEAKER;
	return ret;
}

static SoundPriorities PrioFromString(const char *str)
{
	SoundPriorities ret = SoundPriorities();
	const char *ptr = str;
	while (ret.numPriorities < 3) {
		while (*ptr == '-')
			ptr++;
		if (*ptr == 0)
			break;
		if (strncmp(ptr, "digi", 4) == 0) {
			ptr += 4;
			ret.priorities[ret.numPriorities++] = SoundData::DIGITAL;
			continue;
		}
		if (strncmp(ptr, "adlib", 5) == 0) {
			ptr += 5;
			ret.priorities[ret.numPriorities++] = SoundData::ADLIB;
			continue;
		}
		if (strncmp(ptr, "speaker", 7) == 0) {
			ptr += 7;
			ret.priorities[ret.numPriorities++] = SoundData::PCSPEAKER;
			continue;
		}
	}
	if (ret.numPriorities == 0)
		return DigAdlibPC();
	return ret;
}

struct SoundPriorities currentPriorities;

void SetSoundPriorities(const char *prio)
{
	currentPriorities = PrioFromString(prio);
}

int SD_PlayDigitized(const char *sound, const SoundPriorities &priorities, const SoundData &which,int leftpos,int rightpos,SoundChannel chan)
{
	// If this sound has been played too recently, don't play it again.
	// (Fix for extremely loud sounds when one plays over itself too much.)
	uint32_t currentTick = GetTimeCount();
	if (currentTick - SoundInfo.GetLastPlayTick(which) < 1)
		return 0;

	Mix_Chunk *sample = NULL;
	SoundData::Type type;
	for (int i = 0; i < priorities.numPriorities; i++) {
		type = priorities.priorities[i];
		sample = GetSoundDataType(which, type);
		if (sample) {
			break;
		}
	}
	if(sample == NULL)
		return 0;

	SoundInfo.SetLastPlayTick(which, currentTick);

	int channel = chan;
	if(chan == SD_GENERIC)
	{
		int oldest = -1, available = -1;
		int i;
		for (i = 2; i < MIX_CHANNELS; i++) {
			if (!g_state.channels[i].isPlaying(currentTick)) {
				available = i;
				break;
			}
			if (oldest == -1 || g_state.channels[i].startTick < g_state.channels[oldest].startTick)
				oldest = i;
		}
		if (available != -1)
			channel = available;
		else
			channel = oldest;
	}
	SD_SetPosition(channel, leftpos,rightpos);

	g_state.channels[channel].sample = sample;
	g_state.channels[channel].sound = sound;
	g_state.channels[channel].type = type;
	g_state.channels[channel].startTick = currentTick;
	g_state.channels[channel].skipTicks = 0;
	g_state.channels[channel].isMusic = false;
	g_state.channels[channel].stopTicks = currentTick + sample->GetLengthTicks() + 1;

	// Return channel + 1 because zero is a valid channel.
	return channel + 1;
}

int SD_PlaySound(const char* sound,SoundChannel chan)
{
	int             lp,rp;

	lp = LeftPosition;
	rp = RightPosition;
	LeftPosition = 0;
	RightPosition = 0;
 
	nextsoundpos = false;

	const SoundData &sindex = SoundInfo[sound];

	if (sindex.IsNull()) {
		return 0;
	}

	int channel = SD_PlayDigitized(sound, currentPriorities, sindex, lp, rp, chan);
	//		SoundPositioned = ispos;
	SoundPlaying = sound;
	return channel;
}

void    SD_StopSound(void)
{
	for (int channel = 0; channel < MIX_CHANNELS; channel++) {
		g_state.channels[channel].sample = NULL;
		g_state.channels[channel].sound = "";
	}
}

void    SD_WaitSoundDone(void)
{
	//TODO
}

struct MusicCacheItem
{
	const char *name;
	Mix_Chunk *chunk;
};

#define MUSIC_CACHE_SIZE 20

static MusicCacheItem music_cache[MUSIC_CACHE_SIZE];
static int cacheptr;

Mix_Chunk *GetMusic(const char* chunk)
{
	for (int i = 0; i < MUSIC_CACHE_SIZE; i++) {
		if (music_cache[i].name != NULL && strcmp(music_cache[i].name, chunk) == 0)
			return music_cache[i].chunk;
	}
	int lumpNum = Wads.CheckNumForName(chunk, ns_music);
	if(lumpNum == -1) {
		return NULL;
	}

	int size = Wads.LumpLength(lumpNum);
	if(size == 0)
		return NULL;

	FMemLump soundLump = Wads.ReadLump(lumpNum);

	Mix_Chunk *res = SynthesizeAdlibIMF((const byte *) soundLump.GetMem(), size);
	music_cache[cacheptr].name = strdup(chunk);
	music_cache[cacheptr].chunk = res;
	cacheptr++;
	return res;
}

void    SD_ContinueMusic(const char* chunk, int startoffs)
{
	uint32_t currentTick = GetTimeCount();

	SD_MusicOff();

	Mix_Chunk *sample = GetMusic(chunk);

	g_state.musicChannel.sample = sample;
	g_state.musicChannel.sound = chunk;
	g_state.musicChannel.type = SoundData::DIGITAL;
	g_state.musicChannel.startTick = currentTick;
	g_state.musicChannel.isMusic = true;
	g_state.musicChannel.skipTicks = startoffs;
	g_state.musicChannel.stopTicks = -1;
}


void    SD_StartMusic(const char* chunk)
{
	SD_ContinueMusic(chunk, 0);
}

int     SD_MusicOff(void)
{
	uint32_t currentTick = GetTimeCount();
	g_state.musicChannel.stopTicks = currentTick;
	return 	currentTick - g_state.musicChannel.startTick + g_state.musicChannel.skipTicks;
}

bool    SD_SoundPlaying(void)
{
	uint32_t currentTick = GetTimeCount();
	for (int i = 0; i < MIX_CHANNELS; i++) {
		if (g_state.channels[i].isPlaying(currentTick))
			return true;
	}

	return false;
}

struct Mix_Chunk *SD_PrepareSound(int which)
{
	int size = Wads.LumpLength(which);
	if(size == 0)
		return NULL;

	FMemLump soundLump = Wads.ReadLump(which);

	// 0x2A is the size of the sound header. From what I can tell the csnds
	// have mostly garbage filled headers (outside of what is precisely needed
	// since the sample rate is hard coded). I'm not sure if the sounds are
	// 8-bit or 16-bit, but it looks like the sample rate is coded to ~22050.
	if(size > 0x2A && BigShort(*(WORD*)soundLump.GetMem()) == 1)
	{
		int sample_count = size - 0x2a;
		void *samples = malloc (size - 0x2a);
		CHECKMALLOCRESULT(samples);
		memcpy(samples, ((char*)soundLump.GetMem())+0x2A, size-0x2A);
		return new Mix_Chunk_Digital(
			22050,
			samples,
			sample_count,
			FORMAT_8BIT_LINEAR_UNSIGNED,
			false
		);
	}

	// TODO: support skipping extra headers
	if (size > 44 && memcmp(soundLump.GetMem(), "RIFF", 4) == 0
	    && memcmp((char*)soundLump.GetMem() + 8, "WAVEfmt ", 8) == 0) {
		int rate = LittleLong(((DWORD*)soundLump.GetMem())[6]);
		int bits = LittleShort(((WORD*)soundLump.GetMem())[17]);
		int channels = LittleShort(((WORD*)soundLump.GetMem())[11]);
		int format = LittleShort(((WORD*)soundLump.GetMem())[10]);
		int sample_count = (size - 44) / (bits / 8);
		SampleFormat sample_format;
		if (format == 1 && channels == 1 && bits == 8) {
			sample_format = FORMAT_8BIT_LINEAR_SIGNED;
			sample_count = size - 44;
		} else if (format == 1 && channels == 1 && bits == 16) {
			sample_format = FORMAT_16BIT_LINEAR_SIGNED;
		} else {
			printf ("Unknown WAV variant %d/%d/%d\n",
				bits, channels, format);
			return NULL;
		}
		void *samples = malloc (size - 44);
		CHECKMALLOCRESULT(samples);
		memcpy(samples, ((char*)soundLump.GetMem())+44, size-44);
		return new Mix_Chunk_Digital(
			rate,
			samples,
			sample_count,
			sample_format,
			false
		);
	}

	printf ("unknown format. Header: %x\n", BigLong(*(DWORD*)soundLump.GetMem()));
	return NULL;
}

Mix_Chunk *SynthesizeSpeaker(const byte *dataRaw)
{
	PCSound *sound = (PCSound*) dataRaw;
	int pcPhaseTick = 0;
	int pcLastSample = 0;
	longword	pcPhaseLength = 0;
	int pcLength = LittleLong(sound->common.length);
	byte *pcSound = sound->data;

	int16_t *samples = (int16_t*) malloc (pcLength * samplesPerSoundTick * 2);
	CHECKMALLOCRESULT(samples);
	int16_t *sampleptr = samples;
	short	pcVolume = 5000;

	for (int i = 0; i < pcLength; i++, pcSound++) {
		if(*pcSound!=pcLastSample)
		{
			pcLastSample=*pcSound;
			if(pcLastSample) {
					pcPhaseLength = (pcLastSample*60*synthesisRate)/(2*PC_BASE_TIMER);
					pcPhaseTick = 0;
			}
			else
			{
					pcPhaseTick = 0;
			}
		}

		for (int j = 0; j < samplesPerSoundTick; j++) {
			*sampleptr++ = pcVolume;
			// Update the PC speaker state:
			if(pcPhaseTick++ >= pcPhaseLength)
			{
				pcVolume = -pcVolume;
				pcPhaseTick = 0;
			}
		}
	}
	return new Mix_Chunk_Digital(
		synthesisRate,
		samples,
		sampleptr - samples,
		FORMAT_16BIT_LINEAR_SIGNED,
		false
		);
}

static void SDL_AlSetChanInst(DBOPL::Chip &oplChip, const Instrument *inst, unsigned int chan)
{
	static const byte chanOps[OPL_CHANNELS] = {
		0, 1, 2, 8, 9, 0xA, 0x10, 0x11, 0x12
	};
	byte c,m;

	m = chanOps[chan]; // modulator cell for channel
	c = m + 3; // carrier cell for channel
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

	alOut(chan + alFreqL,0);
	alOut(chan + alFreqH,0);
	alOut(chan + alFeedCon,0);
}
static void SDL_AlSetFXInst(DBOPL::Chip &oplChip, const Instrument *inst)
{
	SDL_AlSetChanInst(oplChip, inst, 0);
}

Mix_Chunk *SynthesizeAdlib(const byte *dataRaw)
{
	AdLibSound *sound = (AdLibSound*) dataRaw;

	alOut(alFreqH + 0, 0);

	int alLength = LittleLong(sound->common.length);
	byte alBlock = ((sound->block & 7) << 2) | 0x20;
	Instrument      *inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		Quit("SDL_ALPlaySound() - Bad instrument");
	}

	SDL_AlSetFXInst(oplChip, inst);
	byte *alSound = (byte *)sound->data;

	int16_t *samples = (int16_t*) malloc (alLength * samplesPerSoundTick * 2);
	CHECKMALLOCRESULT(samples);
	int16_t *sampleptr = samples;

	for (int i = 0; i < alLength; i++, alSound++) {
		if(*alSound)
		{
			alOut(alFreqL, *alSound);
			alOut(alFreqH, alBlock);
		} else alOut(alFreqH, 0);

		YM3812UpdateOneMono(oplChip, sampleptr, samplesPerSoundTick);
		sampleptr += samplesPerSoundTick;
	}
	return new Mix_Chunk_Digital(
		synthesisRate,
		samples,
		sampleptr - samples,
		FORMAT_16BIT_LINEAR_SIGNED,
		false
		);
}


Mix_Chunk *GetSoundDataType(const SoundData &which, SoundData::Type type)
{
	if (!which.HasType(type))
		return NULL;
	int idx = which.GetIndex();
	if (idx >= synthCache.Size())
		synthCache.Resize(idx + 1);
	SynthCacheItem &synth = synthCache[idx];
	switch(type) {
	case SoundData::DIGITAL:
		return which.GetDigitalData();
	case SoundData::ADLIB:
		if (!synth.adlibSynth)
			synth.adlibSynth.Reset(SynthesizeAdlib(which.GetAdLibData()));
		return synth.adlibSynth;
	case SoundData::PCSPEAKER:
		if (!synth.pcSynth)
			synth.pcSynth.Reset(SynthesizeSpeaker(which.GetSpeakerData()));
		return synth.pcSynth;
	}
	return NULL;
}

void SoundChannelState::Serialize(FArchive &arc)
{	
	arc << sound;
	arc << startTick;
	arc << skipTicks;
	arc << stopTicks;
	arc << leftPos;
	arc << rightPos;
	arc << (DWORD &) type;
	arc << isMusic;

	if (!arc.IsStoring())
		sample = isMusic ? GetMusic(sound) : GetSoundDataType(SoundInfo[sound], type);
}

template <typename T, int bias>
static void
mix_no_upscale(int16_t *result, const T *input, int num_samples, fixed leftmul, fixed rightmul)
{
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int16_t l = (input[i] - bias) * leftmul >> (FRACBITS - (16 - sizeof(T) * 8));
		int16_t r = (input[i] - bias) * rightmul >> (FRACBITS - (16 - sizeof(T) * 8));
		*outptr++ += l;
		*outptr++ += r;    
	}
}

template <typename T, int bias>
static void
mix_int_upscale(int16_t *result, const T *input, int num_samples, int upsample, fixed leftmul, fixed rightmul)
{
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int16_t l = (input[i] - bias) * leftmul >> (FRACBITS - (16 - sizeof(T) * 8));
		int16_t r = (input[i] - bias) * rightmul >> (FRACBITS - (16 - sizeof(T) * 8));
		for (int j = 0 ; j < upsample; j++) {
			*outptr++ += l;
			*outptr++ += r;    
		}
	}
}

template <typename T, int bias>
static void
mix_fractional_upscale(int16_t *result, int output_rate, const T *input, int input_rate, int num_samples, fixed leftmul, fixed rightmul)
{
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int16_t l = (input[i] - bias) * leftmul >> (FRACBITS - (16 - sizeof(T) * 8));
		int16_t r = (input[i] - bias) * rightmul >> (FRACBITS - (16 - sizeof(T) * 8));
		while ((outptr - result) / 2 * (long long)input_rate < i * (long long) output_rate) {
			*outptr++ += l;
			*outptr++ += r;    
		}
	}
}

template <typename T, int bias>
static void
mix(int16_t *result, int output_rate, const T *input, int input_rate, int num_samples, fixed leftmul, fixed rightmul)
{
	if (output_rate == input_rate) {
		mix_no_upscale<T, bias>(result, input, num_samples, leftmul, rightmul);
		return;
	}
	if (output_rate % input_rate == 0) {
		mix_int_upscale<T, bias>(result, input, num_samples, output_rate / input_rate, leftmul, rightmul);
		return;
	}

	if (output_rate < input_rate) {
		printf("Downsampling is not supported (from %d to %d)\n", input_rate, output_rate);
		return;
	}

	mix_fractional_upscale<T, bias>(result, output_rate, input, input_rate, num_samples, leftmul, rightmul);
	return;
}

void Mix_Chunk_Sampled::MixSamples (int16_t *result, int output_rate, size_t size, int start_ticks,
				    fixed leftmul, fixed rightmul)
{
	int start_sample = (start_ticks * rate) / TICRATE;
	int num_samples = (size * (long long)rate) / output_rate;
	if (isLooping) {
		start_sample %= sample_count;
	}
	if (start_sample >= sample_count || start_sample < 0)
		return;
	if (num_samples > sample_count - start_sample)
		num_samples = sample_count - start_sample;
	if (num_samples <= 0)
		return;
	switch (sample_format)
	{
	case FORMAT_8BIT_LINEAR_UNSIGNED:
	{
		mix<uint8_t, 0x8000>(result, output_rate,(uint8_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	case FORMAT_8BIT_LINEAR_SIGNED:
	{
		mix<int8_t, 0>(result, output_rate,(int8_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	case FORMAT_16BIT_LINEAR_SIGNED:
	{
		mix<int16_t, 0>(result, output_rate,(int16_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	}
}

Mix_Chunk_IMF::Mix_Chunk_IMF(int rate, const byte *imf, size_t imf_size,
			     bool isLooping)
{
	YM3812Init(1,3579545,synthesisRate);
	this->rate = rate;
	this->sample_count = 0;
	this->chunk_samples = NULL;
	this->sample_format = FORMAT_16BIT_LINEAR_SIGNED;
	this->isLooping = isLooping;
	this->samples_allocated = 0;
	this->imf = (byte*) malloc(imf_size * 4);
	CHECKMALLOCRESULT(this->imf);
	memcpy(this->imf, imf, imf_size * 4);
	this->imfptr = 0;
	this->imfsize = imf_size;

	static const Instrument ChannelRelease = {
		0, 0,
		0x3F, 0x3F,
		0xFF, 0xFF,
		0xF, 0xF,
		0, 0,
		0,

		0, 0, {0, 0, 0}
	};

	for (int i = 0;i < OPL_CHANNELS;++i)
		SDL_AlSetChanInst(musicOpl, &ChannelRelease, i);
}

Mix_Chunk *SynthesizeAdlibIMF(const byte *dataRaw, size_t size)
{
	int alLength = size / 4;
	const byte *alSound = dataRaw;
	if(alSound[0] != 0 || alSound[1] != 0) {
		alLength = LittleShort(*(WORD*)alSound) / 4;
		if (alLength > (size - 2) / 4)
			alLength = (size - 2) / 4;
		alSound += 2;
	}

	return new Mix_Chunk_IMF(synthesisRate, alSound, alLength, true);
}

void Mix_Chunk_IMF::EnsureSpace(int need_samples)
{
	if (samples_allocated >= need_samples)
		return;
	size_t nl = need_samples + need_samples / 2;
	if (nl < 256)
		nl = 256;
	chunk_samples = realloc(chunk_samples, nl * 2);
	samples_allocated = nl;
}

void Mix_Chunk_IMF::EnsureSynthesis(int maxTics)
{	
	for (;imfptr < imfsize && sample_count < maxTics * rate / TICRATE; imfptr++) {
		byte reg = imf[4*imfptr];
		byte val = imf[4*imfptr + 1];
		int tics = LittleShort(((WORD *)imf)[imfptr * 2 + 1]);
		YM3812Write(musicOpl, reg, val, 20);

		EnsureSpace(sample_count + samplesPerMusicTick * tics);

		while (tics) {
			int curtics = tics;
			if (curtics > 500 / samplesPerMusicTick)
				curtics = 500 / samplesPerMusicTick;
			YM3812UpdateOneMono(musicOpl, (int16_t *)chunk_samples + sample_count, samplesPerMusicTick * curtics);
			sample_count += samplesPerMusicTick * curtics;
			tics -= curtics;
		}
	}
//	printf ("synth: %d\n", sample_count);
}
