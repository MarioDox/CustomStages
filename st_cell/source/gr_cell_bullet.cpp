#include "gr_cell_bullet.h"
#include "st_cell_data.h"
#include <ec/ec_mgr.h>
#include <gr/gr_madein.h>
#include <mt/mt_prng.h>
#include <nw4r/math/math_types.h>
#include <hkmath/hkMath.h>
using namespace nw4r;

grCellBullet* grCellBullet::create(int mdlIndex, const char* tgtNodeName, const char* taskName)
{
    grCellBullet* ground = new (Heaps::StageInstance) grCellBullet(taskName);
    ground->setupMelee();
    ground->setMdlIndex(mdlIndex);
    ground->setTgtNode(tgtNodeName);
    ground->makeCalcuCallback(1, Heaps::StageInstance);
    ground->setCalcuCallbackRoot(7);

    return ground;
}

void grCellBullet::startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType)
{
    grMadein::startup(archive, unk1, layerType);
    setupAttack();
    initializeEntity();
    startEntity();
    setSleepAttack(true);
    setNodeVisibilityAll(false,0);
}

void grCellBullet::setupAttack()
{
    stCellData* stageData = (stCellData*)m_stageData;
    Vec3f offsetPos = Vec3f(0,0,0);
    setAttack(1.0,&offsetPos);
    m_attackInfo->m_preset = Attack_Overwrite;
    soCollisionAttackData* overwriteAttackData = getOverwriteAttackData();
    createAttackPointNormal(overwriteAttackData);
    overwriteAttackData->m_power = stageData->bulletPower;
    if (isFinal) {
        overwriteAttackData->m_reactionEffect = stageData->bulletFinalKBG;
        overwriteAttackData->m_reactionFix = 0;
        overwriteAttackData->m_reactionAdd = stageData->bulletFinalBKB;
        overwriteAttackData->m_vector = stageData->bulletFinalAngle;
        overwriteAttackData->m_soundLevel = soCollisionAttackData::Sound_Level_Large;
        overwriteAttackData->m_soundAttribute = soCollisionAttackData::Sound_Attribute_Kick;
    } else {
        overwriteAttackData->m_reactionEffect = stageData->bulletKBG;
        overwriteAttackData->m_reactionFix = 0;
        overwriteAttackData->m_reactionAdd = stageData->bulletBKB;
        overwriteAttackData->m_vector = stageData->bulletAngle;
        overwriteAttackData->m_soundLevel = soCollisionAttackData::Sound_Level_Small;
        overwriteAttackData->m_soundAttribute = soCollisionAttackData::Sound_Attribute_Kick;
    }
    overwriteAttackData->m_size = stageData->bulletSize;          // they miss a lot, even in the original
    if (isAbove) {
        overwriteAttackData->m_offsetPos = Vec3f(stageData->bulletHitboxAboveOffsetPosX,stageData->bulletHitboxAboveOffsetPosY,stageData->bulletHitboxAboveOffsetPosZ);      // in the original they only come from Y 400
        overwriteAttackData->m_shapeType = soCollision::Shape_Capsule;
    } else {
        overwriteAttackData->m_offsetPos = Vec3f(stageData->bulletHitboxOffsetPosX,stageData->bulletHitboxOffsetPosY,stageData->bulletHitboxOffsetPosZ);
        overwriteAttackData->m_shapeType = soCollision::Shape_Sphere;
    }
    overwriteAttackData->m_hitStopFrame = 1.0;
    overwriteAttackData->m_hitStopDelay = stageData->bulletSDIMult; //SDI mult
    overwriteAttackData->m_nodeIndex = 0;
    overwriteAttackData->m_targetCategory = soCollision::CATEGORY_MASK_ALL;

    overwriteAttackData->m_targetSituationODD = true;
    overwriteAttackData->m_targetSituationAir = true;
    overwriteAttackData->m_targetSituationGround = true;

    overwriteAttackData->m_targetLr = false;
    overwriteAttackData->m_targetPart = soCollision::PART_MASK_ALL;
    overwriteAttackData->m_attribute = soCollisionAttackData::Attribute_Normal;

    overwriteAttackData->m_setOffKind = soCollisionAttackData::SetOff_Off;

    overwriteAttackData->m_noScale = false;
    overwriteAttackData->m_isShieldable = false;
    overwriteAttackData->m_isReflectable = false;
    overwriteAttackData->m_isAbsorbable = false;
    overwriteAttackData->m_region = soCollisionAttackData::Region_None;

    overwriteAttackData->m_serialHitFrame = 0;
    overwriteAttackData->m_isDirect = false;
    overwriteAttackData->m_isInvalidInvincible = false;
    overwriteAttackData->m_isInvalidXlu = false;
    overwriteAttackData->m_lrCheck = soCollisionAttackData::Lr_Check_Pos;
    overwriteAttackData->m_isCatch = false;
    overwriteAttackData->m_noTeam = true;
    overwriteAttackData->m_noHitStop = false;
    overwriteAttackData->m_noEffect = false;
    overwriteAttackData->m_noTransaction = false;
    overwriteAttackData->m_subShield = 0;
    overwriteAttackData->m_slipChance = 0.0;
}

void grCellBullet::update(float deltaFrame)
{
    grMadein::update(deltaFrame);
    updateTravel(deltaFrame);
    updateImpact(deltaFrame);
}

void grCellBullet::updateTravel(float deltaFrame)
{
    if (!isActive) { return; }
    hitboxDelay-=deltaFrame;
    if (hitboxDelay < 0.0) {
        hitboxDelay = 0.0;
    }
    if (hitboxDelay == 0) {
        if (hitboxDuration == 0) {
            setSleepAttack(true);
            setNodeVisibilityAll(false,0);
        } else {
            setSleepAttack(false);
        }
        hitboxDuration-=deltaFrame;
        if (hitboxDuration < 0.0) {
            hitboxDuration = 0.0;
        }
    }
}

void grCellBullet::updateImpact(float deltaFrame)
{
    impactDelay-=deltaFrame;
    if (impactDelay < 0.0) {
        impactDelay = 0.0;
    }
    // do the impact
    if (impactDelay == 0.0) {
        isActive = false;
        endEntity();
        if (!awaitingImpact) { return; }
        Vec3f gfxRot = Vec3f(0,0,0);
        Vec3f gfxScale = Vec3f(1,1,1);
        gfxRot.m_x = hkMath::acos(impactNormal.m_y);
        gfxRot.m_y = atan2f(impactNormal.m_x,impactNormal.m_z);
        g_ecMgr->killEffect(bulletGFX,1,1);
        g_ecMgr->setDrawPrio(1);
        g_ecMgr->setEffect((EfID)0x530002,&impactPos,&gfxRot,&gfxScale);
        g_ecMgr->setDrawPrio(0xffffffff);
        gfxScale = Vec3f(0.35,0.35,0.35);
        g_ecMgr->setDrawPrio(1);
        g_ecMgr->setEffect((EfID)0x530004,&impactPos,&gfxRot,&gfxScale);
        g_ecMgr->setDrawPrio(0xffffffff);
        soundGenerator.setPos(&impactPos);
        int startSFX = snd_bound1;
        if (isAbove) {
            startSFX = snd_bound4;
        }
        soundGenerator.playSE((SndID)(startSFX+randi(3)),0,0,-1);
        awaitingImpact = false;
    }
}

void grCellBullet::shoot()
{
    stCellData* stageData = (stCellData*)m_stageData;
    setupAttack();
    startEntity();  // this also makes hitbox changes by setupAttack actually be applied
    setSleepAttack(true);
    hitboxDuration = internalAttackDuration;
    isActive = true;
    awaitingImpact = false;
    impactDelay = 999;
    impactPos = *originWork;
    //simple look at / point at logic
    Vec3f startPos = *originWork;
    if (isAbove) {
        startPos = startPos + Vec3f(stageData->bulletAboveStartPosX,stageData->bulletAboveStartPosY,stageData->bulletAboveStartPosZ);
    } else {
        startPos = startPos + Vec3f(stageData->bulletStartPosX,stageData->bulletStartPosY,stageData->bulletStartPosZ);
    }
    Vec3f tempVec;
    tempVec = getPos()-startPos;
    tempVec = tempVec / tempVec.length();
    tempVec = Vec3f(asinf(tempVec.m_y),atan2f(-tempVec.m_x,-tempVec.m_z),0);
    Vec3f scale = Vec3f(1,1,1);
    g_ecMgr->setDrawPrio(1);
    bulletGFX = g_ecMgr->setEffect((EfID)0x530001,&startPos,&tempVec,&scale);
    g_ecMgr->setDrawPrio(0xffffffff);
    Vec3f endPos = Vec3f(getPos().m_x,getPos().m_y,getPos().m_z);

    soundGenerator.setPos(&startPos);
    soundGenerator.playSE((SndID)snd_gun,0,0,-1);

    // Bullet impact effect position calculation
    // we want the bullet effects to be "aimed" towards the shoot direction
    endPos = endPos-startPos;
    float length = endPos.length();
    float ext = (length+100.0)/length;
    endPos = Vec3f(startPos.m_x+endPos.m_x*ext,startPos.m_y+endPos.m_y*ext,startPos.m_z+endPos.m_z*ext);
    float bestFraction = 1.0;
    bulletCalc outCalc;
    outCalc.outFraction = 1.0;
    outCalc.outVec = Vec3f(0,0,0);
    outCalc.outNormal = Vec3f(0,1,0);
    for (int i=0;i<13;i++) {
        if (calcBulletIntersect(&startPos,&endPos,&grCellRef->collisionPlanes[i].min,&grCellRef->collisionPlanes[i].max,&outCalc)) {
            if (outCalc.outFraction >= bestFraction) { continue; }     // prioritize the closest impact
            bestFraction = outCalc.outFraction;
            impactPos = outCalc.outVec;        // we write to the variable used then after the impact timer
            impactNormal = outCalc.outNormal;
            awaitingImpact = true;
            // check if it's in the mask zone! if so, don't do impacts!
            if (grCellRef->collisionPlanes[i].hasMask &&
                impactPos.m_x > grCellRef->collisionPlanes[i].maskMin.m_x && impactPos.m_x < grCellRef->collisionPlanes[i].maskMax.m_x &&
                impactPos.m_y > grCellRef->collisionPlanes[i].maskMin.m_y && impactPos.m_y > grCellRef->collisionPlanes[i].maskMax.m_y) {
                awaitingImpact = false;
            }
        }
    }
    // our gfx speed is 32.0
    Vec3f distanceVec = endPos;
    float distance = distanceVec.distance(&startPos);
    impactDelay = distance/32.0;
    hitboxDelay = (distance/32.0)*0.5;  //half way

}


// check if the specified segment intersected the specified plane
bool grCellBullet::calcBulletIntersect(Vec3f* start, Vec3f* end, Vec3f* min, Vec3f* max, bulletCalc* outCalc)
{
    // convert to floats to allow selecting with index as an array
    float startVec[3] = {start->m_x,start->m_y,start->m_z}; 
    float endVec[3] = {end->m_x,end->m_y,end->m_z}; 
    float minVec[3] = {min->m_x,min->m_y,min->m_z}; 
    float maxVec[3] = {max->m_x,max->m_y,max->m_z}; 
    int axis = -1;      // start with no axis selected
    // loop for each axis (3) and check if the minimum and maximum value are the same (the models allow this)
    // if so, they're the wall to be intersected on that axis
    for (int i=0;i<3;i++) {
        if (minVec[i] == maxVec[i]) {
            axis = i;       // our direction to check
            break;
        }
    }
    if (axis < 0) { return false; }     // we didn't find a flat wall

    // modulo, allows us to get the other two axis that aren't the chosen one
    int j = (axis+1)%3;
    int k = (axis+2)%3;

    // start to calculate the intersection on our axis
    float dir = endVec[axis]-startVec[axis];
    if (dir == 0.0) { return false; }                           // never intersecting
    float fraction = (minVec[axis] - startVec[axis]) / dir;     // get the traction from the start to our blocking position
    if (fraction < 0.0 || fraction > 1.0) { return false; }     // if we're outside our range, then the wall wasn't hit

    float newVec[3];                                // new vector
    newVec[axis] = minVec[axis];                    // we hit the wall on this coordinate so it stops there

    // get the difference of the end vector from the start, multiply it with the fraction to reduce the axis by the same amount
    // add back the start vector to have the proper blocking position on all axis
    newVec[j] = startVec[j]+(endVec[j]-startVec[j])*fraction;
    newVec[k] = startVec[k]+(endVec[k]-startVec[k])*fraction;

    // now check if the other axis are in the bounds of the wall
    if (newVec[j] < minVec[j] || newVec[j] > maxVec[j]) { return false; }
    if (newVec[k] < minVec[k] || newVec[k] > maxVec[k]) { return false; }
    outCalc->outVec = Vec3f(newVec[0],newVec[1],newVec[2]);
    outCalc->outFraction = fraction;                            // export also our fraction
    float outNormal[3] = {0.0,0.0,0.0};                         // our stage is aligned so normal calculation can be easily skipped
    if (axis == 0 && newVec[0] > startVec[0]) {                 // if we collided horizontally and the wall is on the right
        outNormal[axis] = -1.0;                                 // then we point left
    } else {
        outNormal[axis] = 1.0;                                  // otherwise right (or up for Y and towards us for Z!)
    }
    outCalc->outNormal = Vec3f(outNormal[0],outNormal[1],outNormal[2]);
    return true;
}