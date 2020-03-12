#ifndef __WL_INTER_H__
#define __WL_INTER_H__

/*
=============================================================================

								WL_INTER

=============================================================================
*/

extern struct LRstruct
{
	uint32_t killratio;
	uint32_t secretsratio;
	uint32_t treasureratio;
	uint32_t numLevels;
	uint32_t time;
	uint32_t par;
} LevelRatios;

void DrawHighScores(void);
void CheckHighScore (int32_t score, const class LevelInfo *levelInfo);
void Victory (bool fromIntermission);
void LevelCompleted (void);
void ClearSplitVWB (void);

void PreloadGraphics(bool showPsych);

#endif
