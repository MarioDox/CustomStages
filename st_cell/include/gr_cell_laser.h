#pragma once

#include <gr/gr_madein.h>
#include <ft/ft_manager.h>
#include "gr_cell.h"
#include "gr_cellsearch.h"

#define MAX_PLAYERS 7
#define laserAmount 3

class grCellLaser : public grMadein {
protected:
    snd3DGenerator soundGenerator;
    int caughtPlayers[MAX_PLAYERS*2];
    int decidedVictim;
    float stateTimer;
    char alarmedCount;
public:
    enum laserState {
        OFF,
        SPAWN,
        IDLE,
        TRIGGERED_START,
        TRIGGERED,
        DISAPPEAR
    };
    laserState state;
    grCell* grCellRef;
    grCellSearch* grCellSearchRef;
    grCellLaser** laserList;
    Vec3f initialPos;
    grCellLaser(const char* taskName) : grMadein(taskName)
    {
        __memfill(&caughtPlayers,false,sizeof(caughtPlayers));
        stateTimer = 0.0;
        state = OFF;
        // grCellSearchRef = NULL;
        grCellRef = NULL;
        initialPos = Vec3f(0,0,0);
        decidedVictim = NULL;
        alarmedCount = 0;
    };
    virtual void update(float deltaFrame);
    virtual void updateState(float deltaFrame);
    virtual void updateDetect(float deltaFrame);
    virtual void performAlarm();
    virtual void startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType);
    virtual ~grCellLaser(){
    };

    virtual bool checkDead(int playerNo);
    virtual void addAlarmingPlayer(int* group, int taskId);
    virtual void onInflictEach(soCollisionLog* collisionLog, float power);
    virtual void spawnLaser();
    virtual void setupAttack();
    static grCellLaser* create(int mdlIndex, const char* tgtNodeName, const char* taskName);

};
