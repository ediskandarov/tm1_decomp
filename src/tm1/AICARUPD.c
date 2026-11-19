#include "common.h"

#include <RAND.H>

#include "tm1/CAR.H"
#include "tm1/DIFFICUL.H"
#include "tm1/INTERACT.H"
#include "tm1/SHELL.H"
#include "tm1/UA.H"

#include "tm1/AICARUPD.H"

// clang-format off
static CarFrontProfile IceCreamProfileFront = {
    33, 0, 65, 2
};
static CarRearProfile IceCreamProfileRear = {
    0, 90, 10, 0, 0, 0
};
static CarFrontProfile TaxiProfileFront = {
    70, 0, 30, 0
};
static CarRearProfile TaxiProfileRear = {
    10, 0, 0, 90, 0, 0
};
static CarFrontProfile SemiProfileFront = {
    20, 80, 0, 0
};
static CarRearProfile SemiProfileRear = {
    20, 70, 10, 0, 0, 0
};
static CarFrontProfile ImpalaProfileFront = {
    20, 20, 0, 0
};
static CarRearProfile ImpalaProfileRear = {
    35, 5, 60, 0, 0, 0
};
static CarFrontProfile MonsterProfileFront = {
    10, 90, 0, 0
};
static CarRearProfile MonsterProfileRear = {
    0, 80, 0, 20, 0, 0
};
static CarFrontProfile DuneBuggyProfileFront = {
    90, 5, 5, 0
};
static CarRearProfile DuneBuggyProfileRear = {
    50, 20, 0, 0, 0, 30
};
static CarFrontProfile PoliceProfileFront = {
    10, 90, 0, 0
};
static CarRearProfile PoliceProfileRear = {
    0, 20, 0, 0, 80, 0
};
static CarFrontProfile MadMaxProfileFront = {
    100, 0, 0, 0
};
static CarRearProfile MadMaxProfileRear = {
    20, 0, 20, 0, 30, 30
};
static CarFrontProfile HarleyProfileFront = {
    80, 20, 0, 0
};
static CarRearProfile HarleyProfileRear = {
    0, 0, 10, 0, 0, 90
};
static CarFrontProfile HumveeProfileFront = {
    90, 0, 5, 5
};
static CarRearProfile HumveeProfileRear = {
    0, 0, 100, 0, 0, 0
};
static CarFrontProfile VetteProfileFront = {
    100, 0, 0, 0
};
static CarRearProfile VetteProfileRear = {
    0, 0, 0, 100, 0, 0
};
static CarFrontProfile LamborghiniProfileFront = {
    95, 0, 0, 5
};
static CarRearProfile LamborghiniProfileRear = {
    90, 0, 10, 0, 0, 0
};
static CarFrontProfile BossProfileFront = {
    15, 15, 35, 35
};
static CarRearProfile BossProfileRear = {
    20, 10, 25, 20, 10, 15
};
static CarFrontProfile Level1ProfileFront = {
    60, 20, 0, 0
};
static CarRearProfile Level1ProfileRear = {
    50, 0, 0, 30, 0, 0
};

static s32 IceCreamSpecialChance = 35;
static s32 IceCreamForeChance = 60;
static s32 IceCreamAftChance = 90;

static s32 TaxiSpecialChance = 90;
static s32 TaxiForeChance = 10;
static s32 TaxiAftChance = 75;

static s32 PoliceSpecialChance = 35;
static s32 PoliceForeChance = 25;
static s32 PoliceAftChance = 50;

static s32 VetteSpecialChance = 90;
static s32 VetteForeChance = 30;
static s32 VetteAftChance = 10;

static s32 MonsterSpecialChance = 50;
static s32 MonsterForeChance = 20;
static s32 MonsterAftChance = 50;

static s32 SemiSpecialChance = 35;
static s32 SemiForeChance = 70;
static s32 SemiAftChance = 10;

static s32 DuneBuggySpecialChance = 50;
static s32 DuneBuggyForeChance = 50;
static s32 DuneBuggyAftChance = 90;

static s32 LamborghiniSpecialChance = 95;
static s32 LamborghiniForeChance = 30;
static s32 LamborghiniAftChance = 80;

static s32 HumveeSpecialChance = 60;
static s32 HumveeForeChance = 60;
static s32 HumveeAftChance = 60;

static s32 HarleySpecialChance = 35;
static s32 HarleyForeChance = 80;
static s32 HarleyAftChance = 60;

static s32 MadMaxSpecialChance = 80;
static s32 MadMaxForeChance = 50;
static s32 MadMaxAftChance = 20;

static s32 ImpalaSpecialChance = 75;
static s32 ImpalaForeChance = 80;
static s32 ImpalaAftChance = 80;

static s32 BossSpecialChance = 100;
static s32 BossForeChance = 75;
static s32 BossAftChance = 75;

void AICarUpdateAttackProfile(AICar* car)
{
    if (car->unk40 != 0) {
        if ((s16)car->weap.fireDelay < 2) {
            AIPickAttackWeapon(car);
            if (car->weap.cur == WEAPON_NONE) {
                car->weap.fireDelay = rand() % 20 + 10;
            }
        }
        UpdateWeapons(car, 0);
    } else {
        car->weap.cur = WEAPON_NONE;
    }
}

void AIPickAttackWeapon(AICar* car)
{
    s16 lvl;

    car->weap.cur = WEAPON_NONE;
    lvl = car->weap.ammo[WEAPON_SPECIAL];
    if (lvl >= 6 && AIInSpecialParameters(car)) {
        if (rand() % 24 < car->weap.ammo[WEAPON_SPECIAL]) {
            car->weap.cur = WEAPON_SPECIAL;
            return;
        }
    }
    switch (car->direction) {
    case DIRECTION_FRONT:
        if (rand() % 100 + 1 < (s16)car->weap.foreChance) {
            AICarChooseForeWeapon(car);
        } else {
            car->weap.cur = WEAPON_NONE;
        }
        break;
    case DIRECTION_REAR:
        if (rand() % 100 + 1 < (s16)car->weap.aftChance) {
            AICarChooseAftWeapon(car);
        } else {
            car->weap.cur = WEAPON_NONE;
        }
        break;
    }
}

u8 AIInSpecialParameters(AICar* car)
{
    s32 value;

    value = car->uaIndex;
    switch (value) {
    case UA_ICE_CREAM_TRUCK:
    case UA_TAXI_CAB:
    case UA_SEMI_TRUCK:
    case UA_LAMBORGHINI:
    case UA_HARLEY:
    case UA_DUNE_BUGGY:
    case UA_CORVETTE:
        return car->direction == DIRECTION_FRONT;
    case UA_BOSS_CAR:
        if (car->direction == DIRECTION_FRONT) {
            return 1;
        }
        if (car->unk44 != 0) {
            return 1;
        }
        return car->direction == DIRECTION_REAR;
    default:
        value = car->direction;
        if (value != DIRECTION_FRONT) {
            return 0;
        }
        return car->unk44 != 0;
    case UA_POLICE_CAR:
        return car->unk44 != 0;
    }
}

void AICarSetDefaultWeapons(AICar* car)
{
    CarWeap* w;
    s32 special;
    s32 fore;
    s32 aft;
    s32 diff;
    s32 kind;

    shellGetCurrentLevel();
    kind = car->uaIndex;
    w = &car->weap;
    switch (kind) {
    case UA_BOSS_CAR:
        special = BossSpecialChance;
        fore = BossForeChance;
        aft = BossAftChance;
        w->foreProfile = &BossProfileFront;
        w->aftProfile = &BossProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_ICE_CREAM_TRUCK:
        special = IceCreamSpecialChance;
        fore = IceCreamForeChance;
        aft = IceCreamAftChance;
        w->foreProfile = &IceCreamProfileFront;
        w->aftProfile = &IceCreamProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_TAXI_CAB:
        special = TaxiSpecialChance;
        fore = TaxiForeChance;
        aft = TaxiAftChance;
        w->foreProfile = &TaxiProfileFront;
        w->aftProfile = &TaxiProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_SEMI_TRUCK:
        special = SemiSpecialChance;
        fore = SemiForeChance;
        aft = SemiAftChance;
        w->foreProfile = &SemiProfileFront;
        w->aftProfile = &SemiProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_MONSTER_TRUCK:
        special = MonsterSpecialChance;
        fore = MonsterForeChance;
        aft = MonsterAftChance;
        w->foreProfile = &MonsterProfileFront;
        w->aftProfile = &MonsterProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_POLICE_CAR:
        special = PoliceSpecialChance;
        fore = PoliceForeChance;
        aft = PoliceAftChance;
        w->foreProfile = &PoliceProfileFront;
        w->aftProfile = &PoliceProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_LAMBORGHINI:
        special = LamborghiniSpecialChance;
        fore = LamborghiniForeChance;
        aft = LamborghiniAftChance;
        w->foreProfile = &LamborghiniProfileFront;
        w->aftProfile = &LamborghiniProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_HUMVEE:
        special = HumveeSpecialChance;
        fore = HumveeForeChance;
        aft = HumveeAftChance;
        w->foreProfile = &HumveeProfileFront;
        w->aftProfile = &HumveeProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_HARLEY:
        special = HarleySpecialChance;
        fore = HarleyForeChance;
        aft = HarleyAftChance;
        w->foreProfile = &HarleyProfileFront;
        w->aftProfile = &HarleyProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_DUNE_BUGGY:
        special = DuneBuggySpecialChance;
        fore = DuneBuggyForeChance;
        aft = DuneBuggyAftChance;
        w->foreProfile = &DuneBuggyProfileFront;
        w->aftProfile = &DuneBuggyProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_LOW_RIDER:
        special = ImpalaSpecialChance;
        fore = ImpalaForeChance;
        aft = ImpalaAftChance;
        w->foreProfile = &ImpalaProfileFront;
        w->aftProfile = &ImpalaProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_CORVETTE:
        special = VetteSpecialChance;
        fore = VetteForeChance;
        aft = VetteAftChance;
        w->foreProfile = &VetteProfileFront;
        w->aftProfile = &VetteProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    case UA_MAD_MAX:
        special = MadMaxSpecialChance;
        fore = MadMaxForeChance;
        aft = MadMaxAftChance;
        w->foreProfile = &MadMaxProfileFront;
        w->aftProfile = &MadMaxProfileRear;
        w->specialChance = special;
        w->foreChance = fore;
        w->aftChance = aft;
        break;
    default:
        w->foreProfile = &Level1ProfileFront;
        w->aftProfile = &Level1ProfileRear;
        w->specialChance = 0;
        w->foreChance = 10;
        w->aftChance = 10;
        break;
    }
    diff = uaGetDifficulty();
    /* Original bug: this tests the function address. */
    if (shellGetCurrentLevel == 0 && diff < DIFFICULTY_HARD) {
        w->foreChance = (s16)w->foreChance / 2;
        w->aftChance = (s16)w->aftChance / 2;
        if ((s16)w->foreChance < 10) {
            w->foreChance = 10;
        }
        if ((s16)w->aftChance < 10) {
            w->aftChance = 10;
        }
    }
    if (diff == DIFFICULTY_EASY) {
        w->specialChance = w->specialChance - 15;
        if ((s16)w->specialChance < 10) {
            w->specialChance = 10;
        }
        w->foreChance = w->foreChance - 10;
        if ((s16)w->foreChance < 10) {
            w->foreChance = 10;
        }
        w->aftChance = w->aftChance - 10;
        if ((s16)w->aftChance < 10) {
            w->aftChance = 10;
        }
    } else if (diff == DIFFICULTY_HARD) {
        w->specialChance = w->specialChance + 20;
        w->foreChance = w->foreChance + 10;
        if ((s16)w->foreChance >= 96) {
            w->foreChance = 95;
        }
        w->aftChance = w->aftChance + 10;
        if ((s16)w->aftChance >= 96) {
            w->aftChance = 95;
        }
    }
}

void AICarChooseForeWeapon(AICar* car)
{
    s32 sum = 0;
    s16 roll = rand() % 100;
    s32 i = WEAPON_FIRE_MISSILE;
    s32* p = &car->weap.foreProfile->fire;

    do {
        sum += *p;
        if (roll < sum) {
            car->weap.cur = i;
            return;
        }
        i++;
        p++;
    } while (i <= WEAPON_POWER_MISSILE);
}

void AICarChooseAftWeapon(AICar* car)
{
    s32 sum = 0;
    s16 roll = rand() % 100;
    s32* probs = &car->weap.aftProfile->rearMissile;
    s32 i = WEAPON_REAR_MISSILE;

    do {
        sum += probs[i - WEAPON_REAR_MISSILE];
        if (roll < sum) {
            car->weap.cur = i;
            return;
        }
        i++;
    } while (i <= WEAPON_SPIKE);
}
