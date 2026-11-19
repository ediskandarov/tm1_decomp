#include "common.h"

#include <LIBGPU.H>
#include <LIBGTE.H>

#include "tm1/CS.H"
#include "tm1/MATH.H"
#include "tm1/TIMER.H"

#include "tm1/HD.H"
#include "tm1/HDP.H"

typedef struct HdBoxProjection {
    VECTOR3 point;
    s32 pad0C;
    s32 distances[6];
} HdBoxProjection;

typedef struct HdVolumeProjection {
    VECTOR3 point0;
    s32 pad0C;
    VECTOR3 point1;
    s32 pad1C;
    s32 distances0[6];
    s32 distances1[6];
} HdVolumeProjection;

HdEnt* fastHdStack = (HdEnt*)0x1F800000;
VECTOR3* hdCsOtherPos = NULL;
MATRIX* hdCsOtherMat = NULL;
Cs* hdCsSelf = NULL;

static HdEnt* hdStack;
static s32 hd_stackIdx;
static s32 matCnt;

static MATRIX* matPtrs[70];
static s32 worldToTp[9][3];

void hdPush(DbNode* node, VECTOR3* v, s32 matIdx, s32 id0, s16 id1)
{
    HdEnt* e;
    HdEnt* base;
    s32 idx;

    idx = hd_stackIdx;
    if ((u32)idx < 40) {
        do {
            base = fastHdStack;
        } while (0);
    } else {
        do {
            base = hdStack;
        } while (0);
    }
    e = (HdEnt*)(idx * sizeof(*e) + (s32)base);
    e->node = node;
    copyVector(&e->v, v);
    do {
        e->matIdx = matIdx;
        e->id0 = id0;
        e->id1 = id1;
    } while (0);
    if (hd_stackIdx < 200) {
        hd_stackIdx++;
    }
}

s32 hdPop(HdEnt** out)
{
    s32 i;
    HdEnt* b;

    i = hd_stackIdx - 1;
    hd_stackIdx = i;
    if (i >= 0) {
        if ((u32)i < 40) {
            b = fastHdStack;
        } else {
            b = hdStack;
        }
        b += i;
        *out = b;
    }
    return hd_stackIdx;
}

void hdSetNodeIds(s16* out0, s16* out1, HdEnt* e, s16 id0, s16 id1)
{
    if (id0 == 0) {
        *out0 = e->id0;
        *out1 = e->id1;
    } else {
        *out0 = id0;
        *out1 = id1;
    }
}

s32 hdIsPtInSphere(VECTOR3* pt, s32 radius, VECTOR3* centre, s32 slack)
{
    u32 d[3];
    u32 dist;

    if (radius <= 0xB504) {
        d[0] = (u32)pt->vx - (u32)centre->vx;
        d[1] = (u32)pt->vy - (u32)centre->vy;
        d[2] = (u32)pt->vz - (u32)centre->vz;
        dist = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        if ((s32)dist < 0 || (s32)dist >= (s32)((u32)radius * (u32)radius + (u32)slack)) {
            return 0;
        }
        return 1;
    }
    return 1;
}

void hdTraverse(Cs* cs, VECTOR3* pt, HdCsHit* res, s32 slack)
{
    HdEnt* e;
    s16 id0;
    s16 id1;
    s32 r;

    e = NULL;
    hd_stackIdx = 0;
    hdPush(cs->epNode, pt, matCnt, 0, 0);
    while (res->depth == 0 && hdPop(&e) >= 0) {
        switch (e->node->h.op) {
        case DB_OP_OBJECT: {
            DbObject* n = &e->node->obj;

            if (n->skip == 1) {
                break;
            }
            if (hdIsPtInSphere(&n->center, n->radius, &e->v, slack) == 0) {
                break;
            }
            res->depth = hdPntBox(&n->box, &e->v, res, e->matIdx);
            if (res->hit == 0) {
                break;
            }
            hdSetHitTag(res, n->h.kind, n->sub, e);
            break;
        }
        case DB_OP_BOX: {
            DbBox* n = &e->node->box;

            if (n->code == -2) {
                r = hdpPntBox(&n->box, &e->v, &res->hit, 1);
            } else {
                r = hdPntBox(&n->box, &e->v, res, e->matIdx);
            }
            res->depth = r;
            if (res->hit == 0) {
                break;
            }
            hdSetHitTag(res, n->h.kind, n->code, e);
            break;
        }
        case DB_OP_VOLUME: {
            DbVolume* n = &e->node->vol;

            if (n->code == -2) {
                r = hdpPntVolume(n, &e->v, res, 1);
            } else {
                r = hdPntVolume(n, &e->v, res, e->matIdx);
            }
            res->depth = r;
            if (res->hit == 0) {
                break;
            }
            hdSetHitTag(res, n->h.kind, n->sub, e);
            break;
        }
        case DB_OP_GROUP: {
            DbGroup* n = &e->node->grp;
            s32 i;
            s32 cnt;

            if (n->flags & DB_NODE_NO_COLLIDE) {
                break;
            }
            if (!(n->flags & DB_NODE_NO_BOUNDS_TEST)) {
                if (hdIsPtInSphere(&n->center, n->radius, &e->v, slack) == 0) {
                    break;
                }
            }
            cnt = n->nChild;
            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            i = 0;
            if (cnt != 0) {
                while (1) {
                    hdPush(n->child[i], &e->v, e->matIdx, id0, id1);
                    i++;
                    if (i >= cnt) {
                        break;
                    }
                }
            }
            break;
        }
        case DB_OP_LOD:
            hdLod(&e->node->lod, e);
            break;
        case DB_OP_TRANS: {
            DbTrans* n = &e->node->trans;
            s32 i;

            if (n->flags & DB_NODE_NO_COLLIDE) {
                break;
            }
            setVector(&e->v, e->v.vx - n->ofs.vx, e->v.vy - n->ofs.vy, e->v.vz - n->ofs.vz);
            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            for (i = 0; i < n->nChild; i++) {
                hdPush(n->child[i], &e->v, e->matIdx, id0, id1);
            }
            break;
        }
        case DB_OP_ROT: {
            DbRot* n = &e->node->rot;
            VECTOR3 v;
            s32 dead[2];
            s32 i;

            if (n->flags & DB_NODE_NO_COLLIDE) {
                break;
            }
            setVector(&e->v, e->v.vx - n->mat.t[0], e->v.vy - n->mat.t[1], e->v.vz - n->mat.t[2]);
            v.vx = e->v.vx;
            v.vy = e->v.vy;
            v.vz = e->v.vz;
            mathMulVec(&n->mat, &v, &e->v);
            matPtrs[e->matIdx] = &n->mat;
            e->matIdx++;
            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            for (i = 0; i < n->nChild; i++) {
                hdPush(n->child[i], &e->v, e->matIdx, id0, id1);
            }
            break;
        }
        case DB_OP_SWITCH: {
            DbSwitch* n = &e->node->sw;

            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            hdPush(n->child[n->sel], &e->v, e->matIdx, id0, id1);
            break;
        }
        case DB_OP_ANIM: {
            DbAnim* n = &e->node->anim;

            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            hdPush(n->child[n->cur], &e->v, e->matIdx, id0, id1);
            break;
        }
        case DB_OP_CTRL: {
            DbCtrl* n = &e->node->ctrl;

            if (n->flags & DB_NODE_NO_COLLIDE) {
                break;
            }
            hdSetNodeIds(&id0, &id1, e, n->h.kind, n->sub);
            hdCtrlNode(e, id0, id1);
            break;
        }
        }
    }
    matCnt = e->matIdx;
}

void hdLod(DbLod* node, HdEnt* e)
{
    VECTOR3 d;
    u32 dist;
    s32 i;
    s32 more;

    d.vx = node->center.vx - e->v.vx;
    d.vy = node->center.vy - e->v.vy;
    d.vz = node->center.vz - e->v.vz;
    d.vx = d.vx >> 3;
    d.vy = d.vy >> 3;
    d.vz = d.vz >> 3;
    more = 1;
    i = 0;
    dist = d.vx * d.vx + d.vy * d.vy + d.vz * d.vz;
    for (i = 0; i < node->nRec; i++) {
        if (more == 0) {
            break;
        }
        if (dist >= node->rec[i]->near && dist < node->rec[i]->far) {
            hdPush(node->rec[i]->child, &e->v, e->matIdx, e->id0, e->id1);
            more = 0;
        }
    }
}

s32 hdPntBox(HdBox* b, VECTOR3* pos, HdCsHit* res, s32 matIdx)
{
    HdBoxProjection projection;
    s32 d1[6];
    VECTOR3 q;
    VECTOR3 p;
    VECTOR3 tmp;
    VECTOR3 w;
    VECTOR n0;
    VECTOR n1;
    s32 fwd;
    s32 i;
    s32 j;
    s32 k;
    s32 first;
    s32 second;
    s32 cnt;
    s32 depth;
    s32 result;
    HdBoxProjection* projected;

    fwd = GetFrameCount() & 1;
    i = 0;
    if (fwd == 0) {
        i = hdCsSelf->nhist - 1;
    }
    while (res->hit != 1 && i >= 0 && i < hdCsSelf->nhist) {
        mathMulTransVecShort(&hdCsSelf->mat, &hdCsSelf->hist[i], &p);
        worldToTp[i][0] = p.vx + hdCsSelf->pos.vx;
        worldToTp[i][1] = p.vy + hdCsSelf->pos.vy;
        worldToTp[i][2] = p.vz + hdCsSelf->pos.vz;
        for (j = 0; j < matIdx; j++) {
            tmp.vx = p.vx;
            tmp.vy = p.vy;
            tmp.vz = p.vz;
            mathMulVec(matPtrs[j], &tmp, &p);
        }
        p.vx = p.vx + pos->vx;
        p.vy = p.vy + pos->vy;
        p.vz = p.vz + pos->vz;
        mathMulVec(&b->mat, &p, &projection.point);
        projection.distances[0] = b->lo.vx + projection.point.vx;
        if (projection.distances[0] > 0) {
            projection.distances[1] = b->lo.vy + projection.point.vy;
            if (projection.distances[1] > 0) {
                projection.distances[2] = b->lo.vz + projection.point.vz;
                if (projection.distances[2] > 0) {
                    projection.distances[3] = b->hi.vx - projection.distances[0];
                    if (projection.distances[3] > 0) {
                        projection.distances[4] = b->hi.vy - projection.distances[1];
                        if (projection.distances[4] > 0) {
                            projection.distances[5] = b->hi.vz - projection.distances[2];
                            if (projection.distances[5] > 0) {
                                res->histIdx = i;
                                res->hit = 1;
                            }
                        }
                    }
                }
            }
        }
        if (fwd != 0) {
            i++;
        } else {
            i--;
        }
    }
    if (res->hit != 1) {
        return 0;
    }
    k = res->histIdx;
    mathMulTransVecShort(hdCsOtherMat, &hdCsSelf->hist[k], &w);
    w.vx = w.vx + hdCsOtherPos->vx;
    w.vy = w.vy + hdCsOtherPos->vy;
    w.vz = w.vz + hdCsOtherPos->vz;
    q.vx = w.vx - worldToTp[k][0];
    q.vy = w.vy - worldToTp[k][1];
    i = 0;
    q.vz = w.vz - worldToTp[k][2];
    for (i = 0; i < matIdx; i++) {
        tmp.vx = q.vx;
        tmp.vy = q.vy;
        tmp.vz = q.vz;
        mathMulVec(matPtrs[i], &tmp, &q);
    }
    q.vx = q.vx + p.vx;
    q.vy = q.vy + p.vy;
    q.vz = q.vz + p.vz;
    mathMulVec(&b->mat, &q, &projection.point);
    d1[0] = b->lo.vx + projection.point.vx;
    d1[1] = b->lo.vy + projection.point.vy;
    d1[2] = b->lo.vz + projection.point.vz;
    first = -1;
    second = 0;
    d1[3] = b->hi.vx - d1[0];
    i = 0;
    cnt = 0;
    d1[4] = b->hi.vy - d1[1];
    d1[5] = b->hi.vz - d1[2];
    for (; i < 6; i++) {
        if ((projection.distances[i] & 0x80000000) != (d1[i] & 0x80000000)) {
            if (first < 0) {
                first = i;
            } else {
                second = i;
            }
            if (i < 3) {
                setVector(&res->normal, b->mat.m[i][0], b->mat.m[i][1], b->mat.m[i][2]);
            } else {
                setVector(
                    &res->normal, -b->mat.m[i - 3][0], -b->mat.m[i - 3][1], -b->mat.m[i - 3][2]);
            }
            cnt++;
        }
    }
    if (cnt >= 2) {
        if (first < 3) {
            n0.vx = b->mat.m[first][0];
            n0.vy = b->mat.m[first][1];
            n0.vz = b->mat.m[first][2];
        } else {
            j = first - 3;
            n0.vx = -b->mat.m[j][0];
            n0.vy = -b->mat.m[j][1];
            n0.vz = -b->mat.m[j][2];
        }
        if (second < 3) {
            n1.vx = b->mat.m[second][0];
            n1.vy = b->mat.m[second][1];
            n1.vz = b->mat.m[second][2];
        } else {
            j = second - 3;
            n1.vx = -b->mat.m[j][0];
            n1.vy = -b->mat.m[j][1];
            n1.vz = -b->mat.m[j][2];
        }
        n0.vx = (n0.vx + n1.vx) >> 1;
        n0.vy = (n0.vy + n1.vy) >> 1;
        n0.vz = (n0.vz + n1.vz) >> 1;
        VectorNormal(&n0, (VECTOR*)&res->normal);
        projected = &projection;
        depth = projected->distances[first];
        if (depth < projected->distances[second]) {
            depth = projected->distances[second];
        }
        return depth;
    } else if (cnt != 0) {
        result = projection.distances[first];
    } else {
        res->unk04 = 1;
        result = 1;
    }
    return result;
}

s32 hdPntVolume(DbVolume* n, VECTOR3* pos, HdCsHit* res, s32 matIdx)
{
    HdVolumeProjection projection;
    VECTOR3 q;
    VECTOR3 p;
    VECTOR3 tmp;
    VECTOR3 w;
    VECTOR n0;
    VECTOR n1;
    s32 fwd;
    s32 i;
    s32 j;
    s32 k;
    s32 code;
    s32 first;
    s32 second;
    s32 cnt;
    s32 depth;
    s32 result;
    HdVolumeProjection* projected;

    fwd = GetFrameCount() & 1;
    if (fwd != 0) {
        i = 0;
    } else {
        i = hdCsSelf->nhist - 1;
    }
    while (res->hit != 1 && i >= 0 && i < hdCsSelf->nhist) {
        mathMulTransVecShort(&hdCsSelf->mat, &hdCsSelf->hist[i], &p);
        worldToTp[i][0] = p.vx + hdCsSelf->pos.vx;
        worldToTp[i][1] = p.vy + hdCsSelf->pos.vy;
        worldToTp[i][2] = p.vz + hdCsSelf->pos.vz;
        for (j = 0; j < matIdx; j++) {
            tmp.vx = p.vx;
            tmp.vy = p.vy;
            tmp.vz = p.vz;
            mathMulVec(matPtrs[j], &tmp, &p);
        }
        p.vx = p.vx + pos->vx;
        p.vy = p.vy + pos->vy;
        p.vz = p.vz + pos->vz;
        mathMulVec(&n->mat0, &p, &projection.point0);
        projection.distances0[0] = n->d0.vx + projection.point0.vx;
        if (projection.distances0[0] > 0) {
            projection.distances0[1] = n->d0.vy + projection.point0.vy;
            if (projection.distances0[1] > 0) {
                projection.distances0[2] = n->d0.vz + projection.point0.vz;
                if (projection.distances0[2] > 0) {
                    if (n->nplane < 4) {
                        res->hit = 1;
                    } else {
                        mathMulVec(&n->mat1, &p, &projection.point1);
                        projection.distances0[3] = n->d1.vx + projection.point1.vx;
                        if (projection.distances0[3] > 0) {
                            if (n->nplane < 5) {
                                res->hit = 1;
                            } else {
                                projection.distances0[4] = n->d1.vy + projection.point1.vy;
                                if (projection.distances0[4] > 0) {
                                    if (n->nplane < 6) {
                                        res->hit = 1;
                                    } else {
                                        projection.distances0[5] = n->d1.vz + projection.point1.vz;
                                        if (projection.distances0[5] > 0) {
                                            res->hit = 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (res->hit == 1) {
            res->histIdx = i;
        }
        if (fwd != 0) {
            i++;
        } else {
            i--;
        }
    }
    if (res->hit == 0) {
        return 0;
    }
    code = n->code;
    if (code >= 0) {
        if (code < 3) {
            setVector(&res->normal, n->mat0.m[code][0], n->mat0.m[code][1], n->mat0.m[code][2]);
        } else {
            j = code - 3;
            setVector(&res->normal, n->mat1.m[j][0], n->mat1.m[j][1], n->mat1.m[j][2]);
        }
        result = projection.distances0[code];
    } else {
        k = res->histIdx;
        mathMulTransVecShort(hdCsOtherMat, &hdCsSelf->hist[k], &w);
        w.vx = w.vx + hdCsOtherPos->vx;
        w.vy = w.vy + hdCsOtherPos->vy;
        w.vz = w.vz + hdCsOtherPos->vz;
        q.vx = w.vx - worldToTp[k][0];
        q.vy = w.vy - worldToTp[k][1];
        i = 0;
        q.vz = w.vz - worldToTp[k][2];
        for (i = 0; i < matIdx; i++) {
            tmp.vx = q.vx;
            tmp.vy = q.vy;
            tmp.vz = q.vz;
            mathMulVec(matPtrs[i], &tmp, &q);
        }
        q.vx = q.vx + p.vx;
        q.vy = q.vy + p.vy;
        q.vz = q.vz + p.vz;
        mathMulVec(&n->mat0, &q, &projection.point0);
        projection.distances1[0] = n->d0.vx + projection.point0.vx;
        projection.distances1[1] = n->d0.vy + projection.point0.vy;
        projection.distances1[2] = n->d0.vz + projection.point0.vz;
        if (n->nplane >= 4) {
            mathMulVec(&n->mat1, &q, &projection.point1);
            projection.distances1[3] = n->d1.vx + projection.point1.vx;
            projection.distances1[4] = n->d1.vy + projection.point1.vy;
            projection.distances1[5] = n->d1.vz + projection.point1.vz;
        }
        first = -1;
        second = 0;
        i = 0;
        cnt = 0;
        for (; i < n->nplane; i++) {
            if ((projection.distances0[i] & 0x80000000)
                != (projection.distances1[i] & 0x80000000)) {
                if (first < 0) {
                    first = i;
                } else {
                    second = i;
                }
                if (i < 3) {
                    setVector(&res->normal, n->mat0.m[i][0], n->mat0.m[i][1], n->mat0.m[i][2]);
                } else {
                    setVector(&res->normal, n->mat1.m[i - 3][0], n->mat1.m[i - 3][1],
                        n->mat1.m[i - 3][2]);
                }
                cnt++;
            }
        }
        if (cnt >= 2) {
            if (first < 3) {
                n0.vx = n->mat0.m[first][0];
                n0.vy = n->mat0.m[first][1];
                n0.vz = n->mat0.m[first][2];
            } else {
                j = first - 3;
                n0.vx = n->mat1.m[j][0];
                n0.vy = n->mat1.m[j][1];
                n0.vz = n->mat1.m[j][2];
            }
            if (second < 3) {
                n1.vx = n->mat0.m[second][0];
                n1.vy = n->mat0.m[second][1];
                n1.vz = n->mat0.m[second][2];
            } else {
                j = second - 3;
                n1.vx = n->mat1.m[j][0];
                n1.vy = n->mat1.m[j][1];
                n1.vz = n->mat1.m[j][2];
            }
            n0.vx = (n0.vx + n1.vx) >> 1;
            n0.vy = (n0.vy + n1.vy) >> 1;
            n0.vz = (n0.vz + n1.vz) >> 1;
            VectorNormal(&n0, (VECTOR*)&res->normal);
            projected = &projection;
            depth = projected->distances0[first];
            if (depth < projected->distances0[second]) {
                depth = projected->distances0[second];
            }
            return depth;
        } else if (cnt != 0) {
            result = projection.distances0[first];
        } else {
            res->unk04 = 1;
            result = 1;
        }
    }
    return result;
}

void hdCtrlNode(HdEnt* e, s16 id0, s16 id1)
{
    DbCtrl* node;
    s32 i;
    s32 in;
    HdBox* box;

    node = &e->node->ctrl;
    i = 0;
    in = 1;
    while (in != 0) {
        if (i >= node->nBox) {
            break;
        }
        box = node->box[i];
        if (hdInCtrlBox(box, &e->v) != 0) {
            in = 0;
        }
        i++;
    }
    if (in != 0) {
        if (node->childOut != 0) {
            hdPush(node->childOut, &e->v, e->matIdx, id0, id1);
        }
    } else {
        if (node->childIn != 0) {
            hdPush(node->childIn, &e->v, e->matIdx, id0, id1);
        }
    }
}

s32 hdInCtrlBox(HdBox* box, VECTOR3* pt)
{
    VECTOR3 r;
    VECTOR3 q;

    mathMulVec(&box->mat, pt, &r);
    if ((q.vz = box->lo.vz + r.vz) >= 0 && (q.vy = box->lo.vy + r.vy) >= 0
        && (q.vx = box->lo.vx + r.vx) >= 0 && box->hi.vx - q.vx >= 0 && box->hi.vy - q.vy >= 0
        && box->hi.vz - q.vz >= 0) {
        return 1;
    }
    return 0;
}

void hdSetHitTag(HdCsHit* hit, s16 id0, s16 id1, HdEnt* e)
{
    if (id0 == 0) {
        hit->tag0 = e->id0;
        hit->tag1 = e->id1;
    } else {
        hit->tag0 = id0;
        hit->tag1 = id1;
    }
}

void HdCsTest(Cs* cs, HdCsHit* hit, s32 doWorld, MATRIX* otherMat, VECTOR3* otherPos)
{
    VECTOR3 delta;
    VECTOR3 local;
    VECTOR3 tmp;
    Cs* p;
    s32 rad;
    s32 i;
    MATRIX** m;
    DbGroup* info;

    hit->hit = 0;
    hit->unk04 = 0;
    hit->obj = 0;
    hit->histIdx = -1;
    hit->tag0 = -1;
    hit->depth = 0;
    hdCsOtherMat = otherMat;
    hdCsSelf = cs;
    hdCsOtherPos = otherPos;
    info = &cs->epNode->grp;
    if (info == 0 || info->h.op != DB_OP_GROUP) {
        rad = 100;
    } else {
        rad = cs->epNode->grp.radius;
    }
    p = csGetWorldCs();
    rad = rad * rad;
    if (p->epNode != NULL && doWorld != 0) {
        matCnt = 0;
        hit->obj = p;
        hdTraverse(p, &cs->pos, hit, rad);
        if (hit->depth == -2) {
            hit->depth = 0;
        }
    }
    p = csGetCsList();
    while (hit->depth == 0) {
        if (p == 0) {
            break;
        }
        matCnt = 0;
        if (p->drawMode != 0 && p != cs && p->unk0C != 0) {
            m = matPtrs;
            *m = &p->mat;
            delta.vx = cs->pos.vx - p->pos.vx;
            delta.vy = cs->pos.vy - p->pos.vy;
            matCnt = 1;
            delta.vz = cs->pos.vz - p->pos.vz;
            mathMulVec(&p->mat, &delta, &local);
            hdTraverse(p, &local, hit, rad);
            if (hit->hit == 1) {
                hit->obj = p;
            }
        }
        p = p->next;
    }
    if (hit->hit == 1) {
        for (i = matCnt - 1; i >= 0; i--) {
            copyVector(&tmp, &hit->normal);
            mathMulTransVec(matPtrs[i], &tmp, &hit->normal);
        }
    }
}

void hdInit(HdEnt* stack)
{
    hdStack = stack;
}
