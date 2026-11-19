#include "common.h"

#include "tm1/SMOOTH.H"

void BoundVector(VECTOR3* v)
{
    while (v->vx >= 2048) {
        v->vx = v->vx - 4096;
    }
    while (v->vx < -2048) {
        v->vx = v->vx + 4096;
    }
    while (v->vy >= 2048) {
        v->vy = v->vy - 4096;
    }
    while (v->vy < -2048) {
        v->vy = v->vy + 4096;
    }
    while (v->vz >= 2048) {
        v->vz = v->vz - 4096;
    }
    while (v->vz < -2048) {
        v->vz = v->vz + 4096;
    }
}

void BoundSVector(SVECTOR* v)
{
    while (v->vx >= 2048) {
        v->vx = v->vx - 4096;
    }
    while (v->vx < -2048) {
        v->vx = v->vx + 4096;
    }
    while (v->vy >= 2048) {
        v->vy = v->vy - 4096;
    }
    while (v->vy < -2048) {
        v->vy = v->vy + 4096;
    }
    while (v->vz >= 2048) {
        v->vz = v->vz - 4096;
    }
    while (v->vz < -2048) {
        v->vz = v->vz + 4096;
    }
}

void BoundAngle(s32* a)
{
    while (a[0] >= 2048) {
        a[0] = a[0] - 4096;
    }
    while (a[0] < -2048) {
        a[0] = a[0] + 4096;
    }
}

void BoundSAngle(s16* a)
{
    while (a[0] >= 2048) {
        a[0] = a[0] - 4096;
    }
    while (a[0] < -2048) {
        a[0] = a[0] + 4096;
    }
}

s32 SmoothAngleValue(s32 from, s32 to, s32 pct)
{
    s32 r;

    if (from < -1024 && to > 1024) {
        to = to - 4096;
        r = (from * pct + to * (100 - pct)) / 100;
        BoundAngle(&r);
    } else if (from > 1024 && to < -1024) {
        to = to + 4096;
        r = (from * pct + to * (100 - pct)) / 100;
        BoundAngle(&r);
    } else {
        r = (from * pct + to * (100 - pct)) / 100;
    }
    return r;
}

s32 SmoothValue(s32 from, s32 to, s32 pct)
{
    return (from * pct + to * (100 - pct)) / 100;
}

s32 SmoothValue2(s32 a, s32 b, s32 c)
{
    return SmoothValue(c, b, 99 - (c - a) * 100 / ((b - a) * 10));
}
