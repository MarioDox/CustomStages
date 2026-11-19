#pragma once

#include <StaticAssert.h>
#include <types.h>

struct stCellData {
    float patternInitialTimerMin;       //300
    float patternInitialTimerMax;       //420
    float patternTimerMin;              //180
    float patternTimerMax;              //300
    float searchLightBaseSize;          //16
    float searchLightAreaOffsetX;       //0
    float searchLightAreaOffsetY;       //-2.5
    float searchLightSizeMult;          //1
    float searchLightSpeed;             //1
    float searchLightDetectSizeMult;    //1.8
    float searchLightDetectDuration;    //180
    float searchLightGracePeriod;       //80
    float searchLightAttackDuration;    //120
    float searchLightAttackFirerate;    //3
    float searchLightAttackFireSpread;  //15
    float searchLightAttackFinalFrame;  //15
    float heliCameraDist1;              //300
    float heliCameraDist2;              //220
    float heliMinX;                     //-100
    float heliMaxX;                     //100
    float heliSearchSpeed;              //9
    float heliWindGFXScale;             //1.2
    float heliWindGFXXPosMult;          //0.25
    float heliStartSFXFrame;            //105
    float heliLoopSFXFrame;             //480
    float heliStartAnimEndFrame;        //600
    float laserSpawnFrames;             //60
    float laserRandX;                   //25
    float laserBaseY;                   //5
    float laserAddRandY;                //25
    float laserDuration;                //300
    float laserDisappearFrames;         //30
    float laserTriggerFrames;           //30
    float laserTriggerAlarmDelay;       //45
    float laserHitboxOffsetX;           //0
    float laserHitboxOffsetY;           //0
    float laserHitboxOffsetZ;           //180
    float laserHitboxSize;              //1
    float bulletSize;                   //1.5
    int bulletPower;                    //2
    int bulletKBG;                      //0
    int bulletBKB;                      //32
    int bulletAngle;                    //361
    int bulletFinalKBG;                 //220
    int bulletFinalBKB;                 //64
    int bulletFinalAngle;               //90
    float bulletSDIMult;                //0.75
    float bulletHitboxOffsetPosX;       //0
    float bulletHitboxOffsetPosY;       //0
    float bulletHitboxOffsetPosZ;       //0
    float bulletHitboxAboveOffsetPosX;  //0
    float bulletHitboxAboveOffsetPosY;  //400
    float bulletHitboxAboveOffsetPosZ;  //40
    float bulletStartPosX;              //0
    float bulletStartPosY;              //0
    float bulletStartPosZ;              //-15
    float bulletAboveStartPosX;         //0
    float bulletAboveStartPosY;         //-5
    float bulletAboveStartPosZ;         //30
    float searchDetectFrames;           //30
    float searchEscapeFrames;           //30
};

// helper enum
enum CellSndId {
    snd_ambient = 0x1CFC,
    snd_hind_in = 0x1CFA,
    snd_hind_loop = 0x1CFB,
    snd_hind_out = 0x1D0D,
    snd_siren = 0x1D03,
    snd_gun = 0x1D01,
    snd_bound1 = 0x1CFE,
    snd_bound2 = 0x1CFF,
    snd_bound3 = 0x1D00,
    snd_bound4 = 0x1D05,
    snd_bound5 = 0x1D06,
    snd_bound6 = 0x1D07,
    snd_infrared = 0x1D02,
};
// static_assert(sizeof(stCellData) == 84, "Class is the wrong size!");