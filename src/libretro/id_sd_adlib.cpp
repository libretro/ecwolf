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
#include "deps/tremor/id_sd_ogg.h"
#include "wl_play.h"
#include "dosbox/dbopl.h"

// Rates and the OPL block-buffer size now live in state_machine.h (included
// above) as the single source of truth shared with id_sd.cpp and
// id_sd_n3dmus.cpp. Keep short local aliases so the synthesis code below reads
// the same as before; the compile-time invariants are asserted in the header.
static const int synthesisRate        = SYNTHESIS_RATE;
static const int samplesPerSoundTick  = SAMPLES_PER_SOUND_TICK;
static const int samplesPerMusicTick  = SAMPLES_PER_MUSIC_TICK;
#undef alOut
#define alOut(chip,n,b) 	YM3812Write(chip, n, b, 20)

static inline void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, int volume)
{
	which.WriteReg(reg, val);
}

void YM3812UpdateOneMono(DBOPL::Chip &which, int16_t *stream, int length)
{
	Bit32s buffer[OPL_BLOCK_SAMPLES * 2];
	int i;

	// length is bounded by the EnsureSynthesis caller (samplesPerMusicTick *
	// (500 / samplesPerMusicTick) = 441 samples at the fixed 44100 Hz synthesis
	// rate), which is <= OPL_BLOCK_SAMPLES. The clamp below is a defensive cap;
	// a compile-time assert above guarantees the buffer is large enough so this
	// never actually truncates a real request. (The synthesis rate is fixed at
	// 44100 and independent of the frontend output rate, which the generic
	// resampler in MixSamples handles.)
	if(length > OPL_BLOCK_SAMPLES)
		length = OPL_BLOCK_SAMPLES;

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
			stream[i] = sample;
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
			stream[i] = sample;
		}
	}
}

void    SD_Startup_Adlib(void)
{
	/* Nothing to do: there is no shared OPL chip any more. Every synthesis
	 * site (SFX below, IMF and N3D music chunks) owns a chip constructed for
	 * the occasion, and the DBOPL::Chip constructor initialises the shared
	 * read-only wave tables on first use. */
}

static void SDL_AlSetChanInst(DBOPL::Chip &oplChip, const Instrument *inst, unsigned int chan)
{
	static const uint8_t chanOps[OPL_CHANNELS] = {
		0, 1, 2, 8, 9, 0xA, 0x10, 0x11, 0x12
	};
	uint8_t c,m;

	m = chanOps[chan]; // modulator cell for channel
	c = m + 3; // carrier cell for channel
	alOut(oplChip, m + alChar,inst->mChar);
	alOut(oplChip, m + alScale,inst->mScale);
	alOut(oplChip, m + alAttack,inst->mAttack);
	alOut(oplChip, m + alSus,inst->mSus);
	alOut(oplChip, m + alWave,inst->mWave);
	alOut(oplChip, c + alChar,inst->cChar);
	alOut(oplChip, c + alScale,inst->cScale);
	alOut(oplChip, c + alAttack,inst->cAttack);
	alOut(oplChip, c + alSus,inst->cSus);
	alOut(oplChip, c + alWave,inst->cWave);

	// Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);

	alOut(oplChip, chan + alFreqL,0);
	alOut(oplChip, chan + alFreqH,0);
	alOut(oplChip, chan + alFeedCon,0);
}
static void SDL_AlSetFXInst(DBOPL::Chip &oplChip, const Instrument *inst)
{
	SDL_AlSetChanInst(oplChip, inst, 0);
}

Mix_Chunk *SynthesizeAdlib(const uint8_t *dataRaw)
{
	AdLibSound *sound = (AdLibSound*) dataRaw;

	int alLength = LittleLong(sound->common.length);
	uint8_t alBlock = ((sound->block & 7) << 2) | 0x20;
	Instrument      *inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		I_FatalError("SDL_ALPlaySound() - Bad instrument");
	}

	// Own OPL chip for this synthesis, same ownership rule as the music
	// chunks: sounds are synthesised lazily and cached, so rendering them
	// through a shared chip made each cached waveform a function of whatever
	// happened to be synthesised before it. The previous sound's envelope
	// level, LFO/tremolo/vibrato phase and feedback history all bled in --
	// measured on representative instruments, an instant-attack sound
	// inherited a lone +11268 click at its very first sample (vs 144 from a
	// silent chip), and a slow-attack sound diverged on 8811 of 8820 samples
	// with peaks 15340 apart. A fresh chip makes every cached sound the same
	// bytes no matter when or after what it was first played. Heap-allocated
	// because DBOPL::Chip is too large to be casual stack on the small
	// handheld ports.
	DBOPL::Chip *sfxOpl = new DBOPL::Chip();
	sfxOpl->Setup(synthesisRate);

	SDL_AlSetFXInst(*sfxOpl, inst);
	uint8_t *alSound = (uint8_t *)sound->data;

	// Bounded release ring-out after the last data byte (see below). Half a
	// second at 140 Hz service ticks; typical Wolf3D release rates reach true
	// silence within a few ticks and the loop stops there.
	const int maxReleaseTicks = 70 / 2;

	int16_t *samples = (int16_t*) malloc ((alLength + maxReleaseTicks) * samplesPerSoundTick * 2);
	CHECKMALLOCRESULT(samples);
	int16_t *sampleptr = samples;

	for (int i = 0; i < alLength; i++, alSound++) {
		// Vanilla SDL_ALSoundService semantics (ID_SD.C, kept by upstream
		// ECWolf): a zero data byte writes alFreqL = 0 and leaves the channel
		// KEYED ON -- the phase counter freezes at fnum 0, the output holds
		// its last sample, and the envelope stays in sustain, so the tone
		// resumes seamlessly at the next nonzero byte with no new attack.
		// This port used to key the channel off instead (alFreqH = 0), which
		// put every gap into release and re-triggered a full envelope attack
		// plus phase reset at every gap end. Since most Wolf3D effects use
		// zero bytes as rests/articulation, that rewrote their whole shape:
		// A/B on a representative articulated instrument, 7558 of 9450
		// samples differed with peaks 25272 apart (~77%% of full scale).
		if(*alSound)
		{
			alOut(*sfxOpl, alFreqL, *alSound);
			alOut(*sfxOpl, alFreqH, alBlock);
		} else alOut(*sfxOpl, alFreqL, 0);

		// Vanilla keys the channel off in the same 140 Hz service tick that
		// consumes the final data byte, so the last interval renders in
		// release, not sustain.
		if (i == alLength - 1)
			alOut(*sfxOpl, alFreqH, 0);

		YM3812UpdateOneMono(*sfxOpl, sampleptr, samplesPerSoundTick);
		sampleptr += samplesPerSoundTick;
	}

	// On hardware the chip keeps ringing its release after the sound "ends";
	// the cached chunk used to hard-cut at the last tick, which left any
	// sound not already silent -- including the held-sample gap state above --
	// ending on a step, i.e. a click. Render the release until a whole tick
	// comes out as exact digital silence (DBOPL outputs literal zeros once
	// the envelope is silent), bounded by maxReleaseTicks. This makes the
	// chunk slightly longer than the data, so the mixer channel stays
	// occupied for the ring-out; vanilla reported the sound finished at data
	// end while the chip rang, a divergence bounded by the half-second cap
	// and by the few-tick reality of typical release rates.
	for (int i = 0; i < maxReleaseTicks; i++) {
		int16_t *tickstart = sampleptr;
		int s;
		YM3812UpdateOneMono(*sfxOpl, sampleptr, samplesPerSoundTick);
		sampleptr += samplesPerSoundTick;
		for (s = 0; s < samplesPerSoundTick; s++)
			if (tickstart[s] != 0)
				break;
		if (s == samplesPerSoundTick) {
			// Fully silent: drop this all-zero tick and stop.
			sampleptr = tickstart;
			break;
		}
	}
	delete sfxOpl;

	{
		// Shrink the allocation to what was actually rendered.
		int16_t *shrunk = (int16_t*) realloc (samples, (sampleptr - samples) * 2);
		if (shrunk != NULL) {
			sampleptr = shrunk + (sampleptr - samples);
			samples = shrunk;
		}
	}
	return new Mix_Chunk_Digital(
		synthesisRate,
		samples,
		sampleptr - samples,
		FORMAT_16BIT_LINEAR_SIGNED_NATIVE,
		false
		);
}

Mix_Chunk_IMF::Mix_Chunk_IMF(int rate, const uint8_t *imf, size_t imf_size,
			     bool isLooping)
{
	this->rate = rate;
	this->sample_count = 0;
	this->chunk_samples = NULL;
	this->sample_format = FORMAT_16BIT_LINEAR_SIGNED_NATIVE;
	this->isLooping = isLooping;
	this->samples_allocated = 0;
	this->imf = (uint8_t*) malloc(imf_size * 4);
	CHECKMALLOCRESULT(this->imf);
	memcpy(this->imf, imf, imf_size * 4);
	this->imfptr = 0;
	this->imfsize = imf_size;

	// Own OPL chip, set up to this chunk's synthesis rate, so interleaved
	// lazy synthesis of different cached tracks doesn't share register state.
	this->imfOpl = new DBOPL::Chip();
	this->imfOpl->Setup(synthesisRate);

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
		SDL_AlSetChanInst(*imfOpl, &ChannelRelease, i);
}

// Out-of-line so DBOPL::Chip is a complete type at the delete site (see the
// declaration in state_machine.h).
Mix_Chunk_IMF::~Mix_Chunk_IMF()
{
	free (imf);
	delete imfOpl;
}

Mix_Chunk *SynthesizeAdlibIMFOrN3D(const uint8_t *dataRaw, size_t size)
{
	// Ogg Vorbis music track (e.g. a high-quality replacement for the IMF
	// tune). Decode to mono 16-bit PCM and play it as a looping digital
	// chunk; MixSamples resamples the native rate to the output rate.
	if (OggIsOgg(dataRaw, size)) {
		int ogg_rate = 0, ogg_samples = 0;
		int16_t *pcm = OggDecodeToMonoPCM(dataRaw, size,
						  synthesisRate, &ogg_rate, &ogg_samples);
		if (pcm != NULL)
			return new Mix_Chunk_Digital(ogg_rate, pcm, ogg_samples,
						     FORMAT_16BIT_LINEAR_SIGNED_NATIVE,
						     true);
		// Fall through to OPL synthesis on decode failure (the lump is
		// almost certainly not really an IMF/N3D tune, but returning a
		// best-effort silent IMF is preferable to a hard failure).
	}

	if (midiN3DValidate(dataRaw, size)) {
		return new Mix_Chunk_N3D(synthesisRate, dataRaw, size, true);
	}
	int alLength = size / 4;
	const uint8_t *alSound = dataRaw;
	if(alSound[0] != 0 || alSound[1] != 0) {
		alLength = ReadLittleShort(alSound) / 4;
		if (alLength > (int) (size - 2) / 4)
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
		uint8_t reg = imf[4*imfptr];
		uint8_t val = imf[4*imfptr + 1];
		int tics = ReadLittleShort((uint8_t *)imf + imfptr * 4 + 2);
		YM3812Write(*imfOpl, reg, val, 20);

		EnsureSpace(sample_count + samplesPerMusicTick * tics);

		while (tics) {
			int curtics = tics;
			if (curtics > 500 / samplesPerMusicTick)
				curtics = 500 / samplesPerMusicTick;
			YM3812UpdateOneMono(*imfOpl, (int16_t *)chunk_samples + sample_count, samplesPerMusicTick * curtics);
			sample_count += samplesPerMusicTick * curtics;
			tics -= curtics;
		}
	}
//	printf ("synth: %d\n", sample_count);
}
