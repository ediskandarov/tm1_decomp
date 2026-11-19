#include "common.h"

#include <LIBCD.H>
#include <LIBETC.H>
#include <LIBGPU.H>
#include <LIBPRESS.H>
#include <LIBSND.H>
#include <STDIO.H>

#include "tm1/CTLPAD.H"
#include "tm1/FILEIO.H"
#include "tm1/RT.H"
#include "tm1/SCREEN.H"
#include "tm1/SOUND.H"
#include "tm1/TIMER.H"

#include "tm1/MOVIE.H"

typedef struct StrEnv {
    u_long* buf[2]; /* 0x00 */
    s32 idx; /* 0x08 */
    u_long* img; /* 0x0C */
    RECT rect[2]; /* 0x10 */
    s32 flip; /* 0x20 */
    RECT slice; /* 0x24 */
    s32 done; /* 0x2C */
} StrEnv; /* 0x30 */

s32 lastFrame = 0;
s32 lastW = 0;
s32 lastH = 0;

static s32 SliceSize;
static RECT SliceRect;
static u16 gSectorsPer;
static u_long* mdec_bs;
static u_long* mdec_rl;
static u_long* mdec_image;
static u8 gIsRGB24;
static CdlLOC loc;
static s32 Rewind_Switch;
static u_long* sect_buff;

static DISPENV Disp[2];
static StrEnv dec;

extern s32 StCdIntrFlag;

extern void DecDCToutCallback(void (*func)());

void disp_mdec(u_long* src, s32* pflip);
void strSetDefDecEnv(StrEnv* env, s32 flip, s32 x0, s32 y0, s32 x1, s32 y1, s32 w, s32 h);
void strInit(CdlLOC* pos, void (*cb)());
void strCallback(void);
s32 strNextVlc(StrEnv* env, u32 limit);
u_long* strNext(StrEnv* env, u32 limit);
void strSync(StrEnv* env, s32 unused);
void strKickCD(CdlLOC* pos);

s32 moviePlaySingleTracLogoMovie(u_long* buf, char* name)
{
    s32 size;
    s32 flip;
    s32 fd;
    s32 pos;
    s32 nsec;
    s32 abort;

    flip = 0;
    fd = fileioOpenFile(name, &size);
    if (fd == -1) {
        return 0;
    }
    do {
        fileioReadFile(fd, buf, 0x800);
    } while (0);
    gSectorsPer = ((u16*)buf)[3];
    nsec = gSectorsPer;
    fileioReadFile(fd, buf + 512, (nsec - 1) << 11);
    SliceSize = 0x780;
    SliceRect.w = 16;
    SliceRect.h = 240;
    pos = gSectorsPer << 11;
    SetDefDispEnv(&Disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&Disp[1], 320, 0, 320, 240);
    Disp[1].isrgb24 = 0;
    Disp[0].isrgb24 = 0;
    DecDCTReset(0);
    abort = 0;
    while (pos <= size) {
        disp_mdec(buf, &flip);
        UpdCtlPad();
        if ((GetCtlPad(2, 2) & 0xff) || (GetCtlPad(1, 2) & 0xff)) {
            abort = 1;
            break;
        }
        fileioReadFile(fd, buf, gSectorsPer << 11);
        pos = pos + (gSectorsPer << 11);
    }
    if (flip == 1) {
        screenDisplayToDisplay(0, 1);
    }
    fileioCloseFile(fd);
    return abort;
}

void disp_mdec(u_long* src, s32* pflip)
{
    u_long* d;
    u_long* s;
    s32 i;
    s32 j;

    d = mdec_bs;
    for (i = 0; i < gSectorsPer; i++) {
        s = src + 8;
        for (j = 0; j < 504; j++) {
            *d++ = *s++;
        }
        src += 512;
    }
    DecDCTvlc(mdec_bs, mdec_rl);
    DecDCTin(mdec_rl, 0);
    SliceRect.x = (*pflip != 0) ? 320 : 0;
    SliceRect.y = 0;
    i = SliceRect.x + 320;
    while (SliceRect.x < i) {
        DecDCTout(mdec_image, SliceSize);
        DecDCToutSync(0);
        LoadImage((RECT*)&SliceRect.x, mdec_image);
        SliceRect.x = SliceRect.x + SliceRect.w;
    }
    VSync(0);
    PutDispEnv(&Disp[*pflip]);
    *pflip = (*pflip == 0);
}

void movieInit(void)
{
    mdec_bs = (u_long*)rtGetDoubleBufferArea();
    mdec_rl = (u_long*)rtGetDoubleBufferArea() + 5040;
    mdec_image = (u_long*)rtGetDoubleBufferArea();
}

s32 moviePlaySlideShow(char* fmt, s32 count, s32 secs, s32 allowAbort)
{
    char name[40];
    s32 i;
    s32 t0;
    s32 first;
    s32 elapsed;

    soundInterruptPlayDA();
    first = ctlpadAnyKey(2) & 0xff;
    for (i = 0; i < count; i++) {
        t0 = GetCurTics();
        sprintf(name, fmt, i);
        fileioLoadFileIntoRam(rtGetDoubleBufferArea(), name);
        screenDisplayImage(1, (u_long*)rtGetDoubleBufferArea(), 0, 0);
        for (elapsed = GetCurTics() - t0; elapsed < secs * 68644; elapsed = GetCurTics() - t0) {
            VSync(0);
            UpdCtlPad();
            if (ctlpadAnyKey(2) & 0xff) {
                if (first == 0 && allowAbort != 0) {
                    return -1;
                }
            } else {
                first = 0;
            }
        }
    }
    return 0;
}

void strSetDefDecEnv(StrEnv* env, s32 flip, s32 x0, s32 y0, s32 x1, s32 y1, s32 w, s32 h)
{
    u8* p;
    s32 size;

    p = rtGetDoubleBufferArea();
    size = 320 * 240;
    p += 32 * 2048;
    env->buf[0] = (u_long*)p;
    p += size;
    env->buf[1] = (u_long*)p;
    p += size;
    env->idx = 0;
    env->img = (u_long*)p;
    env->flip = flip ^ 1;
    env->done = 0;
    setRECT(&env->rect[0], x0, y0, w, h);
    setRECT(&env->rect[1], x1, y1, w, h);
    if (gIsRGB24) {
        setRECT(&env->slice, x0, y0, 24, h);
    } else {
        setRECT(&env->slice, x0, y0, 16, h);
    }
}

void strInit(CdlLOC* pos, void (*cb)())
{
    DecDCTReset(0);
    Rewind_Switch = 0;
    DecDCToutCallback(cb);
    StSetRing(sect_buff, 32);
    StSetStream(gIsRGB24, 1, -1, 0, 0);
    strKickCD(pos);
}

void strCallback(void)
{
    if (gIsRGB24 != 0 && StCdIntrFlag != 0) {
        StCdInterrupt();
        StCdIntrFlag = 0;
    }
    LoadImage(&dec.slice, dec.img);
    dec.slice.x = dec.slice.x + dec.slice.w;
    if (dec.slice.x < dec.rect[dec.flip].x + dec.rect[dec.flip].w) {
        DecDCTout(dec.img, (dec.slice.w * dec.slice.h) / 2);
    } else {
        dec.done = 1;
        dec.flip = (dec.flip == 0);
        dec.slice.x = dec.rect[dec.flip].x;
        dec.slice.y = dec.rect[dec.flip].y;
    }
}

s32 strNextVlc(StrEnv* env, u32 limit)
{
    s32 n;
    u_long* p;

    for (n = 0x800000; n != 0; n--) {
        p = strNext(env, limit);
        if (p != 0) {
            env->idx = (env->idx == 0);
            DecDCTvlc(p, env->buf[env->idx]);
            StFreeRing(p);
            return 0;
        }
    }
    return -1;
}

u_long* strNext(StrEnv* env, u32 limit)
{
    u_long* addr;
    u_long* header;
    s32 n;
    s32 w;
    s32 width;
    StHEADER* frame;

    n = 0x800000;
    do {
        if (StGetNext(&addr, &header) == 0) {
            goto found;
        }
        n--;
    } while (n != 0);
    return 0;
found:
    frame = (StHEADER*)header;
    lastFrame = frame->frameCount;
    if (frame->frameCount >= limit) {
        Rewind_Switch = 1;
    }
    w = frame->width;
    if (lastW != w || lastH != frame->height) {
        lastW = frame->width;
        lastH = frame->height;
    }
    if (gIsRGB24) {
        w = lastW;
        width = (w * 3) / 2;
    } else {
        width = lastW;
    }
    env->rect[1].w = width;
    env->rect[0].w = env->rect[1].w;
    do {
    } while (0);
    env->rect[1].h = lastH;
    env->rect[0].h = env->rect[1].h;
    env->slice.h = lastH;
    return addr;
}

void strSync(StrEnv* env, s32 unused)
{
    s32 n;

    *(&n) = 0x800000;
    while (env->done == 0) {
        *(&n) = *(&n) - 1;
        if (*(&n) == 0) {
            printf("time out in decoding !\n");
            env->done = 1;
            env->flip = (env->flip == 0);
            env->slice.x = env->rect[env->flip].x;
            env->slice.y = env->rect[env->flip].y;
        }
    }
    env->done = 0;
}

void strKickCD(CdlLOC* pos)
{
    while (CdControl(0x15, (u_char*)pos, 0) == 0) { }
    while (CdRead2(0x1C0) == 0) { }
}

s32 moviecdPlayMovie(char* name, MovieCallback cb, u8 wait, u8 rgb24, s32 limit, s32 x0, s32 y0,
    s32 x1, s32 y1, s32 w, s32 h, DISPENV* disp, u8* pflip)
{
    CdlFILE pos;
    u8 waitFlag;
    s32 done;
    u8 quit;
    s32 mode;
    s32 flip;

    done = 0;
    quit = 0;
    mode = 2;
    gIsRGB24 = rgb24;
    waitFlag = wait;
    if (rgb24 != 0) {
        mode = 3;
    }
    if (CdSearchFile(&pos, name) == 0) {
        printf("In moviecdPlayMovie file %s not found\n", name);
        return 0;
    }
    sect_buff = (u_long*)rtGetDoubleBufferArea();
    SsSetSerialVol(0, soundGetMasterVolume(), soundGetMasterVolume());
    do {
        loc.minute = pos.pos.minute;
        loc.second = pos.pos.second;
        loc.sector = pos.pos.sector;
        strSetDefDecEnv(&dec, *pflip, x0, y0, x1, y1, w, h);
        strInit(&loc, strCallback);
        strNextVlc(&dec, limit);
        while (1) {
            if (done != 0) {
                break;
            }
            DecDCTin(dec.buf[dec.idx], mode);
            DecDCTout(dec.img, (dec.slice.w * dec.slice.h) / 2);
            strNextVlc(&dec, limit);
            strSync(&dec, 0);
            VSync(0);
            flip = (dec.flip == 0);
            *pflip = flip;
            if (rgb24 != 0) {
                disp[flip].isrgb24 = gIsRGB24;
            }
            PutDispEnv(&disp[flip]);
            SetDispMask(1);
            if (rgb24 == 0) {
                FntFlush(-1);
            }
            if (Rewind_Switch == 1) {
                break;
            }
            if (cb != 0) {
                done = cb(lastFrame, limit, flip);
            }
        }
        if (done != 0 || Rewind_Switch == 0 || waitFlag == 0) {
            quit = 1;
        }
    } while (quit == 0);
    SsSetSerialVol(0, 0, 0);
    DecDCToutCallback(0);
    CdDataCallback(0);
    CdReadyCallback(0);
    SsSetSerialVol(0, soundGetMusicVolume(), soundGetMusicVolume());
    CdControl(9, 0, 0);
    return done;
}
