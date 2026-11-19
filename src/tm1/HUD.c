#include "common.h"
#include <ABS.H>
#include <LIBGPU.H>
#include <LIBGTE.H>

#include "tm1/GRUTILS.H"
#include "tm1/HUD.H"
#include "tm1/RT.H"
#include "tm1/UA.H"

typedef struct Radar {
    /* 0x000 */ POLY_F4 bg[2];
    /* 0x030 */ LINE_F2 sig[48];
    /* 0x330 */ LINE_F2 mark[2];
    /* 0x350 */ DR_MODE mode;
    /* 0x35C */ u8 sigCount0;
    /* 0x35D */ u8 sigCount1;
    /* 0x35E */ u8 sigCount2;
    /* 0x35F */ u8 sigCount3;
    /* 0x360 */ s32 sigTotal;
    /* 0x364 */ s32 range;
    /* 0x368 */ s32 scale;
    /* 0x36C */ s32 buffer;
} Radar; /* 0x370 */

u8 radarOn = 1;

static GrSprite ArrowUpInfo;
static GrSprite ArrowDownInfo;
static GrSprite ArrowLeftInfo;
static GrSprite ArrowRightInfo;

static Radar radar;

void hudResetRadar(void)
{
    const s32* buffer = &radar.buffer;
    radar.sigTotal = 0;
    radar.sigCount2 = 0;
    radar.sigCount3 = 0;
    radar.sigCount1 = 0;
    radar.sigCount0 = 0;
    radar.buffer = 1 - *buffer;
}

void hudDisplayRadar(u_long* ot)
{
    Db* cdb;
    SPRT* p;
    s32 i;
    u8* end;

    if (rtIsSplitScreenOn() == 0 && radarOn != 0) {
        cdb = rtGetCdb();
        if (radar.sigCount0 != 0) {
            p = (SPRT*)cdb->prim;
            end = cdb->prim + 128;
            if (end < cdb->areaEnd) {
                DR_MODE* m = (DR_MODE*)(p + 1);
                cdb->prim = cdb->prim + 128;
                setSprt(p);
                setRGB0(p, 0x80, 0x80, 0x80);
                SetDrawMode(m, 0, 0, 0, NULL);
                setShadeTex(p, 1);
                setXY0(p, 0x2F, 0x1A);
                setWH(p, ArrowUpInfo.w, ArrowUpInfo.h);
                SetSemiTrans(p, 1);
                setUV0(p, ArrowUpInfo.u0 + 1, ArrowUpInfo.v0);
                SetDrawMode(m, 0, 0, ArrowUpInfo.tpage, NULL);
                p->clut = ArrowUpInfo.clut;
                addPrim(ot, p);
                addPrim(ot, m);
            }
        }
        if (radar.sigCount1 != 0) {
            p = (SPRT*)cdb->prim;
            end = cdb->prim + 128;
            if (end < cdb->areaEnd) {
                DR_MODE* m = (DR_MODE*)(p + 1);
                cdb->prim = cdb->prim + 128;
                setSprt(p);
                setRGB0(p, 0x80, 0x80, 0x80);
                SetDrawMode(m, 0, 0, 0, NULL);
                setShadeTex(p, 1);
                setXY0(p, 0x2F, 0x4D);
                setWH(p, ArrowDownInfo.w, ArrowDownInfo.h);
                SetSemiTrans(p, 1);
                setUV0(p, ArrowDownInfo.u0 + 1, ArrowDownInfo.v0);
                SetDrawMode(m, 0, 0, ArrowDownInfo.tpage, NULL);
                p->clut = ArrowDownInfo.clut;
                addPrim(ot, p);
                addPrim(ot, m);
            }
        }
        if (radar.sigCount3 != 0) {
            p = (SPRT*)cdb->prim;
            cdb->prim = cdb->prim + 128;
            end = cdb->prim + 128;
            if (end < cdb->areaEnd) {
                DR_MODE* m = (DR_MODE*)(p + 1);
                setSprt(p);
                setRGB0(p, 0x80, 0x80, 0x80);
                SetDrawMode(m, 0, 0, 0, NULL);
                setShadeTex(p, 1);
                setXY0(p, 0xA, 0x31);
                setWH(p, ArrowLeftInfo.w, ArrowLeftInfo.h);
                SetSemiTrans(p, 1);
                setUV0(p, ArrowLeftInfo.u0, ArrowLeftInfo.v0);
                SetDrawMode(m, 0, 0, ArrowLeftInfo.tpage, NULL);
                p->clut = ArrowLeftInfo.clut;
                addPrim(ot, p);
                addPrim(ot, m);
            }
        }
        if (radar.sigCount2 != 0) {
            p = (SPRT*)cdb->prim;
            end = cdb->prim + 128;
            if (end < cdb->areaEnd) {
                DR_MODE* m = (DR_MODE*)(p + 1);
                cdb->prim = cdb->prim + 128;
                setSprt(p);
                setRGB0(p, 0x80, 0x80, 0x80);
                SetDrawMode(m, 0, 0, 0, NULL);
                setShadeTex(p, 1);
                setXY0(p, 0x59, 0x31);
                setWH(p, ArrowRightInfo.w, ArrowRightInfo.h);
                SetSemiTrans(p, 1);
                setUV0(p, ArrowRightInfo.u0 + 1, ArrowRightInfo.v0);
                SetDrawMode(m, 0, 0, ArrowRightInfo.tpage, NULL);
                p->clut = ArrowRightInfo.clut;
                addPrim(ot, p);
                addPrim(ot, m);
            }
        }
        for (i = 0; i < radar.sigTotal; i++) {
            AddPrim(ot, &radar.sig[radar.buffer + (i * 2)]);
        }
        AddPrim(ot, &radar.mark[0]);
        AddPrim(ot, &radar.mark[1]);
        AddPrim(ot, &radar.bg[0]);
        AddPrim(ot, &radar.mode);
        hudResetRadar();
    }
}

void hudInitRadar(void)
{
    s32 i;

    setPolyF4(&radar.bg[0]);
    setPolyF4(&radar.bg[1]);
    SetSemiTrans(&radar.bg[0], 1);
    SetSemiTrans(&radar.bg[1], 1);
    setRGB0(&radar.bg[0], 0x34, 0x71, 0x11);
    setRGB0(&radar.bg[1], 0x34, 0x71, 0x11);
    setXY4(&radar.bg[0], 0x10, 0x20, 0x59, 0x20, 0x10, 0x4D, 0x59, 0x4D);
    setXY4(&radar.bg[1], 0x10, 0x20, 0x59, 0x20, 0x10, 0x4D, 0x59, 0x4D);
    for (i = 0; i < 48; i++) {
        setLineF2(&radar.sig[i]);
        SetSemiTrans(&radar.sig[i], 0);
    }
    setLineF2(&radar.mark[0]);
    setLineF2(&radar.mark[1]);
    setRGB0(&radar.mark[0], 0, 0x30, 0);
    setRGB0(&radar.mark[1], 0, 0x30, 0);
    setXY2(&radar.mark[0], 0x33, 0x37, 0x37, 0x37);
    setXY2(&radar.mark[1], 0x35, 0x36, 0x35, 0x38);
    SetDrawMode(&radar.mode, 0, 0, 0x1C, 0);
    radar.range = 0x1F40;
    radar.scale = 0x14D555;
    radar.sigTotal = 0;
    radar.buffer = 0;
}

void hudAddRadarSig(s32 id, VECTOR3* pos, s32 ang)
{
    s32 x;
    s32 z;
    s32 sx;
    s32 sz;
    s32 cosine;
    s32 sine;
    s32 buf;
    s32 n;
    LINE_F2* a;
    LINE_F2* b;

    if (id == UA_HEALTH_STAND && radar.buffer != 0) {
        return;
    }
    x = pos->vx;
    z = pos->vy;
    cosine = rcos(ang);
    sine = rsin(ang);
    sx = (x * cosine - z * sine) / radar.scale;
    sine = rsin(ang);
    cosine = rcos(ang);
    sz = -(x * sine + z * cosine) / radar.scale;
    if ((u32)sx + 36u >= 72 || (u32)sz + 22u >= 44 || radar.sigTotal + 2 >= 25) {
        if (id != UA_HEALTH_STAND) {
            if ((u32)sx + 36u < 72) {
                if (sz > 0) {
                    radar.sigCount1 = 1;
                } else {
                    radar.sigCount0 = 1;
                }
            } else if (sx < 0 && ABS(sx) > ABS(sz)) {
                radar.sigCount3 = 1;
            } else if (sx > 0 && ABS(sx) > ABS(sz)) {
                radar.sigCount2 = 1;
            } else if (sz > 0) {
                radar.sigCount1 = 1;
            } else {
                radar.sigCount0 = 1;
            }
        }
    } else {
        buf = radar.buffer;
        sx += 53;
        sz += 55;
        n = radar.sigTotal * 2;
        a = &radar.sig[buf + n];
        n += 2;
        b = &radar.sig[buf + n];
        if (id == UA_HEALTH_STAND) {
            setXY2(a, sx - 1, sz - 1, sx + 1, sz + 1);
            setXY2(b, sx - 1, sz + 1, sx + 1, sz - 1);
        } else {
            setXY2(a, sx, sz, sx + 1, sz);
            setXY2(b, sx, sz + 1, sx + 1, sz + 1);
        }
        switch (id) {
        case UA_ICE_CREAM_TRUCK:
            setRGB0(a, 0xFF, 0x90, 0xBA);
            setRGB0(b, 0xFF, 0x90, 0xBA);
            break;
        case UA_TAXI_CAB:
            setRGB0(a, 0xFF, 0xFF, 0);
            setRGB0(b, 0xFF, 0xFF, 0);
            break;
        case UA_MONSTER_TRUCK:
            setRGB0(a, 0, 0xE9, 0);
            setRGB0(b, 0, 0xE9, 0);
            break;
        case UA_POLICE_CAR:
            setRGB0(a, 0, 0, 0xFF);
            setRGB0(b, 0, 0, 0xFF);
            break;
        case UA_LAMBORGHINI:
            setRGB0(a, 0xFF, 0, 0);
            setRGB0(b, 0xFF, 0, 0);
            break;
        case UA_HUMVEE:
            setRGB0(a, 0x82, 0xCA, 0x36);
            setRGB0(b, 0x82, 0xCA, 0x36);
            break;
        case UA_HARLEY:
            setRGB0(a, 0xFF, 0x66, 0);
            setRGB0(b, 0xFF, 0x66, 0);
            break;
        case UA_DUNE_BUGGY:
            setRGB0(a, 0xC7, 0x71, 0);
            setRGB0(b, 0xC7, 0x71, 0);
            break;
        case UA_LOW_RIDER:
            setRGB0(a, 0xFF, 0, 0xFF);
            setRGB0(b, 0xFF, 0, 0xFF);
            break;
        case UA_CORVETTE:
            setRGB0(a, 0xFF, 0xFF, 0xFF);
            setRGB0(b, 0xFF, 0xFF, 0xFF);
            break;
        case UA_MAD_MAX:
            setRGB0(a, 0x9B, 0x9B, 0x32);
            setRGB0(b, 0x9B, 0x9B, 0x32);
            break;
        case UA_SEMI_TRUCK:
        case UA_BOSS_CAR:
            setRGB0(a, 0, 0, 0);
            setRGB0(b, 0, 0, 0);
            break;
        case UA_HEALTH_STAND:
            setRGB0(a, 0xFF, 0xC8, 0xFF);
            setRGB0(b, 0xFF, 0xC8, 0xFF);
            break;
        default:
            setRGB0(a, 0xFF, 0xF0, 0x78);
            setRGB0(b, 0xFF, 0xF0, 0x78);
            break;
        }
        radar.sigTotal += 2;
    }
}

void hudradarActivate(void)
{
    radarOn = 1;
}

void hudRadarToggle(void)
{
    radarOn = 1 - radarOn;
}

void hudStoreArrowIcon(s32 id, DbNode* node)
{
    switch (id) {
    case UA_ARROW_UP:
        grutilsParse2DSprite(node, &ArrowUpInfo, 1);
        break;
    case UA_ARROW_DOWN:
        grutilsParse2DSprite(node, &ArrowDownInfo, 1);
        break;
    case UA_ARROW_LEFT:
        grutilsParse2DSprite(node, &ArrowLeftInfo, 1);
        break;
    case UA_ARROW_RIGHT:
        grutilsParse2DSprite(node, &ArrowRightInfo, 1);
        break;
    }
}
