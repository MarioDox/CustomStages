#pragma once

#include <StaticAssert.h>
#include <st/st_melee.h>
#include <types.h>
#include "gr_cell.h"
#include "gr_cellsearch.h"
#include "gr_cell_bullet.h"
#include "gr_cell_laser.h"
#include "gr_cell_hind.h"

const float BGM_PLAY_OFFSET_FRAME = 0.0f;
const float BGM_VOLUME = 1.0f;
const float SCROLL_DIR = 0.0f;
const float POKETRAINER_Z = 0.0f;

// 8 is enough to make sure even when shooting the back wall the effect will still play without being cut off
#define bulletAmount 8
#define laserAmount 3

enum stageState {
    IDLE,
    WAIT_PATH,
    WAIT_HELI,
    WAIT_LASER,
};

enum patternGimmickType {
    PATH,
    HELI,
    LASER,
};

class stCell : public stMelee {
protected:
    grCell* grCellRef;
    grCellHind* grCellHindRef;
    grCellSearch* search;
    grCellLaser* lasers[laserAmount];
    int currentPatternIdx;
    int alarmSFX;
    int ambientSFX;
    float stateTimer;
    patternGimmickType currentPatternSet[3];
    bool alarmRequested;
    bool isAlarm;
    bool didHeli;
    bool isGo;
public:
    stageState state;
    stCell() : stMelee("stCell", Stages::Final) {
        alarmRequested = false;
        isAlarm = false;
        grCellRef = NULL;
        search = NULL;
        for (int i=0;i<laserAmount;i++) {
            lasers[i] = NULL;
        }
        lasers[0] = NULL;
        lasers[1] = NULL;
        lasers[2] = NULL;
        state = IDLE;
        currentPatternSet[0] = PATH;
        currentPatternSet[1] = PATH;
        currentPatternSet[2] = PATH;
        currentPatternIdx = 0;
        stateTimer = 0.0;
        alarmSFX = NULL;
        ambientSFX = NULL;
        didHeli = false;
        isGo = false;
    };
    static stCell* create();

    virtual void createObj();
    virtual bool loading();
    virtual void update(float deltaFrame);
    virtual void notifyEventInfoGo();

    virtual void randomizePattern(bool isFirst);
    virtual void nextPattern();

    virtual GXColor getFinalTechniqColor();
    virtual bool isBamperVector();
    virtual ~stCell() { this->releaseArchive(); };

    //STATIC_CHECK(sizeof(stCell) == 0x1D8)
};