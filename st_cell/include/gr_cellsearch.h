#pragma once
#include "mt_tween.h"
#include "gr_cell_bullet.h"
#include <gr/gr_yakumono.h>

static const Vec3f pathPoints[33] = {
    Vec3f(99.2,99.2,0.0), Vec3f(99.2,99.2,0.0), Vec3f(77.92,101.6,0.0),
    Vec3f(-69.44,61.76,0.0), Vec3f(-81.6,38.4,0.0), Vec3f(-93.76,15.04,0.0),
    Vec3f(-90.51,8.84,0.0), Vec3f(-77.76,4.16,0.0), Vec3f(-65.01,-0.52,0.0),
    Vec3f(-75.8,35.22,0.0), Vec3f(-53.16,35.48,0.0), Vec3f(-30.51,35.73,0.0),
    Vec3f(-46.0,2.31,0.0), Vec3f(-23.36,2.56,0.0), Vec3f(-0.72,2.81,0.0),
    Vec3f(-14.78,33.35,0.0), Vec3f(3.68,33.6, 0.0), Vec3f(22.14,33.85,0.0),
    Vec3f(9.12,3.04,0.0), Vec3f(24.48,3.2,0.0), Vec3f(39.84,3.36,0.0),
    Vec3f(34.5,33.73,0.0), Vec3f(52.96,33.6,0.0), Vec3f(71.42,33.47,0.0),
    Vec3f(60.48,-3.84,0.0), Vec3f(76.48,5.12,0.0), Vec3f(92.48,14.08,0.0),
    Vec3f(94.08,15.68,0.0), Vec3f(80.64,40.96,0.0), Vec3f(67.2,66.24,0.0),
    Vec3f(-64.8,102.56,0.0), Vec3f(-98.56,98.88,0.0), Vec3f(-98.56,98.88,0.0),
};

static const f32 pathTime[11] = {
    3.0, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 3.0, 3.0,
};

#define MAX_PLAYERS 7
#define bulletAmount 8

class grCellSearch : public grYakumono {
protected:
    enum searchFlag {
        Flag_Off,
        Flag_Found,
        Flag_Searching,
    };
    soAreaData areaData;
    soSet<soAreaData> areaInit;
    ykAreaData areaInfo;
    int targetPlayer;   // taskId
    float stateTimer;
    Tween sizeTween;
    Tween pathTween;
    snd3DGenerator soundGenerator;
    int dangerZone;
    // double max players for subfighters
    float detectTime[MAX_PLAYERS*2];
    float escapeTime[MAX_PLAYERS*2]; // it has a specific one when outside the area
    float escapeGrace;
    Vec3f currentPoints[4];
    float waitTimeForDetect;
    float searchMaxStep;
    float shootDelay;
    bool* alarmRequestWork;
    searchFlag lightFlag;
    char targetPlayerNo;    // to avoid calculations every frame
    bool detectedPlayer[MAX_PLAYERS*2];     // bool to tell if this fighter has been in the circle on the previous frame
    char currentPathIdx;
    bool pathBackwards;
    bool isAbove;
public:
    enum searchState {
        State_Init,
        State_Move,
        State_MovePath,
        State_MoveHeli,
        State_End,
        State_Detect,
        State_AttackStart,
        State_Attack,
        State_AttackEnd,
        State_Cooldown,
    };
    Vec3f searchPos;
    Vec3f searchTgtPos;
    Vec3f searchSourcePos;
    grCellBullet* bulletList[bulletAmount];
    searchState lightState;
    grCellSearch(const char* taskName) : grYakumono(taskName)
    {
        lightState = State_Init;
        lightFlag = Flag_Searching;
        targetPlayer = NULL;
        stateTimer = 0;
        sizeTween.init();
        pathTween.init();
        __memfill(&detectedPlayer,false,sizeof(detectedPlayer));
        __memfill(&detectTime,0.0,sizeof(detectTime));
        waitTimeForDetect = 0.0;
        currentPathIdx = 0;
        __memfill(&currentPoints,0.0,sizeof(currentPoints));
        pathBackwards = false;
        searchPos = Vec3f(0,0,200);
        searchTgtPos = Vec3f(0,0,200);
        searchSourcePos = Vec3f(0,0,0);
        isAbove = false;
        searchMaxStep = 0.0;
        __memfill(&bulletList,NULL,sizeof(bulletList));
        shootDelay = 0.0;
        alarmRequestWork = NULL;
        dangerZone = -1;
        escapeGrace = 0.0;
    };
    virtual ~grCellSearch(){};
    virtual void renderOpa();
    virtual void renderScnLight();
    virtual void startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType);
    virtual void update(float deltaFrame);
    virtual void updateActive(float deltaFrame);
    virtual void updateArea(float deltaFrame);
    virtual void updatePath(float deltaFrame);
    virtual void updateHeli(float deltaFrame);
    virtual void updateTrack(float deltaFrame);
    virtual void updateShoot(float deltaFrame);
    virtual void updateSearch(float deltaFrame);
    virtual void updateAiDangerZone();
    virtual bool checkDead(int taskId);
    virtual void addAlarmingPlayer(int* group, int playerNo); 
    virtual Vec3f getPlayerPosition(int taskId);
    virtual void loadPathSegment(int idx);
    virtual void resetSearch();
    virtual void renderProjection();
    virtual void adjustModelMatrix();
    virtual void setAlarmRequestWork(bool* work) { alarmRequestWork = work; };
    virtual void startMode(int idx);
    virtual void startDetect(int taskId);
    virtual void onGimmickEvent(soGimmickEventArgs* eventInfo, int* taskId);
    virtual int getTaskIdFromPlayerNo(int playerNo);


    static grCellSearch* create(int mdlIndex, const char* tgtNodeName, const char* taskName);

};
