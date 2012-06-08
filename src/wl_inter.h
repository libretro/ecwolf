#ifndef __WL_INTER_H__
#define __WL_INTER_H__

/*
=============================================================================

								WL_INTER

=============================================================================
*/

void IntroScreen (void);
void DrawHighScores(void);
void CheckHighScore (int32_t score,word other);
void Victory (void);
void LevelCompleted (void);
void ClearSplitVWB (void);

void PreloadGraphics(void);

#endif
