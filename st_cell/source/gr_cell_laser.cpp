#include "gr_cell_laser.h"
#include "st_cell_data.h"
#include <gm/gm_global.h>
#include <ec/ec_mgr.h>
#include <memory.h>
#include <ft/ft_manager.h>
#include <mt/mt_prng.h>
#include <ft/ft_external_value_accesser.h>

grCellLaser* grCellLaser::create(int mdlIndex, const char* tgtNodeName, const char* taskName)
{
    grCellLaser* ground = new (Heaps::StageInstance) grCellLaser(taskName);
    ground->setupMelee();
    ground->setMdlIndex(mdlIndex);
    ground->setTgtNode(tgtNodeName);
    ground->makeCalcuCallback(1, Heaps::StageInstance);
    ground->setCalcuCallbackRoot(7);
    return ground;
}

void grCellLaser::startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType)
{
    grMadein::startup(archive, unk1, layerType);
    setVisibility(false);
    state = OFF;
    setupAttack();
    initializeEntity();
    endEntity();
}

void grCellLaser::update(float deltaFrame)
{
    grMadein::update(deltaFrame);
    updateState(deltaFrame);
    updateDetect(deltaFrame);
}

void grCellLaser::updateState(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    stateTimer-=deltaFrame;
    if (stateTimer < 0.0) {
        stateTimer = 0.0;
    }
    switch (state)
    {
    case SPAWN:
        if (stateTimer == 0.0) {
            state = IDLE;
            stateTimer = stageData->laserDuration;
            setupAttack();
            createAttackPoint();
            // createAttackPointNormal(getOverwriteAttackData());
            changeMatColAnim(1,0);
        }
        break;
    case IDLE:
        if (stateTimer == 0.0) {
            state = DISAPPEAR;
            deleteAttackPoint();
            changeMatColAnim(3,0);
            stateTimer = stageData->laserDisappearFrames;
        }
        break;
    case DISAPPEAR:
        if (stateTimer == 0.0) {
            state = OFF;
            setVisibility(false);
        }
        break;
    case TRIGGERED_START:
        if (stateTimer == 0.0) {
            state = TRIGGERED;
            performAlarm();
        }
        break;
    case TRIGGERED:
        if (grCellSearchRef->lightState == grCellSearch::State_Cooldown) {
            state = DISAPPEAR;
            setVisibility(false);
            stateTimer = stageData->laserDisappearFrames;
        }
        break;
    default:
        break;
    }
}

void grCellLaser::updateDetect(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    if (state != IDLE) { return; }      // if it's going away, don't trigger!
    if (alarmedCount == 0) { return; }
    // same thing as search
    int idx = randi(alarmedCount);
    if (idx < 0) { idx = 0; } // unnecessary?
    if (idx > alarmedCount - 1) {
        idx = alarmedCount - 1;
    }

    decidedVictim = caughtPlayers[idx];
    for (int i=0;i<laserAmount;i++) {
        if (laserList[i] == this) { continue; }
        laserList[i]->state = DISAPPEAR;
        laserList[i]->deleteAttackPoint();
        laserList[i]->changeMatColAnim(3,0);
    }
    state = TRIGGERED_START;
    stateTimer = stageData->laserTriggerAlarmDelay;      // delay for the alarm
    deleteAttackPoint();
    changeMatColAnim(2,0);
}

void grCellLaser::performAlarm()
{
    if (decidedVictim == NULL) { return; }
    // same as search but we also forcefully set the light on the player here otherwise it will use the last position
    Vec3f tempVec = grCellSearchRef->getPlayerPosition(decidedVictim);
    grCellSearchRef->searchPos = tempVec;
    grCellSearchRef->searchTgtPos = tempVec;
    grCellSearchRef->startDetect(decidedVictim);
    decidedVictim = NULL;
}

void grCellLaser::addAlarmingPlayer(int* group, int taskId)
{
    for (int i=0;i<MAX_PLAYERS;i++) {
        if (group[i] != NULL) { continue; }
        if (group[i] == taskId) { return; }
        group[i] = taskId;
        alarmedCount++;
        // OSReport("added allarming player %x\n",entryId);
        
        return;
    }
}

void grCellLaser::onInflictEach(soCollisionLog* collisionLog, float power)
{
    if (state != IDLE) { return; }      // don't hit while spawning!
    int entryID = g_ftManager->getEntryIdFromTaskId(collisionLog->m_taskId, NULL);
    Fighter* fighter = g_ftManager->getFighter(entryID, -1);
    if (fighter == NULL) { return; }
    if (checkDead(collisionLog->m_taskId)) { return; }
    addAlarmingPlayer(caughtPlayers,collisionLog->m_taskId);        // we do this here, more straightforward
    // OSReport("adding player %d\n",playerNo);
}

bool grCellLaser::checkDead(int taskId)
{
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

// instead of area detection, we use a flinchless hitbox to precisely check hurtboxes
// areas can be visually misleading and hurtboxes make this more interesting
void grCellLaser::setupAttack()
{
    stCellData* stageData = (stCellData*)m_stageData;
    Vec3f offsetPos = Vec3f(0,0,0);
    setAttack(1.0,&offsetPos);
    m_attackInfo->m_preset = Attack_Overwrite;
    soCollisionAttackData* overwriteAttackData = getOverwriteAttackData();
    overwriteAttackData->m_power = 0;
    overwriteAttackData->m_reactionEffect = 0;
    overwriteAttackData->m_reactionFix = 0;
    overwriteAttackData->m_reactionAdd = 0;
    overwriteAttackData->m_vector = 0;
    overwriteAttackData->m_soundLevel = soCollisionAttackData::Sound_Level_Small;
    overwriteAttackData->m_soundAttribute = soCollisionAttackData::Sound_Attribute_None;
    overwriteAttackData->m_size = stageData->laserHitboxSize;
    overwriteAttackData->m_offsetPos = Vec3f(stageData->laserHitboxOffsetX,stageData->laserHitboxOffsetY,stageData->laserHitboxOffsetZ);
    overwriteAttackData->m_shapeType = soCollision::Shape_Capsule;
    overwriteAttackData->m_hitStopFrame = 0.0;
    overwriteAttackData->m_hitStopDelay = 0.0;
    overwriteAttackData->m_nodeIndex = 1;
    overwriteAttackData->m_targetCategory = soCollision::CATEGORY_MASK_ALL;

    overwriteAttackData->m_targetSituationODD = true;
    overwriteAttackData->m_targetSituationAir = true;
    overwriteAttackData->m_targetSituationGround = true;

    overwriteAttackData->m_targetLr = false;
    overwriteAttackData->m_targetPart = soCollision::PART_MASK_ALL;
    overwriteAttackData->m_attribute = soCollisionAttackData::Attribute_None;

    overwriteAttackData->m_setOffKind = soCollisionAttackData::SetOff_Off;

    overwriteAttackData->m_noScale = false;
    overwriteAttackData->m_isShieldable = false;
    overwriteAttackData->m_isReflectable = false;
    overwriteAttackData->m_isAbsorbable = false;
    overwriteAttackData->m_region = soCollisionAttackData::Region_None;

    overwriteAttackData->m_serialHitFrame = 1;
    overwriteAttackData->m_isDirect = false;
    overwriteAttackData->m_isInvalidInvincible = true;
    overwriteAttackData->m_isInvalidXlu = true;
    overwriteAttackData->m_lrCheck = soCollisionAttackData::Lr_Check_Pos;
    overwriteAttackData->m_isCatch = false;
    overwriteAttackData->m_noTeam = true;
    overwriteAttackData->m_noHitStop = true;
    overwriteAttackData->m_noEffect = false;
    overwriteAttackData->m_noTransaction = true;
    overwriteAttackData->m_subShield = 0;
    overwriteAttackData->m_slipChance = 0.0;
}

void grCellLaser::spawnLaser()
{
    stCellData* stageData = (stCellData*)m_stageData;
    Vec3f tempVec = initialPos;
    tempVec.m_x += stageData->laserRandX*((randf()-0.5)*2.0);        // -25 to +25
    tempVec.m_y = stageData->laserBaseY+(stageData->laserAddRandY*randf());
    setPos(&tempVec);
    setVisibility(true);
    changeMatColAnim(0,0);
    stateTimer = stageData->laserSpawnFrames;
    state = SPAWN;
    startEntity();
    for (int i=0;i<laserAmount;i++) {
        caughtPlayers[i] = 0;
    }
    alarmedCount = 0;
    soundGenerator.setPos(&tempVec);
    soundGenerator.playSE((SndID)snd_infrared,0,0,-1); // exclamation
}