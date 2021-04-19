/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.

This file is part of Quake 2 Tools source code.

Quake 2 Tools source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 Tools source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 Tools source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// lbmlib.c

#include "cmdlib.h"
#include "lbmlib.h"



/*
============================================================================

						LBM STUFF

============================================================================
*/


typedef unsigned char	UBYTE;
//conflicts with windows typedef short			WORD;
typedef unsigned short	UWORD;
typedef long			LONG;

typedef enum
{
	ms_none,
	ms_mask,
	ms_transcolor,
	ms_lasso
} mask_t;

typedef enum
{
	cm_none,
	cm_rle1
} compress_t;

typedef struct
{
	UWORD		w,h;
	short		x,y;
	UBYTE		nPlanes;
	UBYTE		masking;
	UBYTE		compression;
	UBYTE		pad1;
	UWORD		transparentColor;
	UBYTE		xAspect,yAspect;
	short		pageWidth,pageHeight;
} bmhd_t;

extern	bmhd_t	bmhd;						// will be in native byte order

#define STID( A, B, C, D ) ((A)+((B)<<8)+((int)(C)<<16)+((int)(D)<<24))

#define FORMID STID('F','O','R','M')
#define ILBMID STID('I','L','B','M')
#define PBMID  STID('P','B','M',' ')
#define BMHDID STID('B','M','H','D')
#define BODYID STID('B','O','D','Y')
#define CMAPID STID('C','M','A','P')


bmhd_t  bmhd;

int    Align (int l)
{
	if (l&1)
		return l+1;
	return l;
}



/*
================
LBMRLEdecompress

Source must be evenly aligned!
================
*/
byte  *LBMRLEDecompress (byte *source,byte *unpacked, int bpwidth)
{
	int     count;
	byte    b,rept;

	count = 0;

	do
	{
		rept = *source++;

		if (rept > 0x80)
		{
			rept = (rept^0xff)+2;
			b = *source++;
			memset(unpacked,b,rept);
			unpacked += rept;
		}
		else if (rept < 0x80)
		{
			rept++;
			memcpy(unpacked,source,rept);
			unpacked += rept;
			source += rept;
		}
		else
			rept = 0;               // rept of 0x80 is NOP

		count += rept;

	} while (count<bpwidth);

	if (count>bpwidth)
		Error ("Decompression exceeded width!\n");


	return source;
}


/*
=================
LoadLBM
=================
*/
void LoadLBM (char *filename, byte **picture, byte **palette)
{
	byte    *LBMbuffer, *picbuffer, *cmapbuffer;
	int             y;
	byte    *LBM_P, *LBMEND_P;
	byte    *pic_p;
	byte    *body_p;

	int    formtype,formlength;
	int    chunktype,chunklength;

// qiet compiler warnings
	picbuffer = nullptr;
	cmapbuffer = nullptr;

//
// load the LBM
//
	LoadFile (filename, (void **)&LBMbuffer);

//
// parse the LBM header
//
	LBM_P = LBMbuffer;
	if ( *(int *)LBMbuffer != LittleLong(FORMID) )
	   Error ("No FORM ID at start of file!\n");

	LBM_P += 4;
	formlength = BigLong( *(int *)LBM_P );
	LBM_P += 4;
	LBMEND_P = LBM_P + Align(formlength);

	formtype = LittleLong(*(int *)LBM_P);

	if (formtype != ILBMID && formtype != PBMID)
		Error ("Unrecognized form type: %c%c%c%c\n", formtype&0xff
		,(formtype>>8)&0xff,(formtype>>16)&0xff,(formtype>>24)&0xff);

	LBM_P += 4;

//
// parse chunks
//

	while (LBM_P < LBMEND_P)
	{
		chunktype = LBM_P[0] + (LBM_P[1]<<8) + (LBM_P[2]<<16) + (LBM_P[3]<<24);
		LBM_P += 4;
		chunklength = LBM_P[3] + (LBM_P[2]<<8) + (LBM_P[1]<<16) + (LBM_P[0]<<24);
		LBM_P += 4;

		switch ( chunktype )
		{
		case BMHDID:
			memcpy (&bmhd,LBM_P,sizeof(bmhd));
			bmhd.w = BigShort(bmhd.w);
			bmhd.h = BigShort(bmhd.h);
			bmhd.x = BigShort(bmhd.x);
			bmhd.y = BigShort(bmhd.y);
			bmhd.pageWidth = BigShort(bmhd.pageWidth);
			bmhd.pageHeight = BigShort(bmhd.pageHeight);
			break;

		case CMAPID:
			cmapbuffer = (byte*)malloc (768);
			memset (cmapbuffer, 0, 768);
			memcpy (cmapbuffer, LBM_P, chunklength);
			break;

		case BODYID:
			body_p = LBM_P;

			pic_p = picbuffer = (byte*)malloc (bmhd.w*bmhd.h);
			if (formtype == PBMID)
			{
			//
			// unpack PBM
			//
				for (y=0 ; y<bmhd.h ; y++, pic_p += bmhd.w)
				{
					if (bmhd.compression == cm_rle1)
						body_p = LBMRLEDecompress ((byte *)body_p
						, pic_p , bmhd.w);
					else if (bmhd.compression == cm_none)
					{
						memcpy (pic_p,body_p,bmhd.w);
						body_p += Align(bmhd.w);
					}
				}

			}
			else
			{
			//
			// unpack ILBM
			//
				Error ("%s is an interlaced LBM, not packed", filename);
			}
			break;
		}

		LBM_P += Align(chunklength);
	}

	free (LBMbuffer);

	*picture = picbuffer;

	if (palette)
		*palette = cmapbuffer;
}


/*
============================================================================

							WRITE LBM

============================================================================
*/

/*
==============
WriteLBMfile
==============
*/
void WriteLBMfile (char *filename, byte *data,
				   int width, int height, byte *palette)
{
	byte    *lbm, *lbmptr;
	int    *formlength, *bmhdlength, *cmaplength, *bodylength;
	int    length;
	bmhd_t  basebmhd;

	lbm = lbmptr = (byte*)malloc (width*height+1000);

//
// start FORM
//
	*lbmptr++ = 'F';
	*lbmptr++ = 'O';
	*lbmptr++ = 'R';
	*lbmptr++ = 'M';

	formlength = (int*)lbmptr;
	lbmptr+=4;                      // leave space for length

	*lbmptr++ = 'P';
	*lbmptr++ = 'B';
	*lbmptr++ = 'M';
	*lbmptr++ = ' ';

//
// write BMHD
//
	*lbmptr++ = 'B';
	*lbmptr++ = 'M';
	*lbmptr++ = 'H';
	*lbmptr++ = 'D';

	bmhdlength = (int *)lbmptr;
	lbmptr+=4;                      // leave space for length

	memset (&basebmhd,0,sizeof(basebmhd));
	basebmhd.w = BigShort((short)width);
	basebmhd.h = BigShort((short)height);
	basebmhd.nPlanes = 8;
	basebmhd.xAspect = 5;
	basebmhd.yAspect = 6;
	basebmhd.pageWidth = BigShort((short)width);
	basebmhd.pageHeight = BigShort((short)height);

	memcpy (lbmptr,&basebmhd,sizeof(basebmhd));
	lbmptr += sizeof(basebmhd);

	length = lbmptr-(byte *)bmhdlength-4;
	*bmhdlength = BigLong(length);
	if (length&1)
		*lbmptr++ = 0;          // pad chunk to even offset

//
// write CMAP
//
	*lbmptr++ = 'C';
	*lbmptr++ = 'M';
	*lbmptr++ = 'A';
	*lbmptr++ = 'P';

	cmaplength = (int *)lbmptr;
	lbmptr+=4;                      // leave space for length

	memcpy (lbmptr,palette,768);
	lbmptr += 768;

	length = lbmptr-(byte *)cmaplength-4;
	*cmaplength = BigLong(length);
	if (length&1)
		*lbmptr++ = 0;          // pad chunk to even offset

//
// write BODY
//
	*lbmptr++ = 'B';
	*lbmptr++ = 'O';
	*lbmptr++ = 'D';
	*lbmptr++ = 'Y';

	bodylength = (int *)lbmptr;
	lbmptr+=4;                      // leave space for length

	memcpy (lbmptr,data,width*height);
	lbmptr += width*height;

	length = lbmptr-(byte *)bodylength-4;
	*bodylength = BigLong(length);
	if (length&1)
		*lbmptr++ = 0;          // pad chunk to even offset

//
// done
//
	length = lbmptr-(byte *)formlength-4;
	*formlength = BigLong(length);
	if (length&1)
		*lbmptr++ = 0;          // pad chunk to even offset

//
// write output file
//
	SaveFile (filename, lbm, lbmptr-lbm);
	free (lbm);
}

/*
============================================================================

LOAD IMAGE

============================================================================
*/

/*
==============
Load256Image

Will load either an lbm or pcx, depending on extension.
Any of the return pointers can be NULL if you don't want them.
==============
*/
void Load256Image (char *name, byte **pixels, byte **palette,
				   int *width, int *height)
{
	char	ext[128];

	ExtractFileExtension (name, ext);
	if (!Q_strcasecmp (ext, "lbm"))
	{
		LoadLBM (name, pixels, palette);
		if (width)
			*width = bmhd.w;
		if (height)
			*height = bmhd.h;
	}
	else
		Error ("%s doesn't have a known image extension", name);
}
