#include "gr_cellsearch.h"
#include "st_cell_data.h"
#include <ec/ec_mgr.h>
#include <memory.h>
#include <st/st_melee.h>
#include <ft/ft_manager.h>
#include <ft/ft_external_value_accesser.h>
#include <ai/ai_mgr.h>
#include <mt/mt_prng.h>
#include <mt/mt_spline.h>
#include <mt/mt_vector.h>
#include <mt/mt_trig.h>
#include <nw4r/math/math_types.h>
#include <nw4r/g3d/g3d_camera.h>
#include <nw4r/g3d/g3d_scnroot.h>
using namespace nw4r;

grCellSearch* grCellSearch::create(int mdlIndex, const char* tgtNodeName, const char* taskName)
{
    grCellSearch* ground = new (Heaps::StageInstance) grCellSearch(taskName);
    ground->setupMelee();
    ground->setMdlIndex(mdlIndex);
    ground->setTgtNode(tgtNodeName);
    ground->makeCalcuCallback(1, Heaps::StageInstance);
    // set callback to flags4 + flags8 = 12 (1100), allows scale + rotTrans (matrix), check grCalcWorldCallBack::ExecCallbackA()
    // madein uses 7 (0111), but that allows only pos,rot,scale and not matrix
    ground->setCalcuCallbackRoot(12);
    return ground;
}

void grCellSearch::startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType)
{
    stCellData* stageData = (stCellData*)m_stageData;
    grYakumono::startup(archive, unk1, layerType);
    areaData.set(soAreaInstance::Shape_Circle, gfArea::Stage_Group_Gimmick_Normal, 0, 0, 0, 0, Vec2f(stageData->searchLightAreaOffsetX,stageData->searchLightAreaOffsetY), Vec2f(stageData->searchLightBaseSize,stageData->searchLightBaseSize));
    setAreaGimmick(&areaData, &areaInit, &areaInfo, false);
    stTrigger* trigger = g_stTriggerMng->createTrigger(Gimmick::Area_Common,-1);
    trigger->setObserveYakumono(m_yakumono);
    disableArea();
    setVisibility(false);
}


void grCellSearch::update(float deltaFrame)
{
    grGimmick::update(deltaFrame);
    updateSearch(deltaFrame);
    // if (!m_isUpdate) { return; }
    waitTimeForDetect-=deltaFrame;
    if (waitTimeForDetect < 0) {
        waitTimeForDetect = 0;
    }
    updateArea(deltaFrame);
    updateActive(deltaFrame);
    updateAiDangerZone();
}

// copied from grMetalGearSearch, with additional detect time features
// uses a rect2D to detect fighters
// not an actual area
void grCellSearch::updateArea(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    escapeGrace-=deltaFrame;
    if (escapeGrace < 0) {
        escapeGrace = 0;
    }
    if (lightState != State_MovePath && lightState != State_MoveHeli && lightState != State_Detect) { return; }
    // float radius = sizeTween.getValue()*stageData->searchLightBaseSize;
    // areaData.m_range = Vec2f(radius,radius);
    
    int alarmingTasks[MAX_PLAYERS*2] = {};
    bool alarmingPlayers[MAX_PLAYERS*2] = {};
    int alarmedCount = 0;
    // if the detectedPlayer flag was on, then we increase the player's detectTime
    // if past its window, add them to the caught list
    // if not detected, we decrease their escape window instead

    for (int i=0;i<MAX_PLAYERS*2;i++) {
        alarmingPlayers[i] = false;
        // work the escapeTime first!
        if (detectedPlayer[i] == false) {
            if (escapeGrace > 0) { continue; }      // grace period to make gimmick more consistent
            if (detectTime[i] <= 0) { continue; }
            escapeTime[i]+=deltaFrame;
            if (escapeTime[i] >= stageData->searchEscapeFrames) {
                detectTime[i] = 0.0;
            }
            continue;   // processed, we can skip!
        }
        // below is only if detected
        detectedPlayer[i] = false;
        if (checkDead(getTaskIdFromPlayerNo(i))) { continue; }
        detectTime[i] += deltaFrame;
        escapeTime[i] = 0;   // reset this if they're inside again!
        if (detectTime[i] >= stageData->searchDetectFrames) {
            alarmingPlayers[i] = true;
            addAlarmingPlayer(alarmingTasks,i);
            alarmedCount++;
        }
    }

    // don't do anything else if we already have found someone!
    if (lightFlag != Flag_Searching) { return; }
    if (alarmedCount <= 0) { return; }

    // get random player from caught ones
    // metalgear replicates randi locally (float to int conversion + randf)
    // we can just use it here directly
    int idx = randi(alarmedCount);
    if (idx < 0) { idx = 0; } // unnecessary?
    if (idx > alarmedCount - 1) {
        idx = alarmedCount - 1;
    }
    startDetect(alarmingTasks[idx]);
}

// helper function, to make code cleaner
int grCellSearch::getTaskIdFromPlayerNo(int playerNo)
{
    bool isSub = false;
    if (playerNo > MAX_PLAYERS) {
        isSub = true;
        playerNo/=2;
    }
    int entryId = g_ftManager->getEntryId(playerNo);
    Fighter* fighter;
    if (isSub) {
        fighter = g_ftManager->getSubFighter(entryId);
    } else {
        fighter = g_ftManager->getFighter(entryId,-1);
    }
    if (fighter == NULL) { return NULL; }
    return fighter->m_taskId;
}

void grCellSearch::addAlarmingPlayer(int* group, int playerNo)
{
    int taskId = getTaskIdFromPlayerNo(playerNo);
    if (taskId == NULL) { return; }
    for (int i=0;i<MAX_PLAYERS*2;i++) {
        if (group[i] != NULL) { continue; }
        if (group[i] == taskId) { return; }
        group[i] = taskId;
        return;
    }
}


void grCellSearch::onGimmickEvent(soGimmickEventArgs* eventInfo, int* taskId)
{
    int entryID = g_ftManager->getEntryIdFromTaskId(*taskId, NULL);
    Fighter* fighter = g_ftManager->getFighter(entryID, -1);
    int playerNo = g_ftManager->getPlayerNo(entryID);
    if (fighter == NULL) { return; }
    if (checkDead(*taskId)) { return; }
    Fighter* subFighter = g_ftManager->getSubFighter(entryID);
    if (fighter == subFighter) {
        playerNo*=2;
        fighter = subFighter;
    }
    waitTimeForDetect = 60.0;   // stop the light!
    detectedPlayer[playerNo] = true;
}


// do these even work
void grCellSearch::updateAiDangerZone()
{
    stCellData* stageData = (stCellData*)m_stageData;
    if (lightState != State_MovePath && lightState != State_MoveHeli && lightState != State_Detect) {
        if (dangerZone != -1) {
            g_aiMgr->delDangerZone(dangerZone);
            dangerZone = -1;
        }
    } else {
        Vec2f aiAreaMin = Vec2f(getPos().m_x-stageData->searchLightBaseSize,getPos().m_y+stageData->searchLightBaseSize);
        Vec2f aiAreaMax = Vec2f(getPos().m_x+stageData->searchLightBaseSize,getPos().m_y-stageData->searchLightBaseSize);
        dangerZone = g_aiMgr->setDangerZone(&aiAreaMin,&aiAreaMax,dangerZone,0,0xFF);   //0xFF makes the CPUs attempt to dodge it more than normal, idk
    }
}

// start detect phase using taskId
void grCellSearch::startDetect(int taskId)
{
    stCellData* stageData = (stCellData*)m_stageData;
    targetPlayer = taskId;
    targetPlayerNo = g_ftManager->getPlayerNo(g_ftManager->getEntryIdFromTaskId(taskId,NULL));
    lightFlag = Flag_Found;
    lightState = State_Detect;
    stateTimer = stageData->searchLightDetectDuration;
    sizeTween.start(sizeTween.getValue(),stageData->searchLightDetectSizeMult,0.5,0.0);
    escapeGrace = stageData->searchLightGracePeriod;
    // reset all times now as we need to check for the target's escape
    for (int i=0;i<MAX_PLAYERS;i++) {
        detectTime[i] = 0.0;
        escapeTime[i] = 0.0;
    }
    *alarmRequestWork = true;
}

void grCellSearch::startMode(int idx) {
    stCellData* stageData = (stCellData*)m_stageData;
    enableArea();
    lightFlag = Flag_Searching;
    sizeTween.start(0.01,stageData->searchLightSizeMult,0.5,0.0);
    Vec2f aiAreaMin = Vec2f(-stageData->searchLightBaseSize,-stageData->searchLightBaseSize);
    Vec2f aiAreaMax = Vec2f(stageData->searchLightBaseSize,stageData->searchLightBaseSize);
    int randInt;
    if (idx == 0) {          // path mode
        randInt = randi(2);
        if (randInt == 0) {
            pathBackwards = false;
            currentPathIdx = 0;
        } else {
            pathBackwards = true;
            currentPathIdx = 9;
        }
        loadPathSegment(currentPathIdx);
        lightState = State_MovePath;
    } else {                    // helicopter sweep
        setVisibility(true);    // light beam is visible only here
        lightState = State_MoveHeli;
        pathTween.start(stageData->heliMinX,stageData->heliMaxX,stageData->heliSearchSpeed,0.0);
        isAbove = true;
    }
}

// constructed like grMetalgearSearch's, but uses patterns and behaviors we want
void grCellSearch::updateActive(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    if (stageData == NULL) { return; }
    stateTimer-=deltaFrame;
    if (stateTimer < 0) {
        stateTimer = 0;
    }
    Vec3f tempVec;
    int tempInt;
    switch (lightState)
    {
        case State_Init:
            resetSearch();
            break;
        case State_MovePath:
            updatePath(deltaFrame);
            break;
        case State_MoveHeli:
            updateHeli(deltaFrame);
            if (pathTween.isEnd) {
                lightState = State_End;
                sizeTween.start(sizeTween.getValue(),0.01,0.5,0.0);
            }
            break;
        case State_End:
            sizeTween.advance(deltaFrame);
            if (sizeTween.isEnd) {
                resetSearch();
            }
            break;
        case State_Detect:
            // searchMaxStep = stageData->maxStepDetect;
            // searchMaxStep = 1.0;
            updateTrack(deltaFrame);
            if (checkDead(targetPlayer) || escapeTime[targetPlayerNo] >= stageData->searchEscapeFrames) {
                // OSReport("targetplayer %x, checkDead %x, escapeTime %f\n",targetPlayer, checkDead(targetPlayer),escapeTime[targetPlayerNo]);
                lightState = State_AttackEnd;
                sizeTween.start(sizeTween.getValue(),0.01,0.5,0.0);
            } else if (stateTimer == 0) {
                lightState = State_AttackStart;
                stateTimer = 0.0;
            }
            break;
        case State_AttackStart:
            // updateTrack(deltaFrame);
            lightState = State_Attack;
            stateTimer = stageData->searchLightAttackDuration;
            shootDelay = stageData->searchLightAttackFirerate;
            break;
        case State_Attack:
            updateTrack(deltaFrame);
            updateShoot(deltaFrame);
            if (stateTimer == 0 || checkDead(targetPlayer)) {
                lightState = State_AttackEnd;
                sizeTween.start(sizeTween.getValue(),0.01,0.5,0.0);
            }
            break;
        case State_AttackEnd:
            sizeTween.advance(deltaFrame);
            if (sizeTween.isEnd) {
                resetSearch();
                *alarmRequestWork = false;
            }
            break;
        default:
            break;
    }
}

// track movement
// it doesn't always follow you, unlike what you'd expect
void grCellSearch::updateTrack(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    sizeTween.advance(deltaFrame);
    // check the distance from the current pos to the end pos the light needs to be
    // if we're close, then focus on our target again
    Vec3f dist = searchPos-searchTgtPos;
    if (dist.lengthSq() < 3.0) {
        Vec3f targetPos = getPlayerPosition(targetPlayer);
        searchTgtPos = targetPos;
        if (isAbove) {
            searchTgtPos.m_y = 0.0;
        }
    }
    // recalculate the distance again just in case it changed
    dist = searchPos-searchTgtPos;
    if (dist.lengthSq() > 2.0) {
        searchMaxStep = stageData->searchLightSpeed;
    } else {
        searchMaxStep = 0.0;        // too close! no need to move!
    }
}

void grCellSearch::updateShoot(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    shootDelay-=deltaFrame;
    if (shootDelay < 0) {
        shootDelay = 0;
    }
    if (shootDelay == 0) {
        for (int i=0;i<bulletAmount;i++) {
            if (bulletList[i] == NULL || bulletList[i]->isActive) { continue; } // don't shoot what's already shot!
            Vec3f shootPos = searchPos;
            shootPos.m_x += stageData->searchLightAttackFireSpread*(randf()-0.5);
            bulletList[i]->isAbove = isAbove;
            if (!isAbove) {
                shootPos.m_y += stageData->searchLightAttackFireSpread*(randf()-0.5);
            } else {
                shootPos.m_z += stageData->searchLightAttackFireSpread*(randf()-0.5);
            }
            // if about to end, make the bullets deadlier
            if (stateTimer <= stageData->searchLightAttackFinalFrame) {
                bulletList[i]->isFinal = true;
            } else {
                bulletList[i]->isFinal = false;
            }
            bulletList[i]->setPos(&shootPos);
            bulletList[i]->shoot();
            shootDelay = stageData->searchLightAttackFirerate;
            break;
        }
    }
}

void grCellSearch::updatePath(float deltaFrame)
{
    // restartfx();
    sizeTween.advance(deltaFrame);
    // don't move if we detected something!
    if (waitTimeForDetect == 0) {
        pathTween.advance(deltaFrame);
    }
    Vec3f tempPos;
    mtBezierCurve(pathTween.getValue(),currentPoints,&tempPos); // this moves the actual spotlight
    searchTgtPos = tempPos;
    searchPos = tempPos; //temporarily
    // setPos(&tempPos);
    // OSReport("current value: %f | currentidx: %d | vec3(%f,%f,%f)\n",pathTween.getValue(),currentPathIdx,tempPos.m_x,tempPos.m_y,tempPos.m_z);
    if (pathTween.isEnd) {
        if (pathBackwards) {
            currentPathIdx--;
            if (currentPathIdx < 0) {
                currentPathIdx = 9;
                lightState = State_End;
                sizeTween.start(sizeTween.getValue(),0.01,0.5,0.0);
            }
        } else {
            currentPathIdx++;
            if (currentPathIdx >= 10) {
                currentPathIdx = 0;
                lightState = State_End;
                sizeTween.start(sizeTween.getValue(),0.01,0.5,0.0);
            }
        }
        loadPathSegment(currentPathIdx);
    }
}

void grCellSearch::updateHeli(float deltaFrame)
{
    // size adjusts always, even if not detecting
    sizeTween.advance(deltaFrame);
    // helicopter doesn't pause before detects
    pathTween.advance(deltaFrame);
    Vec3f tempPos = Vec3f(pathTween.getValue(),0.0,0.0);
    searchTgtPos = tempPos;
    searchPos = tempPos; //temporarily
    setPos(&tempPos);
    // OSReport("current value: %f | currentidx: %d | vec3(%f,%f,%f)\n",pathTween.getValue(),currentPathIdx,tempPos.m_x,tempPos.m_y,tempPos.m_z);
}

// save the points into our value
void grCellSearch::loadPathSegment(int idx)
{
    for (int i=0;i<4;i++) {
        // idx*3 as the 1st vec3f is the actual coordinates, the other two are handles
        // +1 to get the one ahead
        currentPoints[i] = pathPoints[idx*3+1+i];
    }
    if (pathBackwards) {
        pathTween.start(1.0,0.0,pathTime[idx],0.0);
    } else {
        pathTween.start(0.0,1.0,pathTime[idx],0.0);
    }
}

#define FLT_MIN          1.175494351e-38F
// copied from stMetalGear
void grCellSearch::updateSearch(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    if (stageData == NULL) { return; }
    Vec3f delta = searchTgtPos-searchPos;

    // this checks if the light is already on the spot, no need to move!

    const float EPS = 1e-5f;    // can this be replaced by 0?
    if (fabsf(delta.m_x) < EPS && fabsf(delta.m_y) < EPS && fabsf(delta.m_z) < EPS) {
        return;
    }

    float lengthSq = delta.m_x*delta.m_x+delta.m_y*delta.m_y+delta.m_z*delta.m_z;
    if (fabsf(lengthSq) > FLT_MIN) {
        lengthSq = lengthSq*rsqrtf(lengthSq);
    } else {
        lengthSq = 0.0;
    }
    delta.normalize(&delta);
    float step = lengthSq;
    if (step > searchMaxStep) {
        step = searchMaxStep;
    }
    searchPos+=delta*step*deltaFrame;
}

void grCellSearch::resetSearch()
{
    stCellData* stageData = (stCellData*)m_stageData;
    if (dangerZone != -1) {
        g_aiMgr->delDangerZone(dangerZone);
        dangerZone = -1;
    }
    setVisibility(false);
    for (int i=0;i<MAX_PLAYERS;i++) {
        detectTime[i] = 0.0;
        escapeTime[i] = 0.0;
    }
    lightState = State_Cooldown;
    lightFlag = Flag_Off;
    stateTimer = 300.0;
    isAbove = false;
    searchPos = Vec3f(0,-500,200);
    searchTgtPos = Vec3f(0,-500,200);
    sizeTween.start(0.01,stageData->searchLightSizeMult,0.5,0.0);
}

Vec3f grCellSearch::getPlayerPosition(int taskId)
{
    if (taskId == NULL) { return Vec3f(0,0,0); }
    Fighter* fighter = (Fighter*)getTask(taskId);
    if (fighter == NULL) { return Vec3f(0,0,0); }
    return ftExternalValueAccesser::getHipPos(fighter);
}

bool grCellSearch::checkDead(int taskId)
{
    if (taskId == NULL) { return true; }
    Fighter* fighter = (Fighter*)getTask(taskId);
    if (fighter == NULL) { return true; };
    soStatusModuleImpl& statusModule = (soStatusModuleImpl&)fighter->m_moduleAccesser->getStatusModule();
    soWorkManageModuleImpl& workModule = (soWorkManageModuleImpl&)fighter->m_moduleAccesser->getWorkManageModule();
    int statusKind = statusModule.m_statusKind;
    if (statusKind == Fighter::Status::Dead || statusKind == Fighter::Status::Standby || statusKind == Fighter::Status::Rebirth) {
        return true;
    }
    if (workModule.isFlag(Fighter::Instance::Work::Flag_KnockOut)) { return true; }
    return false;
}


void grCellSearch::renderOpa()
{
    grGimmick::renderOpa();
    renderProjection();
    adjustModelMatrix();    // callbacks are okay here?
    renderScnLight();
}

// moving the SceneData light to match the spotlight
// this is absolutely not the way to do this
// but also there's not a method in Brawl that allows you to do this
// technically, keyframes are getting manipulated here.
// expects light index 10 to exist!
void grCellSearch::renderScnLight()
{
    //unsure what the value at 0x20 is
    nw4r::g3d::ResAnmScn* scnRes = (nw4r::g3d::ResAnmScn*)(0x20+(int)g_gfSceneRoot->m_anmScnRes);
    if (scnRes == NULL) { return; }
    nw4r::g3d::ResAnmLight scnLightRes = scnRes->GetResAnmLightByRefNumber(10);
    if (!scnLightRes.IsValid()) { return; }
    // pre-g3d headers conversion:
    // 0x24/0x34 are the "Start Points" and "End Points" coordinates of the light (as displayed in BrawlCrate)
    // Vec3f* scnLightStartPos = (Vec3f*)((uintptr_t)scnLightRes+0x24);
    // *scnLightStartPos = searchSourcePos;
    // Vec3f* scnLightEndPos = (Vec3f*)((uintptr_t)scnLightRes+0x34);
    // *scnLightEndPos = Vec3f(searchPos.m_x,searchPos.m_y,searchPos.m_z*sizeTween.getValue());


    scnLightRes.ref().posX.constValue = searchSourcePos.m_x;
    scnLightRes.ref().posY.constValue = searchSourcePos.m_y;
    scnLightRes.ref().posZ.constValue = searchSourcePos.m_z;

    scnLightRes.ref().aimX.constValue = searchPos.m_x;
    scnLightRes.ref().aimY.constValue = searchPos.m_y;
    scnLightRes.ref().aimZ.constValue = searchPos.m_z*sizeTween.getValue();
}

// built from stMetalGear
// changes camera id 1's position to move the projected light (original used id 2)
// requires materials to have a texture set to mode projection on camera 1
void grCellSearch::renderProjection()
{
    stCellData* stageData = (stCellData*)m_stageData;
    // gfSceneRoot->rootNorm (0x40) isn't in the headers
    // g_gfSceneRoot lwz+0x40
    nw4r::g3d::ScnRoot* rootNorm = *(nw4r::g3d::ScnRoot**)((uintptr_t)g_gfSceneRoot+0x40);

    if (g_gfSceneRoot == NULL) { return; }              // also add a isValid check for rootNorm
    nw4r::g3d::Camera::PostureInfo posture; posture;
    posture.tp = nw4r::g3d::Camera::POSTURE_LOOKAT;     // metalgear uses 2 (aim), we want 0 (lookat)
    posture.cameraUp = nw4r::math::VEC3(0,1,0);
    posture.cameraTarget = nw4r::math::VEC3(searchPos.m_x,searchPos.m_y,searchPos.m_z);
    posture.cameraRotate = nw4r::math::VEC3(0,0,0);
    posture.cameraTwist = 0.0;

    nw4r::g3d::Camera g3dcamera = rootNorm->GetCamera(1);

    searchSourcePos = Vec3f(searchPos.m_x*0.66,0.0,170.0);
    if (isAbove) {
        searchSourcePos.m_y = 200.0;
    } else {
        searchSourcePos.m_y = searchPos.m_y+30.0;
    }
    g3dcamera.SetPosition(searchSourcePos.m_x,searchSourcePos.m_y,searchSourcePos.m_z);
    g3dcamera.SetPosture(posture);
    float size = sizeTween.getValue()*stageData->searchLightBaseSize;
    g3dcamera.SetOrtho(size,-size,size,-size,1.0,10000.0);
}


// vector math functions to line up the light model with the origin and target points (only used when the light is above, but works nonetheless)
// while stages don't generally use n4wr VEC3 and instead vec3f, VEC3 has a lot of useful functions that we need here
// outside the name, they're very similar if not the same in structure, you can cast one as the other
// brawl has similar applications of this as seen in LookTo for g3dCamera
// this is one of the closest functions in terms of accuracy, almost identical outside of the callback stuff
// i hate callbacks
void grCellSearch::adjustModelMatrix()
{
    if (m_sceneModels[0] == NULL) { return; }
    if (m_sceneModels[0]->GetScnMdlCallback() == NULL) {
        m_calcWorldCallBack.m_index = 0;
        m_calcWorldCallBack.m_nodeCallbackDatas[0].m_nodeIndex = 0;
        m_sceneModels[0]->SetScnMdlCallback(&m_calcWorldCallBack);
        m_sceneModels[0]->EnableScnMdlCallbackTiming(nw4r::g3d::ScnObj::CALLBACK_TIMING_A);
        m_sceneModels[0]->SetScnMdlCallbackNodeID(m_calcWorldCallBack.m_nodeCallbackDatas[0].m_nodeIndex);
    }
    // OSReport("flags %x %d\n",&m_calcWorldCallBack.m_nodeCallbackDatas[0].m_flags,m_calcWorldCallBack.m_nodeCallbackDatas[0].m_flags);
    // OSReport("m_nodeIndex %x %d\n",&m_calcWorldCallBack.m_nodeCallbackDatas[0].m_nodeIndex,m_calcWorldCallBack.m_nodeCallbackDatas[0].m_nodeIndex);
    Vec3f from = searchSourcePos;
    Vec3f to = searchPos;
    math::VEC3 dirX;
    math::VEC3 dirY;
    math::VEC3 dirZ;
    math::VEC3Sub(&dirY,(math::VEC3*)&from,(math::VEC3*)&to);
    if (math::VEC3LenSq(&dirY) <= 0.0) {
        dirY = math::VEC3(0,1,0);
    } else {
        math::VEC3Normalize(&dirY,&dirY);
    }
    dirZ = math::VEC3(0,0,1);
    math::VEC3Cross(&dirX,&dirY,&dirZ);
    math::VEC3Normalize(&dirX, &dirX);
    math::VEC3Cross(&dirZ,&dirX,&dirY);
    m_calcWorldCallBack.m_nodeCallbackDatas[0].m_matrix = Matrix(
                            dirX.x,dirY.x,dirZ.x,to.m_x,
                            dirX.y,dirY.y,dirZ.y,to.m_y,
                            dirX.z,dirY.z,dirZ.z,to.m_z);
    m_calcWorldCallBack.m_nodeCallbackDatas[0].m_scale = Vec3f(sizeTween.getValue(),1.0,sizeTween.getValue());
    // setPos(&to);
    // setScale(&m_calcWorldCallBack.m_nodeCallbackDatas[0].m_scale);
    // setMatrix(&m_calcWorldCallBack.m_nodeCallbackDatas[0].m_matrix);
    // OSReport("calc complete\n");
    // OSReport("(%f,%f,%f,%f)\n",posHorizontal.x,posVertical.x,posDepth.x,to.m_x);
    // OSReport("(%f,%f,%f,%f)\n",posHorizontal.y,posVertical.y,posDepth.y,to.m_y);
    // OSReport("(%f,%f,%f,%f)\n",posHorizontal.z,posVertical.z,posDepth.z,to.m_z);
    // OSReport("scale: %f\n",scale);
}