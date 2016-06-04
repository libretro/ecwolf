#include "version.h"

#include "actor.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "wl_shade.h"
#include "wl_main.h"
#include "c_cvars.h"

void Scale3DShaper(int x1, int x2, FTexture *shape, uint32_t flags, fixed ny1, fixed ny2,
				fixed nx1, fixed nx2, byte *vbuf, unsigned vbufPitch)
{
	//printf("%s(%d, %d, %p, %d, %f, %f, %f, %f, %p, %d)\n", __FUNCTION__, x1, x2, shape, flags, FIXED2FLOAT(ny1), FIXED2FLOAT(ny2), FIXED2FLOAT(nx1), FIXED2FLOAT(nx2), vbuf, vbufPitch);
	unsigned scale1,starty,endy;
	byte *vmem;
	int dx,len,i,newstart,ycnt,pixheight,screndy,upperedge,scrstarty;
	unsigned j;
	fixed height,dheight,height1,height2;
	int xpos[TEXTURESIZE+1];
	int slinex;
	fixed dxx=(ny2-ny1)<<8,dzz=(nx2-nx1)<<8;
	fixed dxa=0,dza=0;
	byte col;

	len=shape->GetWidth();
	if(!len) return;

	ny1+=dxx>>9;
	nx1+=dzz>>9;

	dxa=-(dxx>>1),dza=-(dzz>>1);
	dxx>>=TEXTURESHIFT,dzz>>=TEXTURESHIFT;

	xpos[0]=(int)((ny1+(dxa>>8))*scale/(nx1+(dza>>8))+centerx);
	height1 = heightnumerator/((nx1+(dza>>8))>>8);
	height=(((fixed)height1)<<12)+2048;

	//printf("{%d,", xpos[0]);
	for(i=1;i<=len;i++)
	{
		dxa+=dxx,dza+=dzz;
		xpos[i]=(int)((ny1+(dxa>>8))*scale/(nx1+(dza>>8))+centerx);
		//printf("%d,", xpos[i]);
		if(xpos[i-1]>viewwidth) break;
	}
	//printf("}\n");
	len=i-1;
	dx = xpos[len] - xpos[0];
	if(!dx) return;

	height2 = heightnumerator/((nx1+(dza>>8))>>8);
	dheight=(((fixed)height2-(fixed)height1)<<12)/(fixed)dx;

	if(x2>viewwidth) x2=viewwidth;

	for(i=0;i<len;i++)
	{
		const BYTE *line=shape->GetColumn(i, NULL);

		for(slinex=xpos[i];slinex<xpos[i+1] && slinex<x2;slinex++)
		{
			height+=dheight;
			if(slinex<0) continue;

			scale1=(unsigned)(height>>15);

			if(wallheight[slinex]<(height>>12) && scale1 /*&& scale1<=maxscale*/)
			{
#define SPRITESCALEFACTOR 2
				pixheight=scale1*SPRITESCALEFACTOR;
				upperedge=viewheight/2-scale1;

				if((endy = 128) != 0)
				{
					endy >>= 1;
					newstart = 0;
					starty = 0 >> 1;
					j=starty;
					ycnt=j*pixheight;
					screndy=(ycnt>>6)+upperedge;
					if(screndy<0) vmem=vbuf+slinex;
					else vmem=vbuf+screndy*vbufPitch+slinex;
					for(;j<endy;j++)
					{
						scrstarty=screndy;
						ycnt+=pixheight;
						screndy=(ycnt>>6)+upperedge;
						if(scrstarty!=screndy && screndy>0)
						{
							col=line[j];
							if(scrstarty<0) scrstarty=0;
							if(screndy>viewheight) screndy=viewheight,j=endy;

							while(scrstarty<screndy)
							{
								if(col)
									*vmem=col;
								vmem+=vbufPitch;
								scrstarty++;
							}
						}
					}
				}
			}
		}
	}
}
