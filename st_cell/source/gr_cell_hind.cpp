#include "gr_cell_hind.h"
#include "st_cell_data.h"
#include <ec/ec_mgr.h>

grCellHind* grCellHind::create(int mdlIndex, const char* tgtNodeName, const char* taskName)
{
    grCellHind* ground = new (Heaps::StageInstance) grCellHind(taskName);
    ground->setupMelee();
    ground->setMdlIndex(mdlIndex);
    ground->setTgtNode(tgtNodeName);
    return ground;
}

void grCellHind::startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType)
{
    grYakumono::startup(archive,unk1,layerType);
    setVisibility(false);
}

void grCellHind::update(float deltaFrame)
{
    stCellData* stageData = (stCellData*)m_stageData;
    Vec3f tempVec;
    Vec3f gfxRot = Vec3f(0,0,0);
    Vec3f gfxScale = Vec3f(stageData->heliWindGFXScale,stageData->heliWindGFXScale,stageData->heliWindGFXScale);
    switch (state)
    {
    case ARRIVING:
        getNodePosition(&tempVec,0,"st16_hind2");
        soundGenerator.setPos(&tempVec);
        if (getMotionFrame(0) >= stageData->heliStartSFXFrame && !didSFX) {
            didSFX = true;
            soundGenerator.playSE((SndID)snd_hind_in,0,0,-1);
        }
        if (getMotionFrame(0) >= stageData->heliLoopSFXFrame && !didLoopSFX) {
            didLoopSFX = true;
            soundGenerator.setPos(&grCellSearchRef->searchPos);
            loopSFX = soundGenerator.playSE((SndID)snd_hind_loop,0,0,-1);
        }
        if (getMotionFrame(0) >= stageData->heliStartAnimEndFrame) {
            state = ACTIVE;
            grCellSearchRef->startMode(1);
            setVisibility(false);
        }
        break;
    case ACTIVE:
        soundGenerator.setPos(&grCellSearchRef->searchPos);
        if (grCellSearchRef->lightState == grCellSearch::State_Cooldown) {
            state = RETREATING;
            setMotionFrame(stageData->heliStartAnimEndFrame,0);
            setMotionRatio(-1.0);
            tempVec = Vec3f(-90,900,0);
            soundGenerator.setPos(&tempVec);
            soundGenerator.stopSE(loopSFX,0.0);
            soundGenerator.playSE((SndID)snd_hind_out,0,0,-1);
        }
        gfxTimer -= deltaFrame;
        tempVec = Vec3f(grCellSearchRef->searchPos.m_x*stageData->heliWindGFXXPosMult,0,0);
        if (gfxTimer <= 0) {
            gfxTimer = 9;
            g_ecMgr->setDrawPrio(1);
            g_ecMgr->setEffect((EfID)0x530003,&tempVec,&gfxRot,&gfxScale);
            g_ecMgr->setDrawPrio(0xffffffff);
        }
    case RETREATING:
        getNodePosition(&tempVec,0,"st16_hind2");
        soundGenerator.setPos(&tempVec);
        if (getMotionFrame(0) <= 0.0) {
            state = OFF;
        }
        break;
    default:
        break;
    }
}

void grCellHind::startActivate()
{
    state = ARRIVING;
    setVisibility(true);
    setMotionFrame(0.0,0);
    setMotionRatio(1.0);
    didSFX = false;
    didLoopSFX = false;
    gfxTimer = 0.0;
}