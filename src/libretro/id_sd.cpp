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
#include "dosbox/dbopl.h"

static  bool					SD_Started;
static  bool					nextsoundpos;
static  int                     LeftPosition;
static  int                     RightPosition;
static const int synthesisRate = 44100;
#define SOUND_RATE 140	// Also affects PC Speaker sounds
static const int samplesPerSoundTick = synthesisRate / SOUND_RATE;
uint64_t used_digital_gen;
size_t loaded_sound_size;
size_t touched_sound_size;

// Saturating S16 accumulate. Mixing sums multiple mono voices (up to
// MIX_CHANNELS + music) into one stereo buffer; the per-voice value is computed
// at full int precision and this clamps the running sum to the int16 range so
// overlapping loud voices clip cleanly instead of wrapping (which sounds like
// loud crackle).
static inline void mix_acc(int16_t *dst, int value)
{
	int sum = (int)*dst + value;
	if (sum > 32767)
		sum = 32767;
	else if (sum < -32768)
		sum = -32768;
	*dst = (int16_t)sum;
}
  
FString                 SoundPlaying;
globalsoundpos channelSoundPos[MIX_CHANNELS];
struct SynthCacheItem
{
	TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> adlibSynth;
	TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> pcSynth;
};

static TArray<SynthCacheItem> synthCache;
static TMap<int, Mix_Chunk_Digital *> unloadableSounds;
static TMap<int, TUniquePtr<Mix_Chunk_Digital, Mix_ChunkDeleter> > digiProxies;

#define PC_BASE_TIMER 1193181

void decreaseSoundCache(size_t target)
{
	while (loaded_sound_size > target) { // TODO: use better algorithm
		uint64_t leastRecentlyUsed = ~(uint64_t)0;
		int leastRecentlyUsedIndex = -1;
		TMap<int, Mix_Chunk_Digital *>::Iterator it(unloadableSounds);
		TMap<int, Mix_Chunk_Digital *>::Pair *pair;
		while (it.NextPair(pair)) {
			if (pair->Value->lastUsed < leastRecentlyUsed) {
				leastRecentlyUsed = pair->Value->lastUsed;
				leastRecentlyUsedIndex = pair->Key;
			}
		}

		if (leastRecentlyUsedIndex < 0)
			break;
		unloadableSounds[leastRecentlyUsedIndex]->unloadSound();
	}
}

void    SD_Startup(void)
{
	if (SD_Started)
		return;

	SoundInfo.Init();
	SoundSeq.Init();
	SoundPlaying = FString();
#ifndef DISABLE_ADLIB
	SD_Startup_Adlib();
#endif

	SD_Started = true;
}

void SD_Shutdown(void)
{
	digiProxies.Clear();
	unloadableSounds.Clear();
	synthCache.Clear();

	SD_Started = false;
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

	~MusicCacheItem() {
		delete chunk;
	}
};

#define MUSIC_CACHE_SIZE 20

static MusicCacheItem music_cache[MUSIC_CACHE_SIZE];
static int cacheptr;

Mix_Chunk *GetMusic(const char* chunk)
{
#ifdef DISABLE_ADLIB
	return NULL;
#else	
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

	Mix_Chunk *res = SynthesizeAdlibIMFOrN3D((const uint8_t *) soundLump.GetMem(), size);

	music_cache[cacheptr].name = strdup(chunk);
	music_cache[cacheptr].chunk = res;
	cacheptr++;
	return res;
#endif
}

void    SD_ContinueMusic(const char* chunk, int startoffs)
{
	uint32_t currentTick = GetTimeCount();

	SD_MusicOff();

	Mix_Chunk *sample = GetMusic(chunk);
	if (!sample)
		return;

	g_state.musicChannel.sample = sample;
	g_state.musicChannel.sound = chunk;
	g_state.musicChannel.type = SoundData::ADLIB;
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
	if (which < 0)
		return NULL;
	if (digiProxies.CheckKey(which)) {
		return new Mix_Chunk_Proxy(digiProxies[which]);
	}
	
	int size = Wads.LumpLength(which);
	if(size == 0)
		return NULL;

	Mix_Chunk_Digital *ret = new Mix_Chunk_Digital(which);

	if (ret && preload_digital_sounds) {
		ret->loadSound();
	}

	digiProxies[which] = ret;

	return new Mix_Chunk_Proxy(ret);
}

Mix_Chunk_Digital::~Mix_Chunk_Digital() {
	unloadSound();
}

void Mix_Chunk_Digital::unloadSound() {
	if (!isLoaded() || !reloadable) {
		return;
	}

	free(chunk_samples);
	chunk_samples = NULL;
	loaded_sound_size -= GetDataSize();
	unloadableSounds.Remove(whichLump);
}

void Mix_Chunk_Digital::loadSound() {
	if (isLoaded()) {
		return;
	}

	FMemLump soundLump = Wads.ReadLump(whichLump);
	size_t size = soundLump.GetSize();
	void *mem = soundLump.GetMem();

	// 0x2A is the size of the sound header. From what I can tell the csnds
	// have mostly garbage filled headers (outside of what is precisely needed
	// since the sample rate is hard coded). I'm not sure if the sounds are
	// 8-bit or 16-bit, but it looks like the sample rate is coded to ~22050.
	if(size > 0x2A && BigShort(*(uint16_t*)mem) == 1)
	{
		sample_count = size - 0x2a;

		rate = 22050;
		sample_format = FORMAT_8BIT_LINEAR_UNSIGNED;
		isLooping = false;
		isValid = true;
		isMetadataLoaded = true;

		chunk_samples = malloc (size - 0x2a);
		if (chunk_samples) {
			memcpy(chunk_samples, ((char*)mem)+0x2A, size-0x2A);
			loaded_sound_size += size-0x2A;
			unloadableSounds[whichLump] = this;
		}

		return;
	}

	// Parse the WAV by walking its RIFF chunks rather than assuming a fixed
	// 44-byte header. Real-world packs ship WAVs with an extended fmt chunk or
	// extra chunks (LIST/fact/INFO) before data, so the audio does not always
	// start at offset 44. Reading from a hard-coded offset plays chunk headers
	// as samples, which comes out as loud noise.
	if (size > 12 && memcmp(mem, "RIFF", 4) == 0
	    && memcmp((char*)mem + 8, "WAVE", 4) == 0) {
		const uint8_t *base = (const uint8_t *)mem;
		size_t pos = 12; // past "RIFF"<size>"WAVE"
		const uint8_t *fmt = NULL;
		size_t fmt_size = 0;
		const uint8_t *pcm = NULL;
		size_t pcm_size = 0;

		while (pos + 8 <= size) {
			const uint8_t *id = base + pos;
			uint32_t clen = ReadLittleLong(base + pos + 4);
			size_t body = pos + 8;
			if (body > size)
				break;
			if (clen > size - body)
				clen = (uint32_t)(size - body); // clamp truncated chunk
			if (memcmp(id, "fmt ", 4) == 0) {
				fmt = base + body;
				fmt_size = clen;
			} else if (memcmp(id, "data", 4) == 0) {
				pcm = base + body;
				pcm_size = clen;
			}
			// Chunks are word-aligned: an odd length is padded by one byte.
			pos = body + clen + (clen & 1);
		}

		if (!fmt || fmt_size < 16 || !pcm || pcm_size == 0) {
			printf ("Unknown WAV variant (malformed)\n");
			sample_count = 0;
			isValid = false;
			isMetadataLoaded = true;
			return;
		}

		int format = ReadLittleShort(fmt + 0);
		int channels = ReadLittleShort(fmt + 2);
		rate = ReadLittleLong(fmt + 4);
		int bits = ReadLittleShort(fmt + 14);

		if (format != 1 || (bits != 8 && bits != 16) ||
		    (channels != 1 && channels != 2)) {
			printf ("Unknown WAV variant %d/%d/%d\n",
				bits, channels, format);
			sample_count = 0;
			isValid = false;
			isMetadataLoaded = true;
			return;
		}

		int bytes_per_sample = bits / 8;
		int frame_bytes = bytes_per_sample * channels;
		int frames = (int)(pcm_size / frame_bytes);
		if (frames <= 0) {
			sample_count = 0;
			isValid = false;
			isMetadataLoaded = true;
			return;
		}
		sample_count = frames;
		// 8-bit WAV PCM is unsigned (0..255, silence at 128); 16-bit WAV PCM is
		// signed. Decoding 8-bit as signed inverts and DC-biases the waveform,
		// which plays back as loud distortion, so use the unsigned format for it.
		sample_format = (bits == 8) ? FORMAT_8BIT_LINEAR_UNSIGNED
					    : FORMAT_16BIT_LINEAR_SIGNED_NATIVE;
		isLooping = false;

		chunk_samples = malloc ((size_t)frames * bytes_per_sample);
		if (chunk_samples) {
			if (bits == 8) {
				const uint8_t *in = pcm;
				uint8_t *out = (uint8_t *)chunk_samples;
				for (int i = 0; i < frames; i++) {
					if (channels == 1) {
						out[i] = in[i];
					} else {
						// Downmix stereo to mono (unsigned).
						int s = (int)in[i * 2] +
							(int)in[i * 2 + 1];
						out[i] = (uint8_t)(s / 2);
					}
				}
			} else { // 16-bit
				int16_t *out = (int16_t *)chunk_samples;
				for (int i = 0; i < frames; i++) {
					if (channels == 1) {
						out[i] = (int16_t)ReadLittleShort(pcm + i * 2);
					} else {
						int l = (int16_t)ReadLittleShort(pcm + (i * 2) * 2);
						int r = (int16_t)ReadLittleShort(pcm + (i * 2 + 1) * 2);
						out[i] = (int16_t)((l + r) / 2);
					}
				}
			}
			loaded_sound_size += (size_t)frames * bytes_per_sample;
			unloadableSounds[whichLump] = this;
		}
		isValid = true;
		isMetadataLoaded = true;

		return;
	}

	printf ("unknown format. Header: %x\n", BigLong(*(uint32_t*)mem));
	isValid = false;
	isMetadataLoaded = true;
	return;
}

Mix_Chunk_Speaker::Mix_Chunk_Speaker(const uint8_t *dataRaw)
{
	PCSound *sound = (PCSound*) dataRaw;
	int pcLength = LittleLong(sound->common.length);
	uint8_t *pcSound = sound->data;

	if (pcLength <= 0) {
		length_pc_ticks = 0;
		return;
	}

	states = (speaker_state *) malloc(pcLength * sizeof(speaker_state));
	length_pc_ticks = pcLength;

	uint32_t pcPhaseTick = 0;
	int pcLastSample = 0;
	uint32_t	pcPhaseLength = 0;
	bool sign = false;

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

		states[i].phaseTick = pcPhaseTick;
		states[i].phaseLength = pcPhaseLength;
		states[i].sign = sign;

		pcPhaseTick += samplesPerSoundTick;
		if ((pcPhaseTick / pcPhaseLength) & 1) 
		{
			sign = !sign;
		}
		pcPhaseTick %= pcPhaseLength;
	}
}

void Mix_Chunk_Speaker::MixInto(int16_t *result, int output_rate, size_t size, int start_ticks,
				fixed leftmul, fixed rightmul)
{
	int start_pc_tick = start_ticks * (SOUND_RATE / TICRATE);
	int num_pc_ticks = (size * (long long)SOUND_RATE + output_rate - 1) / output_rate;
	int16_t *outptr = result;
	size_t sample_ctr = 0;

	if(output_rate != synthesisRate) {
		printf("Speaker resampling is not supported (from %d to %d)\n", synthesisRate, output_rate);
		return;
	}
	if (start_pc_tick >= length_pc_ticks || start_pc_tick < 0)
		return;
	if (num_pc_ticks > length_pc_ticks - start_pc_tick)
		num_pc_ticks = length_pc_ticks - start_pc_tick;
	if (num_pc_ticks <= 0)
		return;

	for (int pc_tick = start_pc_tick; pc_tick < start_pc_tick + num_pc_ticks; pc_tick++) {
		uint32_t pcPhaseLength = states[pc_tick].phaseLength;
		uint32_t pcPhaseTick = states[pc_tick].phaseTick;
		bool sign = states[pc_tick].sign;

		for (int j = 0; j < samplesPerSoundTick && sample_ctr < size; j++, sample_ctr++) {
			int val = sign ? 5000 : -5000;
			int l = (val * leftmul) >> FRACBITS;
			int r = (val * rightmul) >> FRACBITS;
			mix_acc(outptr++, l);
			mix_acc(outptr++, r);
			// Update the PC speaker state:
			if(pcPhaseTick++ >= pcPhaseLength)
			{
				sign = !sign;
				pcPhaseTick = 0;
			}
		}
	}
}

int Mix_Chunk_Speaker::GetLengthTicks() {
	return (length_pc_ticks + 1) / (SOUND_RATE / TICRATE);
}

Mix_Chunk *GetSoundDataType(const SoundData &which, SoundData::Type type)
{
	if (!which.HasType(type))
		return NULL;
	int idx = which.GetIndex();
	if (idx < 0)
		return NULL;
	if (idx >= (int) synthCache.Size())
		synthCache.Resize(idx + 1);
	SynthCacheItem &synth = synthCache[idx];
	switch(type) {
	case SoundData::DIGITAL:
		return which.GetDigitalData();
#ifndef DISABLE_ADLIB
	case SoundData::ADLIB:
		if (!synth.adlibSynth)
			synth.adlibSynth.Reset(SynthesizeAdlib(which.GetAdLibData()));
		return synth.adlibSynth;
#endif
	case SoundData::PCSPEAKER:
		if (!synth.pcSynth)
			synth.pcSynth.Reset(new Mix_Chunk_Speaker(which.GetSpeakerData()));
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
	arc << (uint32_t &) type;
	arc << isMusic;

	if (!arc.IsStoring())
		sample = isMusic ? GetMusic(sound) : GetSoundDataType(SoundInfo[sound], type);
}

template <typename T, int bias>
static void
mix_no_upscale(int16_t *result, const T *input, int num_samples, fixed leftmul, fixed rightmul)
{
	const int shift = FRACBITS - (16 - (int)sizeof(T) * 8);
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int l = (int)(((long long)(input[i] - bias) * leftmul) >> shift);
		int r = (int)(((long long)(input[i] - bias) * rightmul) >> shift);
		mix_acc(outptr++, l);
		mix_acc(outptr++, r);
	}
}

template <typename T, int bias>
static void
mix_int_upscale(int16_t *result, const T *input, int num_samples, int upsample, fixed leftmul, fixed rightmul)
{
	const int shift = FRACBITS - (16 - (int)sizeof(T) * 8);
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int l = (int)(((long long)(input[i] - bias) * leftmul) >> shift);
		int r = (int)(((long long)(input[i] - bias) * rightmul) >> shift);
		for (int j = 0 ; j < upsample; j++) {
			mix_acc(outptr++, l);
			mix_acc(outptr++, r);
		}
	}
}

template <typename T, int bias>
static void
mix_fractional_upscale(int16_t *result, int output_rate, const T *input, int input_rate, int num_samples, fixed leftmul, fixed rightmul)
{
	const int shift = FRACBITS - (16 - (int)sizeof(T) * 8);
	int16_t *outptr = result;
	for (int i = 0; i < num_samples; i++) {
		int l = (int)(((long long)(input[i] - bias) * leftmul) >> shift);
		int r = (int)(((long long)(input[i] - bias) * rightmul) >> shift);
		while ((outptr - result) / 2 * (long long)input_rate < i * (long long) output_rate) {
			mix_acc(outptr++, l);
			mix_acc(outptr++, r);
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
	if (chunk_samples == NULL || sample_count <= 0)
		return;
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
		mix<uint8_t, 0x80>(result, output_rate,(uint8_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	case FORMAT_8BIT_LINEAR_SIGNED:
	{
		mix<int8_t, 0>(result, output_rate,(int8_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	case FORMAT_16BIT_LINEAR_SIGNED_NATIVE:
	{
		mix<int16_t, 0>(result, output_rate,(int16_t *) chunk_samples + start_sample, rate, num_samples, leftmul, rightmul);
		break;
	}
	}
}

void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume)
{
#ifndef DISABLE_ADLIB
	which.SetVolume(volume);
	which.WriteReg(reg, val);
#endif
}
