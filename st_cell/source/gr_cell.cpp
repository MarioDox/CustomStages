#include "gr_cell.h"
#include <ec/ec_mgr.h>
#include <memory.h>
#include <stdio.h>

grCell* grCell::create(int mdlIndex, const char* tgtNodeName, const char* taskName)
{
    grCell* ground = new (Heaps::StageInstance) grCell(taskName);
    ground->setupMelee();
    ground->setMdlIndex(mdlIndex);
    ground->setTgtNode(tgtNodeName);

    return ground;
}


void grCell::startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType)
{
    grYakumono::startup(archive,unk1,layerType);
    Vec3f min, max;
    getNodePosition(&min,0,"collFloor0N");
    getNodePosition(&max,0,"collFloor1N");
    collisionPlanes[0].min = min;
    collisionPlanes[0].max = max;
    collisionPlanes[0].hasMask = false;
    collisionPlanes[0].maskMin = Vec3f(0,0,0);
    collisionPlanes[0].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collWallL0N");
    getNodePosition(&max,0,"collWallL1N");
    collisionPlanes[1].min = min;
    collisionPlanes[1].max = max;
    collisionPlanes[1].hasMask = false;
    collisionPlanes[1].maskMin = Vec3f(0,0,0);
    collisionPlanes[1].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collWallR0N");
    getNodePosition(&max,0,"collWallR1N");
    collisionPlanes[2].min = min;
    collisionPlanes[2].max = max;
    collisionPlanes[2].hasMask = false;
    collisionPlanes[2].maskMin = Vec3f(0,0,0);
    collisionPlanes[2].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxL0N");
    getNodePosition(&max,0,"collBoxL1N");
    collisionPlanes[3].min = min;
    collisionPlanes[3].max = max;
    collisionPlanes[3].hasMask = false;
    collisionPlanes[3].maskMin = Vec3f(0,0,0);
    collisionPlanes[3].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxL2N");
    getNodePosition(&max,0,"collBoxL3N");
    collisionPlanes[4].min = min;
    collisionPlanes[4].max = max;
    collisionPlanes[4].hasMask = false;
    collisionPlanes[4].maskMin = Vec3f(0,0,0);
    collisionPlanes[4].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxL4N");
    getNodePosition(&max,0,"collBoxL5N");
    collisionPlanes[5].min = min;
    collisionPlanes[5].max = max;
    collisionPlanes[5].hasMask = false;
    collisionPlanes[5].maskMin = Vec3f(0,0,0);
    collisionPlanes[5].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxR0N");
    getNodePosition(&max,0,"collBoxR1N");
    collisionPlanes[6].min = min;
    collisionPlanes[6].max = max;
    collisionPlanes[6].hasMask = false;
    collisionPlanes[6].maskMin = Vec3f(0,0,0);
    collisionPlanes[6].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxR2N");
    getNodePosition(&max,0,"collBoxR3N");
    collisionPlanes[7].min = min;
    collisionPlanes[7].max = max;
    collisionPlanes[7].hasMask = false;
    collisionPlanes[7].maskMin = Vec3f(0,0,0);
    collisionPlanes[7].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collBoxR4N");
    getNodePosition(&max,0,"collBoxR5N");
    collisionPlanes[8].min = min;
    collisionPlanes[8].max = max;
    collisionPlanes[8].hasMask = false;
    collisionPlanes[8].maskMin = Vec3f(0,0,0);
    collisionPlanes[8].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collMidWall0N");
    getNodePosition(&max,0,"collMidWall1N");
    collisionPlanes[9].min = min;
    collisionPlanes[9].max = max;
    collisionPlanes[9].hasMask = false;
    collisionPlanes[9].maskMin = Vec3f(0,0,0);
    collisionPlanes[9].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collMidWallFloor0N");
    getNodePosition(&max,0,"collMidWallFloor1N");
    collisionPlanes[10].min = min;
    collisionPlanes[10].max = max;
    collisionPlanes[10].hasMask = false;
    collisionPlanes[10].maskMin = Vec3f(0,0,0);
    collisionPlanes[10].maskMax = Vec3f(0,0,0);

    getNodePosition(&min,0,"collFarWall0N");
    getNodePosition(&max,0,"collFarWall1N");
    collisionPlanes[11].min = min;
    collisionPlanes[11].max = max;
    collisionPlanes[11].hasMask = true;
    getNodePosition(&min,0,"collFarWindow0N");
    getNodePosition(&max,0,"collFarWindow1N");
    collisionPlanes[11].maskMin = min;
    collisionPlanes[11].maskMax = max;

    getNodePosition(&min,0,"collFarWall2N");
    getNodePosition(&max,0,"collFarWall3N");
    collisionPlanes[12].min = min;
    collisionPlanes[12].max = max;
    collisionPlanes[12].hasMask = true;
    getNodePosition(&min,0,"collFarWindow2N");
    getNodePosition(&max,0,"collFarWindow3N");
    collisionPlanes[12].maskMin = min;
    collisionPlanes[12].maskMax = max;
}

void grCell::getLaserSpawnPos(int idx, Vec3f* outVec)
{
    char nodeName[15];
    sprintf(nodeName,"laserPos%01dN",idx);
    getNodePosition(outVec,0,nodeName);
}