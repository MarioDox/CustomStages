#pragma once

#include <StaticAssert.h>
#include <types.h>

// Brawl doesn't have these from what I've seen
class Tween {
public:
    enum easeType {
        EASE_LINEAR,
        EASE_QUAD,
        EASE_UNK,
    };
    float startValue;
    float delta;
    float duration;
    float elapsed;
    float delay;
    easeType easeMode;
    bool isEnd;

    Tween() { }

    void init()
    {
        startValue = 0.0;
        delta = 0.0;
        duration = 0.0;
        elapsed = 0.0;
        delay = 0.0; // original code doesn't initialize it
        easeMode = EASE_LINEAR;
        isEnd = false;
    }
    
    void start(float from, float to, float dur, float del)
    {
        startValue = from;
        delta = to-from;
        duration = dur;
        if (dur < 0) { dur = 0; }
        elapsed = 0;
        delay = del;
        isEnd = false;
    }

    void advance(float dt)
    {
        elapsed+=dt/60.0; // added /60 as origin code expects dt as seconds, not frames
        if (elapsed-delay >= duration) {
            isEnd = true;
        }
    }

    float getValue()
    {
        // didn't start yet!
        if (elapsed < delay) {
            return startValue;
        }
        if (duration < 0.0) {
            return startValue+delta;
        }
        float time = (elapsed-delay)/duration;
        float k;
        if (time > 1.0) {
            k = 1.0;
        } else if (easeMode == EASE_QUAD) {
            k = time*time;
        } else if (easeMode == EASE_UNK) {
            k = 1.0-time*time;
        } else {
            k = time;
        }
        return startValue+delta*k;
    }
};
static_assert(sizeof(Tween) == 28, "Class is wrong size!");