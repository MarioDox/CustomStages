#include "st_cell.h"
#include "gr_cell.h"
#include "st_cell_data.h"
#include "gr_cellsearch.h"
#include "gr_cell_laser.h"
#include "gr_cell_hind.h"
#include <memory.h>
#include <st/st_class_info.h>
#include <mt/mt_prng.h>

static stClassInfoImpl<Stages::Final, stCell> classInfo = stClassInfoImpl<Stages::Final, stCell>();

stCell* stCell::create()
{
    stCell* stage = new (Heaps::StageInstance) stCell();
    return stage;
}
bool stCell::loading()
{
    return true;
}

void stCell::createObj()
{
    testStageParamInit(m_fileData, 0xA);
    testStageDataInit(m_fileData, 0x14, 1);

    stCellData* stageData = (stCellData*)m_stageData;

    grCell* ground = grCell::create(0, "", "grCellScene");
    if (ground != NULL)
    {
        addGround(ground);
        ground->startup(m_fileData, 0, gfSceneRoot::Layer_Ground);
        ground->setStageData(m_stageData);
        grCellRef = ground;
    }
    search = grCellSearch::create(7, "", "grCellSearch");
    if (search != NULL)
    {
        addGround(search);
        search->setStageData(m_stageData);
        search->startup(m_fileData, 0, gfSceneRoot::Layer_Ground);
        search->setAlarmRequestWork(&alarmRequested);
    }
    grCellHindRef = grCellHind::create(1, "", "grCellHind");
    if (grCellHindRef != NULL)
    {
        addGround(grCellHindRef);
        grCellHindRef->setStageData(m_stageData);
        grCellHindRef->startup(m_fileData, 0, gfSceneRoot::Layer_Ground);
        grCellHindRef->grCellSearchRef = search;
    }

    for (int i=0;i<bulletAmount;i++) {
        grCellBullet* bullet = grCellBullet::create(8, "", "grCellBullet");
        addGround(bullet);
        bullet->setStageData(m_stageData);
        bullet->startup(m_fileData, 0, gfSceneRoot::Layer_Ground);
        search->bulletList[i] = bullet;
        bullet->grCellRef = grCellRef;
        bullet->setOriginWork(&search->searchSourcePos);
    }
    Vec3f tempVec;
    for (int i=0;i<laserAmount;i++) {
        lasers[i] = grCellLaser::create(9, "", "grCellLaser");
        addGround(lasers[i]);
        lasers[i]->setStageData(m_stageData);
        lasers[i]->startup(m_fileData, 0, gfSceneRoot::Layer_Ground);
        lasers[i]->grCellRef = grCellRef;
        lasers[i]->grCellSearchRef = search;
        lasers[i]->laserList = lasers;
        grCellRef->getLaserSpawnPos(i,&lasers[i]->initialPos);
    }
    randomizePattern(true);
    float randMin = stageData->patternInitialTimerMin; //300
    float randMax = stageData->patternInitialTimerMax; //420
    stateTimer = randMin+(randf()*(randMax-randMin));

    createCollision(m_fileData, 2, NULL);
    initCameraParam();
    nw4r::g3d::ResFile posData(m_fileData->getData(Data_Type_Model, 0x64, 0xfffe));
    if (posData.ptr() == NULL)
    {
        // if no stgPos model in pac, use defaults
        createStagePositions();
    }
    else
    {
        createStagePositions(&posData);
    }
    ambientSFX = playSeBasic((SndID)snd_ambient,0.0);
    createWind2ndOnly();
    m_wind2ndTrigger->setAreaSleep(true);
    loadStageAttrParam(m_fileData, 0x1E);
    nw4r::g3d::ResFileData* scnData = static_cast<nw4r::g3d::ResFileData*>(m_fileData->getData(Data_Type_Scene, 0, 0xfffe));
    registScnAnim(scnData, 0);
    initPosPokeTrainer(1, 0);
    createObjPokeTrainer(m_fileData, 0x65, "PokeTrainer00", this->m_pokeTrainerPos, 0x0);
}

void stCell::update(float deltaFrame)
{
    if (!isGo) { return; }      // don't do anything if the match didn't start!
    stCellData* stageData = (stCellData*)m_stageData;
    bool tempBool;
    Vec3f tempVec;
    int tempInt;
    if (!isAlarm && alarmRequested) {
        nw4r::g3d::ResFileData* scnData = static_cast<nw4r::g3d::ResFileData*>(m_fileData->getData(Data_Type_Scene, 0, 0xfffe));
        registScnAnim(scnData, 1);
        grCellRef->changeMatColAnim(1,0);
        isAlarm = true;
        alarmSFX = playSeBasic((SndID)snd_siren,0.0); // alarm
    } else if (isAlarm && !alarmRequested) {
        nw4r::g3d::ResFileData* scnData = static_cast<nw4r::g3d::ResFileData*>(m_fileData->getData(Data_Type_Scene, 0, 0xfffe));
        registScnAnim(scnData, 0);
        grCellRef->changeMatColAnim(0,0);
        isAlarm = false;
        stopSeBasic(ambientSFX,0.5);
        ambientSFX = playSeBasic((SndID)snd_ambient,0.0);
        stopSeBasic(alarmSFX,0.5);
    }
    stateTimer-=deltaFrame;
    if (stateTimer < 0) {
        stateTimer = 0;
    }
    switch (state)
    {
    case IDLE:
        if (stateTimer == 0) {
            if (currentPatternSet[currentPatternIdx] == 2) {
                state = WAIT_LASER;
                for (int i=0;i<laserAmount;i++) {
                    lasers[i]->spawnLaser();
                }
            } else if (currentPatternSet[currentPatternIdx] == 1) {
                state = WAIT_HELI;
                grCellHindRef->startActivate();
            } else {
                state = WAIT_PATH;
                search->startMode(0);
            }
        }
        break;
    case WAIT_PATH:
        if (search->lightState == grCellSearch::State_Cooldown) {
            state = IDLE;
            nextPattern();
        }
        break;
    case WAIT_HELI:
        if (grCellHindRef->state == grCellHind::ACTIVE && !didHeli) {
            didHeli = true;
            zoomOutCamera(stageData->heliCameraDist1,stageData->heliCameraDist2);
            m_wind2ndTrigger->setAreaSleep(false);
        }
        if (grCellHindRef->state == grCellHind::RETREATING && didHeli) {
            didHeli = false;
            zoomInCamera();
            m_wind2ndTrigger->setAreaSleep(true);
        }
        if (grCellHindRef->state == grCellHind::OFF) {
            state = IDLE;
            nextPattern();
            didHeli = false;
        }
        break;
    case WAIT_LASER:
        tempBool = false;
        for (int i=0;i<laserAmount;i++) {
            if (lasers[i]->state == grCellLaser::OFF) { continue; }
            tempBool = true;
        }
        if (!tempBool) {
            state = IDLE;
            nextPattern();
        }
        break;
    default:
        break;
    }
}

void stCell::nextPattern()
{
    stCellData* stageData = (stCellData*)m_stageData;
    float randMin = stageData->patternTimerMin; //180
    float randMax = stageData->patternTimerMax; //300
    stateTimer = randMin+(randf()*(randMax-randMin));
    if (currentPatternIdx >= 2) {
        randomizePattern(false);
        currentPatternIdx = 0;
    } else {
        currentPatternIdx++;
    }
}

void stCell::randomizePattern(bool isFirst)
{
    int id = randi(3);      //randi excludes max number
    if (!isFirst) {
        while (id == currentPatternSet[currentPatternIdx])
        {
            id = randi(3);
        }
    }
    // using modulo to limit the range to 0-2
    currentPatternSet[0] = (patternGimmickType)id;
    currentPatternSet[1] = (patternGimmickType)((id+1)%3);
    currentPatternSet[2] = (patternGimmickType)((id+2)%3);
}

void stCell::notifyEventInfoGo()
{
    isGo = true;
}

bool stCell::isBamperVector()
{
    return true;
}
GXColor stCell::getFinalTechniqColor()
{
    return (GXColor){0x14,0x00,0x04,0x96};
}

ST_CLASS_INFO;