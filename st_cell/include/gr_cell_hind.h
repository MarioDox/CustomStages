#pragma once

#include "gr_cellsearch.h"
#include <gr/gr_yakumono.h>
#include <st/st_melee.h>

class grCellHind : public grYakumono {
protected:
    snd3DGenerator soundGenerator;
    int loopSFX;
    float gfxTimer;
    bool didSFX;
    bool didLoopSFX;
public:
    enum hindState {
        OFF,
        ARRIVING,
        ACTIVE,
        RETREATING,
    };
    grCellSearch* grCellSearchRef;
    hindState state;
    grCellHind(const char* taskName) : grYakumono(taskName)
    {
        didSFX = false;
        didLoopSFX = false;
        state = OFF;
        grCellSearchRef = NULL;
        gfxTimer = 0.0;
    };
    virtual ~grCellHind(){};
    virtual void startup(gfArchive* archive, u32 unk1, gfSceneRoot::LayerType layerType);
    virtual void update(float deltaFrame);
    virtual void startActivate();

    static grCellHind* create(int mdlIndex, const char* tgtNodeName, const char* taskName);

};
