/* Mouse cursor functions
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include <prefs/pointer.h>
#include <prefs/prefhdr.h>
#include <intuition/pointerclass.h>

#include "types.h"
#include "funcs.h"


/*** generic constants ***/

#define POINTER_OFFSET 0x1000
#define POINTER_FILE CONFIG_PATH "/pointer.prefs"
#define SUPPORTED_VERSION 0


/*** PointerPrefs.pp_Reserved - fields ***/

#define PP_SIZE(x)   (x)->pp_Reserved[0]
#define PP_DATA(x)   (x)->pp_Reserved[1]
#define PP_OBJECT(x) (x)->pp_Reserved[2]

#define RGBTABLE(x) ((UBYTE *)x + sizeof(PointerPrefs))


struct PointerPrefs *pp[NUM_POINTER];
struct BitMap ppbm[NUM_POINTER];


void
SetMousePointer(struct Window *win, ULONG which)
{
    if (which == STANDARD_POINTER)
        ClearPointer(win);
    else
        SetWindowPointer(win, WA_Pointer, (pp[which] ? (struct PointerPrefs *)PP_OBJECT(pp[which]) : NULL), TAG_END);
}


static int32
loadPointers(STRPTR name)
{
    struct IFFHandle *iff;
	int32  error, count = 0;

    if (!(iff = AllocIFF()))
        return IFFERR_NOMEM;

    InitIFFasDOS(iff);
	iff->iff_Stream = (IPTR)Open(name, MODE_OLDFILE);

	if (!(error = OpenIFF(iff, IFFF_READ))) {
		//int32 chunks[] = {ID_PREF, ID_PRHD, ID_PREF, ID_PNTR};
        struct ContextNode *cn;

		//error = PropChunks(iff, chunks, 2);

		while (error >= 0 || error == IFFERR_EOC) {
			if ((error = ParseIFF(iff, IFFPARSE_STEP)) && error != IFFERR_EOC)
                break;

			if ((cn = CurrentChunk(iff)) != NULL && error != IFFERR_EOC) {
				// entering a chunk
				if (cn->cn_Type == ID_PREF) {
					switch (cn->cn_ID) {
                        case ID_PRHD:
                        {
                            struct PrefHeader ph;

							error = ReadChunkBytes(iff, &ph, sizeof(struct PrefHeader));
							if (error >= 0 && ph.ph_Version > SUPPORTED_VERSION) {
								D(bug("wrong version %ld (type = %ld)!\n", ph.ph_Version, ph.ph_Type));
								error = IFFERR_READ;
                            }
                            break;
                        }
                        case ID_PNTR:
							if (!(pp[count] = AllocPooled(pool, cn->cn_Size)))
                                break;

							error = ReadChunkBytes(iff, pp[count], cn->cn_Size);
							pp[count]->pp_Which = BE2WORD(pp[count]->pp_Which);
							pp[count]->pp_Size = BE2WORD(pp[count]->pp_Size);
							pp[count]->pp_Width = BE2WORD(pp[count]->pp_Width);
							pp[count]->pp_Height = BE2WORD(pp[count]->pp_Height);
							pp[count]->pp_Depth = BE2WORD(pp[count]->pp_Depth);
							pp[count]->pp_YSize = BE2WORD(pp[count]->pp_YSize);
							pp[count]->pp_X = BE2WORD(pp[count]->pp_X);
							pp[count]->pp_Y = BE2WORD(pp[count]->pp_Y);

							if (pp[count]->pp_Which != count + POINTER_OFFSET) {
								D(bug("Zeiger %d hat unzulässige ID %d.\n", count, pp[count]->pp_Which));
								FreePooled(pool, pp[count], cn->cn_Size);
                                pp[count] = NULL;
								error = IFFERR_MANGLED;
							} else {
                                PP_SIZE(pp[count]) = cn->cn_Size;
								PP_DATA(pp[count]) = (ULONG)(((1 << pp[count]->pp_Depth) - 1) * sizeof(struct RGBTable) + (UBYTE *)pp[count] + sizeof(struct PointerPrefs));
                                count++;
                            }
                            break;
                    }
                }
            }
        }

        CloseIFF(iff);
        if (error == IFFERR_EOF)
            error = 0;
    }
    Close((BPTR)iff->iff_Stream);
    FreeIFF(iff);

    return error;
}


void
FreePointers(void)
{
    int i;

	for (i = 0; i < NUM_POINTER; i++) {
        if (!pp[i])
            continue;

        DisposeObject((void *)PP_OBJECT(pp[i]));
        FreePooled(pool, pp[i], PP_SIZE(pp[i]));
    }
}


void
InitPointers(void)
{
    int i;

    if ((i = loadPointers(POINTER_FILE)) != 0)
        ErrorRequest(GetString(&gLocaleInfo, MSG_LOAD_MOUSE_POINTER_ERR), IFFErrorText(i));

	for (i = 0; i < NUM_POINTER; i++) {
        int j, wordwidth;

        if (!pp[i])
            continue;

        InitBitMap(&ppbm[i],pp[i]->pp_Depth,pp[i]->pp_Width,pp[i]->pp_Height);
        wordwidth = (pp[i]->pp_Width + 15) >> 4;
		for (j = 0; j < ppbm[i].Depth; j++) {
            ppbm[i].Planes[j] = (UBYTE *)PP_DATA(pp[i])+j*wordwidth*2*ppbm[i].Rows;
		}

        PP_OBJECT(pp[i]) = (ULONG)NewObject(NULL, "pointerclass",
            POINTERA_BitMap,      &ppbm[i],
            POINTERA_XOffset,     pp[i]->pp_X,
            POINTERA_YOffset,     pp[i]->pp_Y,
            POINTERA_WordWidth,   wordwidth,
            POINTERA_XResolution, pp[i]->pp_Size,
            POINTERA_YResolution, pp[i]->pp_YSize,
            TAG_END);
    }
}

