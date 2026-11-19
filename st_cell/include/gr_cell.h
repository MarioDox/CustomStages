#pragma once

#include <gr/gr_yakumono.h>

struct bulletCollision {
    Vec3f min;
    Vec3f max;
    Vec3f maskMin;
    Vec3f maskMax;
    bool hasMask;
};

class grCell : public grYakumono {
protected:
public:
    bulletCollision collisionPlanes[13];
    grCell(const char* taskName) : grYakumono(taskName)
    {
    };
    virtual ~grCell(){};
    virtual void startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType);
    virtual void getLaserSpawnPos(int idx, Vec3f* outVec);

    static grCell* create(int mdlIndex, const char* tgtNodeName, const char* taskName);

};
