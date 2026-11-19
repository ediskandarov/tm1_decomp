#include "common.h"

#include <ABS.H>
#include <LIBGPU.H>
#include <LIBGS.H>
#include <LIBGTE.H>
#include <MEMORY.H>
#include <RAND.H>
#include <STDIO.H>

#include "tm1/AI_CAR.H"
#include "tm1/CAR.H"
#include "tm1/FILEIO.H"
#include "tm1/FONT.H"
#include "tm1/GRUTILS.H"
#include "tm1/RT.H"
#include "tm1/SHELL.H"
#include "tm1/SMOOTH.H"
#include "tm1/UA.H"
#include "tm1/UA_DASH.H"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define BOTTOM_BLIT_BUFFER ((u_long*)0x800EEEB8)

extern void exit(s32 code);

s32 gDashboardOn = 0;
s32 D_8018C2AC = 0;

static s32 gDisplayDashboard;
static s32 gBulletHoles;
static s32 gListRemainingCars;
static s32 gMaxCarryCapacity;
static s16 whichOT;
static u32* ot;

extern Dashboard gDash[2];
extern POLY_F4 gSeperator;
extern DR_MODE gMaxCarryDrawMode;
extern SPRT gMaxCarrySprites[23];
extern u32 OT[2][2];
extern POLY_FT4 sw;
extern POLY_FT4 hb1;
extern POLY_FT4 hb2;
extern SPRT gListSprites[8][30];
extern DR_MODE gListDrawMode[8];
extern u_long gTopBlitBuffer[];

void UAdashDrawDashboard(DVECTOR* pos)
{
    if (gDashboardOn) {
        gDash[0].dst[0].x = gDash[0].src[0].x + pos->vx;
        gDash[0].dst[0].y = gDash[0].src[0].y + pos->vy;
        LoadImage(&gDash[0].dst[0], gDash[0].data[0]);
        gDash[0].dst[1].x = gDash[0].src[1].x + pos->vx;
        gDash[0].dst[1].y = gDash[0].src[1].y + pos->vy;
        LoadImage(&gDash[0].dst[1], gDash[0].data[1]);
    }
}

void UAdashHealthColor(s32 tier, POLY_G4* p)
{
    u8 r0, g0, b0, r1, g1, b1;

    switch (tier) {
    case 0:
        r0 = 0xFF;
        g0 = 0x00;
        b0 = 0x00;
        r1 = 0xFF;
        g1 = 0x00;
        b1 = 0x00;
        break;
    case 1:
        r0 = 0xFF;
        g0 = 0x2A;
        b0 = 0x00;
        r1 = 0xFF;
        g1 = 0x73;
        b1 = 0x00;
        break;
    case 2:
        r0 = 0xBA;
        g0 = 0x9B;
        b0 = 0x20;
        r1 = 0xDD;
        g1 = 0xE7;
        b1 = 0x32;
        break;
    default:
        r0 = 0x00;
        g0 = 0x70;
        b0 = 0x0A;
        r1 = 0x31;
        g1 = 0x94;
        b1 = 0x29;
        break;
    }
    p->r0 = r0;
    p->g0 = g0;
    p->b0 = b0;
    p->r2 = r0;
    p->g2 = g0;
    p->b2 = b0;
    p->r1 = r1;
    p->g1 = g1;
    p->b1 = b1;
    p->r3 = r1;
    p->g3 = g1;
    p->b3 = b1;
}

s32 UAdashTierFromPercentageOfHealth(s32 pct, s32 setHoles)
{
    if (pct < 10) {
        if (setHoles) {
            gBulletHoles = 6;
        }
        return 0;
    }
    if (pct < 30) {
        if (setHoles) {
            gBulletHoles = 3;
        }
        return 1;
    }
    if (pct < 50) {
        if (setHoles) {
            gBulletHoles = 2;
        }
        return 2;
    }
    if (setHoles) {
        gBulletHoles = 0;
    }
    return 3;
}

void uadashMaxCarryCapacity(void)
{
    gMaxCarryCapacity = 30;
}

void uadashClearCarryCapacity(void)
{
    gMaxCarryCapacity = 0;
}

char* gCockpitTmsNames[12] = { "ICE_DASH.TMS", "CAB_DASH.TMS", "SEMI_DAS.TMS", "COP_DASH.TMS",
    "LOWRIDER.TMS", "LAMBO_DA.TMS", "DUNEB_DA.TMS", "HUMVEE_D.TMS", "HARLEY_D.TMS", "VETTE_DA.TMS",
    "MONSTERT.TMS", "MADMAX_D.TMS" };

void UAdashDrawInstruments(s32 player, Db* cdb)
{
    static DR_MODE drawMode;
    static SPRT sprites[30];
    PlayerCar* pi;
    POLY_G4* g;
    u8 isPlayer;
    s32 i;
    s32 n;
    s32 ammo;
    s16 rounds;
    s32 speed;
    s32 name;
    s32 hp;
    s32 hpMax;
    s32 tier;
    Car* car;
    u32 wep;
    s32 wv;
    s16 sel;
    POLY_G4* g2;

    whichOT = whichOT ^ 1;
    ot = OT[whichOT];
    ClearOTagR((u_long*)ot, 2);
    pi = GetPlayerInfo(player);
    sel = 0;
    if (gDisplayDashboard != 0 && pi->uaIndex != UA_HARLEY) {
        UAdashDrawDashboard((DVECTOR*)cdb);
        uadashDrawSteeringWheel(ot);
    }
    if (rtIsSplitScreenOn() != 0) {
        sel = 2;
    }
    for (i = sel; i < gDash[player].lifeCnt; i++) {
        AddPrim(ot, &gDash[player].lifeSpr[i]);
    }
    n = pi->weap.ammo[WEAPON_SPECIAL] / (s8)pi->weap.unk48;
    if (n >= 6) {
        n = 5;
    }
    for (i = 0; i < n; i++) {
        AddPrim(ot, &gDash[player].specSpr[i]);
    }
    AddPrim(ot, &gDash[player].lifeMode);
    if (pi->weap.gunOverheat != 0) {
        AddPrim(ot, &gDash[player].f2);
    }
    hp = pi->stats.unk40;
    hpMax = pi->stats.unk44;
    hp = (hp * 100) / hpMax;
    tier = UAdashTierFromPercentageOfHealth(hp, 1);
    hp = (hp * 0x58) / 100;
    if (hp > 0) {
        if (hp < 0x59) {
            wv = hp;
        } else {
            wv = 0x58;
        }
    } else {
        wv = 1;
    }
    hp = wv;
    if (hp != 0) {

        gDash[player].g0.x1 = gDash[player].g0.x0 + hp,
        gDash[player].g0.x3 = gDash[player].g0.x2 + hp;
        g = &gDash[player].g0;
        UAdashHealthColor(tier, g);
        AddPrim(ot, g);
    }
    if (gMaxCarryCapacity != 0 && rtIsSplitScreenOn() == 0) {
        fontSpritePrintCenteredXY(2, 0x9F, 0x9B, &gMaxCarryDrawMode, gMaxCarrySprites,
            "Max Carrying Capacity", rtGetCdb()->ot);
        gMaxCarryCapacity--;
    }
    if (rtIsSplitScreenOn() == 0) {
        speed = GetPlayerSpeed(player);
        if (speed < 0) {
            speed = -speed;
        }
        uadashDrawSpeed(
            speed, 0x69, gDash[player].needle, gDash[player].dig, &gDash[player].speedMode, ot);
    }
    wep = pi->weap.cur;
    if (D_8018C2AC != 0 && wep < 12) {
        ammo = pi->weap.ammo[wep];
        if (wep == 11) {
            ammo = ammo / (s8)pi->weap.unk48;
        }
        if (ammo > 0) {
            AddPrim(ot, &gDash[player].wepSpr[wep]);
            AddPrim(ot, &gDash[player].wepMode[wep]);
            grutilsDrawNumberUsingSprites(ammo, gDash[player].ammoDig, gDash[player].dig, ot);
            AddPrim(ot, &gDash[player].selMode);
        }
    } else {
        AddPrim(ot, &gDash[player].selMode);
    }
    rounds = (s16)pi->weap.unk46 * 10 + pi->stats.unkF8 / 18;
    if (rounds >= 100) {
        pi->weap.unk46 = 10;
        pi->stats.unkF8 = 0;
        rounds = 99;
    }
    grutilsDrawNumberUsingSprites(rounds, gDash[player].machDig, gDash[player].dig2, ot);
    AddPrim(ot, &gDash[player].machMode);
    i = shellLivesRemaining() - 1;
    for (; i != -1; i--) {
        AddPrim(ot, &gDash[player].lives[i]);
    }
    AddPrim(ot, &gDash[player].livesMode);
    if (rtIsSplitScreenOn() != 0) {
        AddPrim(ot, &gDash[player].f0);
        if (player == 0) {
            AddPrim(ot, &gSeperator);
        }
        SetDrawMode(&gDash[player].statMode, 0, 0, 0, 0);
        AddPrim(ot, &gDash[player].statMode);
    } else {
        uadashListRemainingCars(ot);
        if (rtIsRearViewOn() != 0 && gDisplayDashboard == 0) {
            UAdashDrawRearViewMirror(ot);
        }
        car = GetClosestCarToPlayer(player, &isPlayer);
        if (car != NULL) {
            if (isPlayer != 0) {
                name = PLAYER_CAR(car)->uaIndex;
                hp = PLAYER_CAR(car)->stats.unk40;
                hpMax = PLAYER_CAR(car)->stats.unk44;
                tier = UAdashTierFromPercentageOfHealth((hp * 100) / hpMax, 0);
            } else {
                name = AI_CAR(car)->uaIndex;
                hp = AI_CAR(car)->stats.unk40;
                hpMax = AI_CAR(car)->stats.unk44;
                tier = AI_CAR(car)->unk02;
            }
            if (hp > 0) {
                fontSetColor(2, 0x24, 0x96, 0x24);
                fontSpritePrintCenteredXY(
                    2, 0x92, 0x21, &drawMode, sprites, uadashGetCarSign(name), ot);
                fontSetDefaultColor(2);
            }
            hp = (hp * 0x56) / hpMax;
            if (hp > 0) {
                if (hp < 0x57) {
                    wv = hp;
                } else {
                    wv = 0x56;
                }
            } else {
                wv = 1;
            }
            hp = wv;
            if (hp != 0) {

                gDash[player].g1.x1 = gDash[player].g1.x0 + hp,
                gDash[player].g1.x3 = gDash[player].g1.x2 + hp;
                g2 = &gDash[player].g1;
                UAdashHealthColor(tier, g2);
                AddPrim(ot, g2);
            }
        }
        rounds = GetNumAICarsLiving();
        grutilsDrawNumberUsingSprites(rounds, &gDash[player].killSpr, gDash[player].dig, ot);
        grutilsDrawNumberUsingSprites(
            GetNumAICars() - rounds, &gDash[player].totSpr, gDash[player].dig, ot);
        AddPrim(ot, &gDash[player].numMode);
        if (gDisplayDashboard != 0) {
            if (pi->uaIndex != UA_HARLEY) {
                for (i = 0; i < gDash[player].count; i++) {
                    AddPrim(ot, &gDash[player].elems[i]);
                    AddPrim(ot, &gDash[player].dmode[i]);
                }
                i = gBulletHoles;
                for (i--; i != -1; i--) {
                    AddPrim(ot, &gDash[player].bullets[i]);
                }
                AddPrim(ot, &gDash[player].bulMode);
            } else {
                for (i = 0; i < 3; i++) {
                    AddPrim(ot, &gDash[player].harley[i]);
                    AddPrim(ot, &gDash[player].hmode[i]);
                }
                uadashDrawHarleyHandleBars(ot);
                for (i = 0; i < 2; i++) {
                    AddPrim(ot, &(&gDash[player].f0)[i]);
                }
            }
        } else {
            for (i = 0; i < 2; i++) {
                AddPrim(ot, &(&gDash[player].f0)[i]);
            }
        }
        SetDrawMode(&gDash[player].statMode, 0, 0, 0, 0);
        AddPrim(ot, &gDash[player].statMode);
    }
    DrawOTag((u_long*)(ot + 1));
}

static inline void uadashInitSpriteFlags(SPRT* sprites, s32 i)
{
    SetShadeTex(&sprites[i], 0);
    SetSemiTrans(&sprites[i], 0);
}

static inline void uadashPositionLifeSprites(s16 which, s16 bias)
{
    setXY0(&gDash[which].lifeSpr[0], 0x66, bias + 40);
    setXY0(&gDash[which].lifeSpr[1], 0x97, bias + 37);
    setRGB0(&gDash[which].lifeSpr[2], 0x24, 0x78, 0x24);
    setXY0(&gDash[which].lifeSpr[2], 0xD8, bias + 202);
    setRGB0(&gDash[which].lifeSpr[3], 0x24, 0x78, 0x24);
    setXY0(&gDash[which].lifeSpr[3], 0xD8, bias + 215);
}

static inline void uadashInitKillSprites(s16 which, s16 bias)
{
    SPRT* kill;
    SPRT* total;
    kill = &gDash[which].killSpr;
    setSprt(kill);
    gDash[which].killSpr.clut = gDash[which].dig[0].clut;
    setRGB0(kill, 0x24, 0x78, 0x24);
    SetShadeTex(kill, 0);
    setXY0(kill, 0x86, bias + 39);
    total = &gDash[which].totSpr;
    setSprt(total);
    gDash[which].totSpr.clut = gDash[which].dig[0].clut;
    setRGB0(total, 0x78, 0x24, 0x24);
    SetShadeTex(total, 0);
    setXY0(total, 0xB3, bias + 39);
}

void UAdashInitDashboardIcons(DbNode* node)
{
    DbNode* group;
    GrSprite buf[12];
    s16 bias;
    s16 which;
    GrSprite* g;
    SPRT* s;
    s32 i;

    bias = 0;
    which = 1;
    group = node;
    if (node == NULL) {
        return;
    }
    if ((group->grp.h.op & 0xFF) != DB_OP_GROUP || group->grp.nChild < 5) {
        return;
    }
    if (rtIsSplitScreenOn() != 0) {
        bias = -120;
        which = 2;
    }
    for (which = which - 1; which != -1; which = which - 1) {
        gDash[which].wepCnt = grutilsParse2DSprite(group->grp.child[0], buf, 12);
        for (i = 0; i < gDash[which].wepCnt; i++) {
            s = &gDash[which].wepSpr[i];
            g = &buf[i];
            setSprt(s);
            s->clut = g->clut;
            setUV0(s, g->u0, g->v0);
            setWH(s, g->w, g->h);
            setRGB0(s, 0x80, 0x80, 0x80);
            SetShadeTex(s, 0);
            setXY0(s, 0xFF, bias + 210);
            SetSemiTrans(s, 1);
            SetDrawMode(&gDash[which].wepMode[i], 0, 0, g->tpage, 0);
        }

        grutilsParse2DSprite(group->grp.child[6], buf, 1);
        for (i = 0; i < 3; i++) {
            s = &gDash[which].lives[i];
            setSprt(s);
            s->clut = buf->clut;
            setUV0(s, buf->u0, buf->v0);
            setWH(s, buf->w, buf->h);
            setRGB0(s, 0x80, 0x80, 0x80);
            SetShadeTex(s, 0);
            setXY0(s, 0x135, i * 5 + (s16)(bias + 189));
            SetSemiTrans(s, 1);
            SetDrawMode(&gDash[which].livesMode, 0, 0, buf->tpage, 0);
        }

        grutilsParse2DSprite(group->grp.child[7], buf, 1);
        for (i = 0; i < 6; i++) {
            s = &gDash[which].bullets[i];
            setSprt(s);
            s->clut = buf->clut;
            setUV0(s, buf->u0, buf->v0);
            setWH(s, buf->w, buf->h);
            setRGB0(s, 0x80, 0x80, 0x80);
            SetShadeTex(s, 0);
            setXY0(s, 0, 0);
            SetSemiTrans(s, 1);
            SetDrawMode(&gDash[which].bulMode, 0, 0, buf->tpage, 0);
        }

        grutilsParse2DSprite(group->grp.child[1], buf, 1);
        SetDrawMode(&gDash[which].numMode, 0, 0, buf->tpage, 0);

        gDash[which].lifeCnt = grutilsParse2DSprite(group->grp.child[2], buf, 5) - 1;
        for (i = 0; i < gDash[which].lifeCnt; i++) {
            s = &gDash[which].lifeSpr[i];
            g = &buf[i];
            setSprt(s);
            s->clut = g->clut;
            setUV0(s, g->u0, g->v0);
            setWH(s, g->w, g->h);
            setRGB0(s, 0x80, 0x80, 0x80);
            SetShadeTex(s, 0);
            SetSemiTrans(s, 0);
        }
        SetDrawMode(&gDash[which].lifeMode, 0, 0, buf->tpage, 0);
        uadashPositionLifeSprites(which, bias);

        g = &buf[gDash[which].lifeCnt];
        for (i = 0; i < 5; i++) {
            s = &gDash[which].specSpr[i];
            setSprt(s);
            s->clut = g->clut;
            setUV0(s, g->u0, g->v0);
            setWH(s, g->w, g->h);
            setRGB0(s, 0x80, 0x80, 0x80);
            setXY0(&gDash[which].specSpr[i], 0x106 + i * 9, bias + 202);
            SetShadeTex(s, 0);
            SetSemiTrans(s, 0);
        }

        grutilsParse2DSprite(group->grp.child[3], gDash[which].dig, 10);
        SetDrawMode(&gDash[which].selMode, 0, 0, gDash[which].dig[0].tpage, 0);
        SetDrawMode(&gDash[which].speedMode, 0, 0, gDash[which].dig[0].tpage, 0);
        for (i = 0; i < 3; i++) {
            setSprt(&gDash[which].ammoDig[i]);
            gDash[which].ammoDig[i].clut = gDash[which].dig[0].clut;
            setRGB0(&gDash[which].ammoDig[i], 0x24, 0x78, 0x24);
            uadashInitSpriteFlags(gDash[which].ammoDig, i);
            setXY0(&gDash[which].ammoDig[i], 0x122, bias + 212);
            setSprt(&gDash[which].needle[i]);
            gDash[which].needle[i].clut = gDash[which].dig[0].clut;
            setRGB0(&gDash[which].needle[i], 0xFF, 0x0F, 0x24);
            uadashInitSpriteFlags(gDash[which].needle, i);
            setXY0(&gDash[which].needle[i], 0x73, bias + 202);
        }

        uadashInitKillSprites(which, bias);

        grutilsParse2DSprite(group->grp.child[4], gDash[which].dig2, 10);
        SetDrawMode(&gDash[which].machMode, 0, 0, gDash[which].dig2[0].tpage, 0);
        for (i = 0; i < 2; i++) {
            setSprt(&gDash[which].machDig[i]);
            gDash[which].machDig[i].clut = gDash[which].dig2[0].clut;
            setRGB0(&gDash[which].machDig[i], 0x24, 0x78, 0x24);
            s = &gDash[which].machDig[i];
            SetShadeTex(s, 0);
            SetSemiTrans(s, 0);
            setXY0(&gDash[which].machDig[i], 0xF3, bias + 202);
        }
    }
    D_8018C2AC = 1;
}

void UAdashInitDashboard(u_long* tim)
{
    s32 y;

    y = 0;
    if (rtIsSplitScreenOn()) {
        SetPolyF4(&gSeperator);
        setRGB0(&gSeperator, 0, 0, 0);
        setSemiTrans(&gSeperator, 0);
        y = -120;
        setXY4(&gSeperator, 0, 0x74, 0x140, 0x74, 0, 0x7A, 0x140, 0x7A);
    }

    setPolyG4(&gDash[0].g0);
    SetSemiTrans(&gDash[0].g0, 1);
    setXY4(&gDash[0].g0, 0xD9, y + 191, 0x131, y + 191, 0xD9, y + 199, 0x131, y + 199);

    setPolyF4(&gDash[0].f0);
    SetSemiTrans(&gDash[0].f0, 1);
    setXY4(&gDash[0].f0, 0xD8, y + 190, 0x132, y + 190, 0xD8, y + 200, 0x132, y + 200);
    setRGB0(&gDash[0].f0, 0xA, 0xA, 0xA);

    setPolyF4(&gDash[0].f2);
    SetSemiTrans(&gDash[0].f2, 1);
    setXY4(&gDash[0].f2, 0xD8, y + 214, 0xFE, y + 214, 0xD8, y + 224, 0xFE, y + 224);
    setRGB0(&gDash[0].f2, 0xFF, 0, 0);

    if (rtIsSplitScreenOn()) {
        setPolyG4(&gDash[1].g0);
        SetSemiTrans(&gDash[1].g0, 1);
        setXY4(&gDash[1].g0, 0xD9, y + 191, 0x131, y + 191, 0xD9, y + 199, 0x131, y + 199);

        setPolyF4(&gDash[1].f0);
        SetSemiTrans(&gDash[1].f0, 1);
        setXY4(&gDash[1].f0, 0xD8, y + 190, 0x132, y + 190, 0xD8, y + 200, 0x132, y + 200);
        setRGB0(&gDash[1].f0, 0xA, 0xA, 0xA);

        setPolyF4(&gDash[1].f2);
        SetSemiTrans(&gDash[1].f2, 1);
        setXY4(&gDash[1].f2, 0xD8, y + 214, 0xFE, y + 214, 0xD8, y + 224, 0xFE, y + 224);
        setRGB0(&gDash[1].f2, 0xFF, 0, 0);
    } else {
        uadashInitCockpit(tim);
        uadashInitRearViewMirror();

        setPolyG4(&gDash[0].g1);
        SetSemiTrans(&gDash[0].g1, 1);
        setXY4(&gDash[0].g1, 0x67, y + 0x16, 0xBD, y + 0x16, 0x67, y + 0x23, 0xBD, y + 0x23);

        setPolyF4(&gDash[0].f1);
        SetSemiTrans(&gDash[0].f1, 1);
        setXY4(&gDash[0].f1, 0x66, y + 0x15, 0xBE, y + 0x15, 0x66, y + 0x24, 0xBE, y + 0x24);
        setRGB0(&gDash[0].f1, 0xA, 0xA, 0xA);
    }
    gDashboardOn = 1;
}

u_long* uadashGetTopBlitAddr(void)
{
    return gTopBlitBuffer;
}

void uadashInitCockpit(u_long* tim)
{
    GrScreenSprite buf[7];
    u_long* pixels[2] = { gTopBlitBuffer, BOTTOM_BLIT_BUFFER };
    GrScreenSprite* s;
    DashElem* e;
    s32 n;
    s32 cnt;
    s32 i;

    n = grutilsParse2DScreenSprite((DbNode*)tim, buf, 7);
    for (i = 0; i < 2; i++) {
        gDash[0].data[i] = pixels[i];
        s = &buf[i * (n - 1)];
        gDash[0].src[i].x = s->x;
        gDash[0].src[i].y = s->y;
        gDash[0].src[i].w = s->w;
        gDash[0].src[i].h = s->h;
        memcpy(&gDash[0].dst[i], &gDash[0].src[i], 8);
        gDash[0].data[i] = pixels[i];
    }
    cnt = n - 2;
    if (cnt < 0) {
        cnt = 0;
    }
    gDash[0].count = cnt;
    for (i = 0; i < gDash[0].count; i++) {
        e = &gDash[0].elems[i];
        SetSprt(&e->sprt);
        s = &buf[i + 1];
        e->sprt.r0 = 0x80;
        e->sprt.g0 = 0x80;
        e->sprt.b0 = 0x80;
        e->sprt.x0 = s->x;
        e->sprt.y0 = s->y;
        e->sprt.w = s->w;
        e->sprt.h = s->h;
        e->sprt.u0 = s->u0;
        e->sprt.v0 = s->v0;
        e->sprt.clut = s->clut;
        e->tpage = s->tpage;
        SetDrawMode(&gDash[0].dmode[i], 0, 0, s->tpage, 0);
    }
    uadashInitHarley();
}

void uadashReloadDashboard(u_long* tim, s32 idx)
{
    RECT r;
    GsIMAGE im;
    s32 tbuf[2];
    s32* t;
    DashElem* e;

    t = tbuf;
    e = &gDash[0].elems[idx];
    t[0] = (e->tpage << 4) & 0x100;
    t[1] = (e->tpage << 6) & 0x3FF;
    GsGetTimInfo(tim + 1, &im);
    r.x = (e->sprt.u0 >> 1) + tbuf[1];
    r.y = tbuf[0] + e->sprt.v0;
    r.w = im.pw;
    r.h = im.ph;
    LoadImage(&r, im.pixel);
    if ((im.pmode >> 3) & 1) {
        t[1] = (e->sprt.clut & 0x3F) * 16;
        t[0] = e->sprt.clut >> 6;
        r.x = tbuf[1];
        r.y = tbuf[0];
        r.w = im.cw;
        r.h = im.ch;
        LoadImage(&r, im.clut);
    }
}

void uadashReloadHarleyDashboard(u_long* tim, s32 idx)
{
    RECT r;
    GsIMAGE im;
    s32 tbuf[2];
    s32* t;
    DashElem* e;

    t = tbuf;
    e = &gDash[0].harley[idx];
    t[0] = (e->tpage << 4) & 0x100;
    t[1] = (e->tpage << 6) & 0x3FF;
    GsGetTimInfo(tim + 1, &im);
    r.x = (e->sprt.u0 >> 1) + tbuf[1];
    r.y = tbuf[0] + e->sprt.v0;
    r.w = im.pw;
    r.h = im.ph;
    LoadImage(&r, im.pixel);
    if ((im.pmode >> 3) & 1) {
        t[1] = (e->sprt.clut & 0x3F) * 16;
        t[0] = e->sprt.clut >> 6;
        r.x = tbuf[1];
        r.y = tbuf[0];
        r.w = im.cw;
        r.h = im.ch;
        LoadImage(&r, im.clut);
    }
}

void uadashReloadSteeringWheel(u_long* tim)
{
    RECT r;
    GsIMAGE im;
    s32 tbuf[2];
    s32* t;
    POLY_FT4* p;
    s32 x;
    s32 y;

    p = &gDash[0].wheel;
    t = tbuf;
    t[0] = (p->tpage << 4) & 0x100;
    t[1] = (p->tpage << 6) & 0x3FF;
    GsGetTimInfo(tim + 1, &im);
    x = MIN(p->u0, p->u1);
    x = MIN(x, p->u2);
    x = MIN(x, p->u3);
    r.x = x + tbuf[1];
    y = MIN(p->v0, p->v1);
    y = MIN(y, p->v2);
    y = MIN(y, p->v3);
    r.y = y + tbuf[0];
    r.w = im.pw;
    r.h = im.ph;
    LoadImage(&r, im.pixel);
    t = tbuf;
    if ((im.pmode >> 3) & 1) {
        t[1] = (p->clut & 0x3F) * 16;
        t[0] = p->clut >> 6;
        r.x = tbuf[1];
        r.y = tbuf[0];
        r.w = im.cw;
        r.h = im.ch;
        LoadImage(&r, im.clut);
    }
}

void uadashLoadHarleyTms(u_long* tims)
{
    s32 count;
    s32 i;
    u32 len;
    s32 idx;

    idx = 0;
    count = tims[0];
    tims++;
    len = *tims;
    for (i = 0; i < count; i++) {
        tims++;
        if (i == 3) {
            uadashReloadSteeringWheel(tims);
        } else {
            uadashReloadHarleyDashboard(tims, idx++);
        }
        tims += len >> 2;
        len = *tims;
    }
}

void uadashLoadCockpit(s32 car)
{
    char name[24];
    GsIMAGE im0;
    GsIMAGE im1;
    s32 num;
    u_long* p;
    s32 count;
    u32 len;
    s32 i;
    s32 idx;
    u32 skipped;

    num = GetCarNumFromCarName(car);
    p = (u_long*)rtGetDoubleBufferArea();
    idx = 0;
    if (car < 0) {
        return;
    }
    sprintf(name, "UADASH\\%s", gCockpitTmsNames[num]);
    fileioLoadFileIntoRam(p, name);
    if (*p++ != 0x50535854) {
        p--;
        printf("Error: %s - Invalid TMS file.", name);
        exit(-1);
    }
    if (*p++ != 0x43) {
        p--;
        printf("Error: %s - Old version.\n", name);
        exit(-1);
    }
    skipped = *p;
    p++;
    if (car == 0x50) {
        uadashLoadHarleyTms(p);
        return;
    }
    count = p[0];
    p++;
    len = *p;
    for (i = 0; i < count; i++) {
        p++;
        switch (i) {
        case 0:
            GsGetTimInfo(p + 1, &im0);
            memcpy(gDash[0].data[0], im0.pixel, im0.pw * (im0.ph * 2));
            break;
        case 6:
            GsGetTimInfo(p + 1, &im1);
            memcpy(gDash[0].data[1], im1.pixel, im1.pw * (im1.ph * 2));
            break;
        case 7:
            uadashReloadSteeringWheel(p);
            break;
        default:
            uadashReloadDashboard(p, idx++);
            break;
        }
        p += len >> 2;
        len = *p;
    }
}

void UAdashSetDashboardDrawFlag(s32 flag)
{
    gDisplayDashboard = flag;
}

void uadashInitSteeringWheel(DbNode* node)
{
    SVECTOR* vt;
    DbPoly* f;
    u8* base;
    DbPolyUV* uv;
    POLY_FT4* wheel = &gDash[0].wheel;

    if (node != NULL && node->h.op == DB_OP_OBJECT) {
        vt = node->obj.verts;
        f = node->obj.polys;
        base = (u8*)(f + 1);
        uv = (DbPolyUV*)(base + f->uvOffset * 4);
        setPolyFT4(wheel);
        SetSemiTrans(wheel, 1);
        setRGB0(wheel, 0xFF, 0xFF, 0xFF);
        wheel->x0 = vt[f->vi[0]].vx / 8 - 120;
        wheel->y0 = -(vt[f->vi[0]].vy / 8) - 64;
        wheel->x1 = vt[f->vi[1]].vx / 8 - 120;
        wheel->y1 = -(vt[f->vi[1]].vy / 8) - 64;
        wheel->x2 = vt[f->vi[2]].vx / 8 - 120;
        wheel->y2 = vt[f->vi[2]].vy / 8 - 64;
        wheel->x3 = vt[f->vi[3]].vx / 8 - 120;
        wheel->y3 = vt[f->vi[3]].vy / 8 - 64;
        wheel->u0 = uv->u0;
        wheel->v0 = uv->v0;
        wheel->u1 = uv->u1;
        wheel->v1 = uv->v1;
        wheel->u2 = uv->u2;
        wheel->v2 = uv->v2;
        wheel->u3 = uv->u3;
        wheel->v3 = uv->v3;
        wheel->clut = uv->clut;
        wheel->tpage = uv->tpage;
        setPolyFT4(&sw);
        SetSemiTrans(&sw, 1);
        setRGB0(&sw, 0xFF, 0xFF, 0xFF);
        uadashInitHarleyHandleBars();
    }
}

void uadashInitHarleyHandleBars(void)
{
    POLY_FT4* handle = &gDash[0].hb[0];

    setPolyFT4(handle);
    SetSemiTrans(handle, 1);
    setRGB0(handle, 0xFF, 0xFF, 0xFF);
    setXY4(handle, -0x0E, 0x7E, -0x72, 0x7E, -0x0E, -0x82, -0x72, -0x82);
    /* Scalar pointer reads preserve the original -fvolatile accesses. */
    setUV4(handle, *(&gDash[0].wheel.u1), *(&gDash[0].wheel.v1), *(&gDash[0].wheel.u3),
        *(&gDash[0].wheel.v3), *(&gDash[0].wheel.u0), *(&gDash[0].wheel.v0), *(&gDash[0].wheel.u2),
        *(&gDash[0].wheel.v2));
    handle->clut = *(&gDash[0].wheel.clut);
    handle->tpage = *(&gDash[0].wheel.tpage);
    setPolyFT4(&hb1);
    SetSemiTrans(&hb1, 1);
    setRGB0(&hb1, 0xFF, 0xFF, 0xFF);
    handle++;
    setPolyFT4(handle);
    SetSemiTrans(handle, 1);
    setRGB0(handle, 0xFF, 0xFF, 0xFF);
    setXY4(handle, 0x72, 0x7E, 0x0E, 0x7E, 0x72, -0x82, 0x0E, -0x82);
    setUV4(handle, gDash[0].wheel.u3, gDash[0].wheel.v3, gDash[0].wheel.u1, gDash[0].wheel.v1,
        gDash[0].wheel.u2, gDash[0].wheel.v2, gDash[0].wheel.u0, gDash[0].wheel.v0);
    handle->clut = gDash[0].wheel.clut;
    handle->tpage = gDash[0].wheel.tpage;
    setPolyFT4(&hb2);
    SetSemiTrans(&hb2, 1);
    setRGB0(&hb2, 0xFF, 0xFF, 0xFF);
}

void uadashDrawSteeringWheel(u32* ot)
{
    static s32 delta;
    MATRIX mtx;
    SVECTOR rot;
    SVECTOR in;
    VECTOR out;
    long flag;
    POLY_FT4* p;
    PlayerCar* pi;
    s32 rate;
    s32 i;
    s32 j;

    rate = 0x46;
    pi = GetPlayerInfo(0);
    p = &gDash[0].wheel;
    if (pi->skid[7] || pi->skid[6]) {
        rate = 0x28;
    }
    if (pi->skid[2]) {
        delta = SmoothValue(delta, -28, rate);
    } else if (pi->skid[3]) {
        delta = SmoothValue(delta, 28, rate);
    } else {
        delta = SmoothValue(delta, 0, rate);
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            mtx.m[i][j] = (i == j) << 12;
        }
        mtx.t[i] = 0;
    }
    mtx.t[2] = 0;
    rot.vx = 0;
    rot.vy = 0;
    mtx.t[0] = 0x7D;
    mtx.t[1] = 0xE6;
    rot.vz = (delta << 12) / 360;
    RotMatrixYXZ(&rot, &mtx);
    SetTransMatrix(&mtx);
    SetRotMatrix(&mtx);
    in.vx = p->x0;
    in.vy = p->y0;
    RotTrans(&in, &out, &flag);
    sw.x0 = out.vx;
    sw.y0 = out.vy;
    in.vx = p->x1;
    in.vy = p->y1;
    RotTrans(&in, &out, &flag);
    sw.x1 = out.vx;
    sw.y1 = out.vy;
    in.vx = p->x2;
    in.vy = p->y2;
    RotTrans(&in, &out, &flag);
    sw.x2 = out.vx;
    sw.y2 = out.vy;
    in.vx = p->x3;
    in.vy = p->y3;
    RotTrans(&in, &out, &flag);
    sw.x3 = out.vx;
    sw.y3 = out.vy;
    sw.u0 = p->u0;
    sw.u1 = p->u1;
    sw.u2 = p->u2;
    sw.u3 = p->u3;
    sw.v0 = p->v0;
    sw.v1 = p->v1;
    sw.v2 = p->v2;
    sw.v3 = p->v3;
    sw.clut = p->clut;
    sw.tpage = p->tpage;
    if (ot != NULL) {
        AddPrim(ot, &sw);
        return;
    }
    DrawPrim(&sw);
}

void uadashDrawHarleyHandleBars(u32* ot)
{
    static s32 delta;
    MATRIX mtx;
    SVECTOR rot;
    SVECTOR in;
    VECTOR out;
    long flag;
    PlayerCar* pi;
    s32 rate;
    s32 i;
    s32 j;

    rate = 0x46;
    pi = GetPlayerInfo(0);
    if (pi->skid[7] || pi->skid[6]) {
        rate = 0x28;
    }
    if (pi->skid[2]) {
        delta = SmoothValue(delta, -28, rate);
    } else if (pi->skid[3]) {
        delta = SmoothValue(delta, 28, rate);
    } else {
        delta = SmoothValue(delta, 0, rate);
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            mtx.m[i][j] = (i == j) << 12;
        }
        mtx.t[i] = 0;
    }
    mtx.t[2] = 0;
    rot.vx = 0;
    rot.vy = 0;
    mtx.t[0] = 0x9F;
    mtx.t[1] = 0xCA;
    rot.vz = (delta << 12) / 360;
    RotMatrixYXZ(&rot, &mtx);
    SetTransMatrix(&mtx);
    SetRotMatrix(&mtx);
    in.vx = gDash[0].hb[0].x0;
    in.vy = gDash[0].hb[0].y0;
    RotTrans(&in, &out, &flag);
    hb1.x0 = out.vx;
    hb1.y0 = out.vy;
    in.vx = gDash[0].hb[0].x1;
    in.vy = gDash[0].hb[0].y1;
    RotTrans(&in, &out, &flag);
    hb1.x1 = out.vx;
    hb1.y1 = out.vy;
    in.vx = gDash[0].hb[0].x2;
    in.vy = gDash[0].hb[0].y2;
    RotTrans(&in, &out, &flag);
    hb1.x2 = out.vx;
    hb1.y2 = out.vy;
    in.vx = gDash[0].hb[0].x3;
    in.vy = gDash[0].hb[0].y3;
    RotTrans(&in, &out, &flag);
    hb1.x3 = out.vx;
    hb1.y3 = out.vy;
    hb1.u0 = gDash[0].hb[0].u0;
    hb1.u1 = gDash[0].hb[0].u1;
    hb1.u2 = gDash[0].hb[0].u2;
    hb1.u3 = gDash[0].hb[0].u3;
    hb1.v0 = gDash[0].hb[0].v0;
    hb1.v1 = gDash[0].hb[0].v1;
    hb1.v2 = gDash[0].hb[0].v2;
    hb1.v3 = gDash[0].hb[0].v3;
    hb1.clut = gDash[0].hb[0].clut;
    hb1.tpage = gDash[0].hb[0].tpage;
    AddPrim(ot, &hb1);
    in.vx = gDash[0].hb[1].x0;
    in.vy = gDash[0].hb[1].y0;
    RotTrans(&in, &out, &flag);
    hb2.x0 = out.vx;
    hb2.y0 = out.vy;
    in.vx = gDash[0].hb[1].x1;
    in.vy = gDash[0].hb[1].y1;
    RotTrans(&in, &out, &flag);
    hb2.x1 = out.vx;
    hb2.y1 = out.vy;
    in.vx = gDash[0].hb[1].x2;
    in.vy = gDash[0].hb[1].y2;
    RotTrans(&in, &out, &flag);
    hb2.x2 = out.vx;
    hb2.y2 = out.vy;
    in.vx = gDash[0].hb[1].x3;
    in.vy = gDash[0].hb[1].y3;
    RotTrans(&in, &out, &flag);
    hb2.x3 = out.vx;
    hb2.y3 = out.vy;
    hb2.u0 = gDash[0].hb[1].u0;
    hb2.u1 = gDash[0].hb[1].u1;
    hb2.u2 = gDash[0].hb[1].u2;
    hb2.u3 = gDash[0].hb[1].u3;
    hb2.v0 = gDash[0].hb[1].v0;
    hb2.v1 = gDash[0].hb[1].v1;
    hb2.v2 = gDash[0].hb[1].v2;
    hb2.v3 = gDash[0].hb[1].v3;
    hb2.clut = gDash[0].hb[1].clut;
    hb2.tpage = gDash[0].hb[1].tpage;
    AddPrim(ot, &hb2);
}

void uadashDrawSpeed(
    s32 speed, s32 unused1, SPRT* unused2, GrSprite* unused3, DR_MODE* unused4, u32* ot)
{
    static DR_MODE drawMode;
    static SPRT digitSprites[4];
    char str[4] = "   ";
    s32 i;
    s32 q;
    s32 r;

    for (i = 2; i != -1; i--) {
        q = speed / 10;
        r = speed % 10;
        speed = q;
        if (i == 2 || r != 0 || speed != 0) {
            str[i] = r + '0';
        }
    }
    fontSpritePrintXY(4, 0x66, 0xD6, &drawMode, digitSprites, str, ot);
}

void uadashInitHarley(void)
{
    s16 xs[3] = { 117, 85, 85 };
    s16 ys[3] = { 132, 168, 204 };
    s16 ws[3] = { 64, 128, 128 };
    s16 cluts[3]
        = { gDash[0].elems[0].sprt.clut, gDash[0].elems[1].sprt.clut, gDash[0].elems[2].sprt.clut };
    u8 us[3] = { gDash[0].elems[3].sprt.u0, gDash[0].elems[4].sprt.u0, gDash[0].elems[4].sprt.u0 };
    u8 vs[3] = { gDash[0].elems[3].sprt.v0, gDash[0].elems[4].sprt.v0, gDash[0].elems[4].sprt.v0 };
    DashElem* e;
    s32 i;

    for (i = 0; i < 3; i++) {
        e = &gDash[0].harley[i];
        SetSprt(&e->sprt);
        e->sprt.r0 = 0x80;
        e->sprt.g0 = 0x80;
        e->sprt.b0 = 0x80;
        e->sprt.x0 = xs[i];
        e->sprt.y0 = ys[i];
        e->sprt.w = ws[i];
        e->sprt.h = 0x24;
        e->sprt.u0 = us[i];
        e->sprt.v0 = vs[i];
        e->sprt.clut = cluts[i];
    }

    gDash[0].harley[0].tpage = gDash[0].elems[3].tpage;
    SetDrawMode(&gDash[0].hmode[0], 0, 0, gDash[0].harley[0].tpage, 0);
    gDash[0].harley[1].tpage = gDash[0].elems[4].tpage;
    SetDrawMode(&gDash[0].hmode[1], 0, 0, gDash[0].harley[1].tpage, 0);
    gDash[0].harley[2].tpage = gDash[0].elems[4].tpage + 1;
    SetDrawMode(&gDash[0].hmode[2], 0, 0, gDash[0].harley[2].tpage, 0);
}

char* gCarSignStrings[13] = { "SWEET TOOTH", "Y\bELL\b\bO\bW \b\b\b\bJ\bA\bCK\bET", "DARKSIDE",
    "OUTLAW", "THUMPER", "CRIMSON \b\bFURY", "PIT VIPER", "WARTHOG", "MR GRIMM", "SPECTRE",
    "HAMMERHEAD", "ROAD KILL", "MINION" };

char* uadashGetCarSign(s32 car)
{
    return gCarSignStrings[GetCarNumFromCarName(car)];
}

void uadashInitRearViewMirror(void)
{
    POLY_F4* p;

    p = gDash[0].mirrors;
    setPolyF4(p);
    setXY4(p, 0xD1, 0x12, 0x13A, 0x12, 0xD1, 0x14, 0x13A, 0x14);
    setRGB0(p, 0, 0, 0);
    SetSemiTrans(p, 0);
    p++;
    setPolyF4(p);
    setXY4(p, 0xD1, 0x32, 0x13A, 0x32, 0xD1, 0x34, 0x13A, 0x34);
    setRGB0(p, 0, 0, 0);
    SetSemiTrans(p, 0);
    p++;
    setPolyF4(p);
    setXY4(p, 0xD1, 0x14, 0xD3, 0x14, 0xD1, 0x32, 0xD3, 0x32);
    setRGB0(p, 0, 0, 0);
    SetSemiTrans(p, 0);
    p++;
    setPolyF4(p);
    setXY4(p, 0x138, 0x14, 0x13A, 0x14, 0x138, 0x32, 0x13A, 0x32);
    setRGB0(p, 0, 0, 0);
    SetSemiTrans(p, 0);
}

void uadashInitBulletHoles(void)
{
    s32 x[6];
    s32 y[6];
    SPRT* p;
    s32 i;
    s32 j;
    s32 dx;
    s32 dy;

    i = 0;
    for (; i < 6; i++) {
        x[i] = rand() % 295;
        y[i] = rand() % 105;
        if (i > 0) {
            for (j = 0; j < i; j++) {
                dx = x[j] - x[i];
                if (ABS(dx) >= 16) {
                    dy = y[j] - y[i];
                    if (ABS(dy) >= 16) { }
                }
            }
        }
        p = &gDash[0].bullets[i];
        p->x0 = x[i] + 20;
        p->y0 = y[i] + 50;
    }
}

void UAdashDrawRearViewMirror(u32* ot)
{
    AddPrim(ot, &gDash[0].mirrors[0]);
    AddPrim(ot, &gDash[0].mirrors[1]);
    AddPrim(ot, &gDash[0].mirrors[2]);
    AddPrim(ot, &gDash[0].mirrors[3]);
}

void uadashToggleRemainingCarsList(void)
{
    gListRemainingCars ^= 1;
}

void uadashListRemainingCars(u32* ot)
{
    s32 sprOfs;
    s32 y;
    s32 modeOfs;
    s32 i;
    AICar* car;

    if (gListRemainingCars) {
        fontSetColor(2, 0x24, 0x96, 0x24);
        i = 0;
        sprOfs = 0;
        y = 0x46;
        modeOfs = 0;
        while (i < GetNumAICars()) {
            car = GetAICarInfo(i);
            if (car != NULL && car->stats.unk40 > 0) {
                fontSpritePrintXY(2, 0xDC, y, (DR_MODE*)((u8*)gListDrawMode + modeOfs),
                    (SPRT*)((u8*)gListSprites + sprOfs), uadashGetCarSign(car->uaIndex), ot);
            }
            sprOfs += sizeof(gListSprites[0]);
            y += 12;
            modeOfs += sizeof(DR_MODE);
            i++;
        }
        fontSetDefaultColor(2);
    }
}
