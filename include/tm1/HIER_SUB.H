#ifndef __TM1_HIER_SUB_H__
#define __TM1_HIER_SUB_H__

#include "common.h"
#include <LIBGPU.H>

#include "tm1/RT.H"
#include "tm1/SCENE.H"

typedef union SubPolyWord {
    DVECTOR position;
    CVECTOR color;
    UVWord uv;
    s32 word;
    u8 bytes[4];
} SubPolyWord;

/* A near polygon queued by geomProc for subdivision. */
typedef struct SubPoly {
    /* 0x00 */ DbPoly* poly;
    /* 0x04 */ s32 otIdx;
    /* 0x08 */ SubPolyWord* prim; /* GPU primitive built by geomProc */
    /* 0x0C */ u16 tpage;
    /* 0x0E */ u16 dither;
    /* 0x10 */ u32* texWindow;
} SubPoly; /* 0x14 */

/* Scratchpad values shared by hierProc, geomProc and the HIER_SUB functions. */
extern volatile s32 sdSubPolyCount;
extern SubPoly* volatile sdSubPolyPtr;
extern volatile s32 sdMirrorView;

#define SPAD_DRMODE ((DR_MODE*)0x1F800060)
#define SPAD_DRMODE_DITHER ((DR_MODE*)0x1F80006C)

void subPoly3(Db* db, SubPoly* f, s32 count);
void subLoadPoly3Words(s32 col, s32 row);
void subPoly4(Db* db, SubPoly* f, s32 count);
void subLoadPoly4Words(s32 col, s32 row);
void subAddPrim(Db* db, SubPoly* f, SubPolyWord** cursor, SubPolyWord** extra);

#endif /* __TM1_HIER_SUB_H__ */
