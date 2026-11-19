#include "common.h"
#include <LIBGPU.H>

#include "tm1/HIER_SUB.H"

extern SubPolyWord* prim2D[3];
extern SubPolyWord pOut[5];

extern u32 sdDrawModeTag;
extern u32 sdDrawModeCode0;
extern u32 sdDrawModeCode1;
extern u32 sdDrawModeDitherTag;
extern u32 sdDrawModeDitherCode0;
extern u32 sdDrawModeDitherCode1;

static SubPolyWord* p0;
static SubPolyWord* p1;
static SubPolyWord* p2;
static SubPolyWord* p3;

void subPoly3(Db* db, SubPoly* f, s32 count)
{
    SubPolyWord* out;
    SubPolyWord* tagPrim;
    SubPolyWord* colorPrim;
    DbPoly* colorPoly;
    SubPolyWord* c0;
    SubPolyWord* c1;
    SubPolyWord* c2;

    SubPolyWord* prim;
    SubPolyWord* q0;
    SubPolyWord* q1;
    SubPolyWord* q2;
    DbPoly* poly;
    s32 i;
    s32 half;
    s32 half2;
    s32 nWords;

    out = (SubPolyWord*)db->prim;
    for (i = 0; i < count; i++) {
        poly = f[i].poly;
        if (poly->numVert == 3) {
            half = poly->vertStride;
            prim = f[i].prim;
            poly = f[i].poly;
            q0 = prim + 2;
            q1 = q0 + half;
            q2 = q1 + half;
            p0 = q0;
            p1 = q1;
            p2 = q2;
            nWords = poly->header.bytes.primLen + 1;
            prim2D[0] = out;
            prim2D[1] = out + nWords;
            prim2D[2] = prim2D[1] + nWords;
            if (db->areaEnd < (u8*)prim2D[2] + 160) {
                break;
            }
            half2 = half;
            pOut[0].position.vx = (prim[2].position.vx + q1->position.vx) >> 1;
            pOut[0].position.vy = (prim[2].position.vy + q1->position.vy) >> 1;
            pOut[1].position.vx = (q1->position.vx + q2->position.vx) >> 1;
            pOut[1].position.vy = (q1->position.vy + q2->position.vy) >> 1;
            pOut[2].position.vx = (q2->position.vx + prim[2].position.vx) >> 1;
            pOut[2].position.vy = (q2->position.vy + prim[2].position.vy) >> 1;
            subLoadPoly3Words(2, half);
            if ((f[i].poly->header.word & 0x7F) != 0) {
                SubPolyWord* r0;
                SubPolyWord* r1;
                SubPolyWord* r2;

                r0 = p0;
                r1 = p1;
                r2 = p2;
                p0 = r0 + 1;
                p1 = r1 + 1;
                p2 = r2 + 1;
                pOut[0].uv.u = (r0[1].uv.u + r1[1].uv.u) >> 1;
                pOut[0].uv.v = (r0[1].uv.v + r1[1].uv.v) >> 1;
                pOut[1].uv.u = (r1[1].uv.u + r2[1].uv.u) >> 1;
                pOut[1].uv.v = (r1[1].uv.v + r2[1].uv.v) >> 1;
                pOut[2].uv.u = (r2[1].uv.u + r0[1].uv.u) >> 1;
                pOut[2].uv.v = (r2[1].uv.v + r0[1].uv.v) >> 1;
                pOut[1].uv.page = r0[1].uv.page;
                pOut[2].uv.page = r1[1].uv.page;
                subLoadPoly3Words(3, half);
            }
            colorPrim = f[i].prim;
            colorPoly = f[i].poly;
            c0 = colorPrim + 1;
            p0 = c0;
            if (colorPoly->header.bytes.numColor >= 2 || colorPoly->numNormal >= 2) {
                c1 = c0 + half;
                c2 = c1 + half;
                p1 = c1;
                p2 = c2;
                pOut[0].color.r = (colorPrim[1].color.r + c1->color.r) >> 1;
                pOut[0].color.g = (colorPrim[1].color.g + c1->color.g) >> 1;
                pOut[0].color.b = (colorPrim[1].color.b + c1->color.b) >> 1;
                pOut[1].color.r = (c1->color.r + c2->color.r) >> 1;
                pOut[1].color.g = (c1->color.g + c2->color.g) >> 1;
                pOut[1].color.b = (c1->color.b + c2->color.b) >> 1;
                pOut[1].color.cd = c1->color.cd;
                pOut[2].color.r = (c2->color.r + colorPrim[1].color.r) >> 1;
                pOut[2].color.g = (c2->color.g + colorPrim[1].color.g) >> 1;
                pOut[2].color.b = (c2->color.b + colorPrim[1].color.b) >> 1;
                subLoadPoly3Words(1, half2);
            } else {
                prim2D[0][1].word = colorPrim[1].word;
                prim2D[1][1].word = colorPrim[1].word;
                prim2D[2][1].word = colorPrim[1].word;
            }
            tagPrim = f[i].prim;
            tagPrim->bytes[3] = nWords - 1;
            prim2D[0]->bytes[3] = nWords - 1;
            prim2D[1]->bytes[3] = nWords - 1;
            prim2D[2]->bytes[3] = nWords - 1;
            out = prim2D[2] + nWords;
            subAddPrim(db, &f[i], &out, prim2D);
        }
    }
    db->prim = (u8*)out;
}

void subLoadPoly3Words(s32 col, s32 row)
{
    s32* q;

    q = &prim2D[0][col].word;
    *q = pOut[1].word;
    q += row;
    *q = p1->word;
    q += row;
    *q = pOut[0].word;
    q = &prim2D[1][col].word;
    *q = pOut[1].word;
    q += row;
    *q = pOut[2].word;
    q += row;
    *q = p2->word;
    q = &prim2D[2][col].word;
    *q = pOut[1].word;
    q += row;
    *q = pOut[2].word;
    q += row;
    *q = pOut[0].word;
    p1->word = pOut[2].word;
    p2->word = pOut[0].word;
}

void subPoly4(Db* db, SubPoly* f, s32 count)
{
    SubPolyWord* out;
    SubPolyWord* tagPrim;
    SubPolyWord* colorPrim;
    DbPoly* colorPoly;
    SubPolyWord* color0;
    SubPolyWord* color1;
    SubPolyWord* color2;
    SubPolyWord* color3;

    SubPolyWord* prim;
    SubPolyWord* q0;
    SubPolyWord* q1;
    SubPolyWord* q2;
    SubPolyWord* q3;
    DbPoly* poly;
    s32 i;
    s32 half;
    s32 half2;
    s32 nWords;
    s32 mx0;
    s32 my0;
    s32 mx2;
    s32 my2;

    out = (SubPolyWord*)db->prim;
    for (i = 0; i < count; i++) {
        poly = f[i].poly;
        if (poly->numVert == 4) {
            half = poly->vertStride;
            prim = f[i].prim;
            poly = f[i].poly;
            q0 = prim + 2;
            q1 = q0 + half;
            q2 = q1 + half;
            q3 = q2 + half;
            p0 = q0;
            p1 = q1;
            p2 = q2;
            p3 = q3;
            nWords = poly->header.bytes.primLen + 1;
            prim2D[0] = out;
            prim2D[1] = out + nWords;
            prim2D[2] = prim2D[1] + nWords;
            if (db->areaEnd < (u8*)prim2D[2] + 160) {
                break;
            }
            half2 = half;
            mx0 = (prim[2].position.vx + q2->position.vx) >> 1;
            pOut[0].position.vx = mx0;
            my0 = (prim[2].position.vy + q2->position.vy) >> 1;
            pOut[0].position.vy = my0;
            pOut[1].position.vx = (q1->position.vx + prim[2].position.vx) >> 1;
            pOut[1].position.vy = (q1->position.vy + prim[2].position.vy) >> 1;
            mx2 = (q1->position.vx + q3->position.vx) >> 1;
            pOut[2].position.vx = mx2;
            my2 = (q1->position.vy + q3->position.vy) >> 1;
            pOut[2].position.vy = my2;
            pOut[3].position.vx = (q3->position.vx + q2->position.vx) >> 1;
            pOut[3].position.vy = (q3->position.vy + q2->position.vy) >> 1;
            pOut[4].position.vx = (mx0 + mx2) >> 1;
            pOut[4].position.vy = (my0 + my2) >> 1;
            subLoadPoly4Words(2, half);
            if ((f[i].poly->header.word & 0x7F) != 0) {
                SubPolyWord* r0;
                SubPolyWord* r1;
                SubPolyWord* r2;
                SubPolyWord* r3;
                s32 u0;
                s32 v0;
                s32 u2;
                s32 v2;

                r0 = p0;
                r1 = p1;
                r2 = p2;
                r3 = p3;
                p0 = r0 + 1;
                p1 = r1 + 1;
                p2 = r2 + 1;
                p3 = r3 + 1;
                u0 = (r0[1].uv.u + r2[1].uv.u) >> 1;
                pOut[0].uv.u = u0;
                v0 = (r0[1].uv.v + r2[1].uv.v) >> 1;
                pOut[0].uv.v = v0;
                pOut[1].uv.u = (r1[1].uv.u + r0[1].uv.u) >> 1;
                pOut[1].uv.v = (r1[1].uv.v + r0[1].uv.v) >> 1;
                u2 = (r1[1].uv.u + r3[1].uv.u) >> 1;
                pOut[2].uv.u = u2;
                v2 = (r1[1].uv.v + r3[1].uv.v) >> 1;
                pOut[2].uv.v = v2;
                pOut[3].uv.u = (r3[1].uv.u + r2[1].uv.u) >> 1;
                pOut[3].uv.v = (r3[1].uv.v + r2[1].uv.v) >> 1;
                pOut[4].uv.u = (u0 + u2) >> 1;
                pOut[4].uv.v = (v0 + v2) >> 1;
                pOut[0].uv.page = r0[1].uv.page;
                pOut[4].uv.page = r1[1].uv.page;
                pOut[1].uv.page = r1[1].uv.page;
                pOut[2].uv.page = r1[1].uv.page;
                subLoadPoly4Words(3, half);
                /* Both palette updates use p1 in the original code. */
                p1 = prim2D[1] + 3;
                p1->uv.page = p0->uv.page;
                p1 = prim2D[2] + 3;
                p1->uv.page = p0->uv.page;
            }
            colorPrim = f[i].prim;
            colorPoly = f[i].poly;
            color0 = colorPrim + 1;
            p0 = color0;
            if (colorPoly->header.bytes.numColor >= 2 || colorPoly->numNormal >= 2) {
                s32 b0;
                s32 c0;
                s32 b2;
                s32 c2;
                s32 d0;
                s32 d2;

                color1 = color0 + half;
                color2 = color1 + half;
                color3 = color2 + half;
                p1 = color1;
                p2 = color2;
                p3 = color3;
                b0 = (colorPrim[1].color.r + color2->color.r) >> 1;
                pOut[0].color.r = b0;
                c0 = (colorPrim[1].color.g + color2->color.g) >> 1;
                pOut[0].color.g = c0;
                d0 = (colorPrim[1].color.b + color2->color.b) >> 1;
                pOut[0].color.b = d0;
                pOut[0].color.cd = colorPrim[1].color.cd;
                pOut[1].color.r = (color1->color.r + colorPrim[1].color.r) >> 1;
                pOut[1].color.g = (color1->color.g + colorPrim[1].color.g) >> 1;
                pOut[1].color.b = (color1->color.b + colorPrim[1].color.b) >> 1;
                pOut[1].color.cd = colorPrim[1].color.cd;
                b2 = (color1->color.r + color3->color.r) >> 1;
                pOut[2].color.r = b2;
                c2 = (color1->color.g + color3->color.g) >> 1;
                pOut[2].color.g = c2;
                d2 = (color1->color.b + color3->color.b) >> 1;
                pOut[2].color.b = d2;
                pOut[3].color.r = (color3->color.r + color2->color.r) >> 1;
                pOut[3].color.g = (color3->color.g + color2->color.g) >> 1;
                pOut[3].color.b = (color3->color.b + color2->color.b) >> 1;
                pOut[4].color.r = (b0 + b2) >> 1;
                pOut[4].color.g = (c0 + c2) >> 1;
                pOut[4].color.b = (d0 + d2) >> 1;
                pOut[4].color.cd = colorPrim[1].color.cd;
                subLoadPoly4Words(1, half2);
            } else {
                prim2D[0][1].word = colorPrim[1].word;
                prim2D[1][1].word = colorPrim[1].word;
                prim2D[2][1].word = colorPrim[1].word;
            }
            tagPrim = f[i].prim;
            tagPrim->bytes[3] = nWords - 1;
            prim2D[0]->bytes[3] = nWords - 1;
            prim2D[1]->bytes[3] = nWords - 1;
            prim2D[2]->bytes[3] = nWords - 1;
            out = prim2D[2] + nWords;
            subAddPrim(db, &f[i], &out, prim2D);
        }
    }
    db->prim = (u8*)out;
}

void subLoadPoly4Words(s32 col, s32 row)
{
    s32* q;

    q = &prim2D[0][col].word;
    *q = pOut[0].word;
    q += row;
    *q = pOut[4].word;
    q += row;
    *q = p2->word;
    q += row;
    *q = pOut[3].word;
    q = &prim2D[1][col].word;
    *q = pOut[1].word;
    q += row;
    *q = p1->word;
    q += row;
    *q = pOut[4].word;
    q += row;
    *q = pOut[2].word;
    q = &prim2D[2][col].word;
    *q = pOut[4].word;
    q += row;
    *q = pOut[2].word;
    q += row;
    *q = pOut[3].word;
    q += row;
    *q = p3->word;
    p1->word = pOut[1].word;
    p2->word = pOut[0].word;
    p3->word = pOut[4].word;
}

void subAddPrim(Db* db, SubPoly* f, SubPolyWord** cursor, SubPolyWord** extra)
{
    if ((f->poly->header.word & DB_POLY_DRAW_MODE) != 0 || f->dither != 0) {
        (*cursor)->word = sdDrawModeTag;
        (*cursor)[1].word = sdDrawModeCode0;
        (*cursor)[2].word = sdDrawModeCode1;
        AddPrim(db->ot + f->otIdx, *cursor);
        *cursor += sizeof(DR_MODE) / sizeof(SubPolyWord);
        AddPrim(db->ot + f->otIdx, f->prim);
        AddPrim(db->ot + f->otIdx, extra[0]);
        AddPrim(db->ot + f->otIdx, extra[1]);
        AddPrim(db->ot + f->otIdx, extra[2]);
        if (f->dither != 0) {
            (*cursor)->word = sdDrawModeDitherTag;
            (*cursor)[1].word = sdDrawModeDitherCode0;
            (*cursor)[2].word = sdDrawModeDitherCode1;
        } else {
            (*cursor)->word = sdDrawModeTag;
            (*cursor)[1].word = sdDrawModeCode0;
            (*cursor)[2].word = sdDrawModeCode1;
        }
        if ((f->poly->header.word & DB_POLY_DRAW_MODE) != 0) {
            (*cursor)[2].word = *f->texWindow;
            (*cursor)[1].bytes[0] = f->tpage;
        }
        AddPrim(db->ot + f->otIdx, *cursor);
        *cursor += sizeof(DR_MODE) / sizeof(SubPolyWord);
    } else {
        AddPrim(db->ot + f->otIdx, f->prim);
        AddPrim(db->ot + f->otIdx, extra[0]);
        AddPrim(db->ot + f->otIdx, extra[1]);
        AddPrim(db->ot + f->otIdx, extra[2]);
    }
}
