#ifndef __WL_DRAW_H__
#define __WL_DRAW_H__

/*
=============================================================================

							WL_DRAW DEFINITIONS

=============================================================================
*/

//
// math tables
//
extern  short *pixelangle;
extern  int32_t finetangent[FINEANGLES/4];
extern	fixed finesine[FINEANGLES+FINEANGLES/4];
extern	fixed* finecosine;
extern  int *wallheight;
extern  word horizwall[],vertwall[];
extern  int32_t    lasttimecount;
extern  int32_t    frameon;

extern  unsigned screenloc[3];

extern  bool fizzlein, fpscounter;

extern  fixed   viewx,viewy;                    // the focal point
extern  fixed   viewsin,viewcos;

void    ThreeDRefresh (void);
void    CalcTics (void);

typedef struct
{
	word leftpix,rightpix;
	word dataofs[64];
// table data after dataofs[rightpix-leftpix+1]
} t_compshape;

#endif
