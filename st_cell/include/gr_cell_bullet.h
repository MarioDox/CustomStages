#pragma once

#include <gr/gr_madein.h>
#include "gr_cell.h"

static const float internalAttackDuration = 1.0;

struct bulletCalc {
    Vec3f outVec;
    float outFraction;
    Vec3f outNormal;
};

class grCellBullet : public grMadein {
protected:
    snd3DGenerator soundGenerator;
    float hitboxDelay;
    float hitboxDuration;
    float impactDelay;
    Vec3f* originWork;
    Vec3f impactPos;
    Vec3f impactNormal;
    u32 bulletGFX;
    bool awaitingImpact;
public:
    bool isActive;
    bool isAbove;
    bool isFinal;
    grCell* grCellRef;
    grCellBullet(const char* taskName) : grMadein(taskName)
    {
        hitboxDelay = 0.0;
        hitboxDuration = 0.0;
        impactDelay = 0.0;
        originWork = NULL;
        impactPos = Vec3f(0,0,0);
        isActive = false;
        isAbove = false;
        awaitingImpact = false;
        isFinal = false;
        bulletGFX = NULL;
    };
    virtual ~grCellBullet(){};

    virtual void update(float deltaFrame);
    virtual void updateTravel(float deltaFrame);
    virtual void updateImpact(float deltaFrame);
    virtual void startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType);
    virtual void setupAttack();
    virtual void shoot();
    virtual bool calcBulletIntersect(Vec3f* start, Vec3f* end, Vec3f* min, Vec3f* max, bulletCalc* outCalc);
    virtual void setOriginWork(Vec3f* work) { originWork = work; };

    static grCellBullet* create(int mdlIndex, const char* tgtNodeName, const char* taskName);

};
