#ifndef _LAZY_FOO_TIMER_H_
#define _LAZY_FOO_TIMER_H_

#include <SDL2/SDL.h>

class LazyFooTimer
{
public:
    //Initializes variables
    LazyFooTimer();

    //The various clock actions
    void start();
    void stop();
    void pause();
    void unpause();

    //Gets the timer's time
    Uint32 getTicks();

    //Checks the status of the timer
    bool isStarted();
    bool isPaused();

private:
    //The clock time when the timer started
    Uint32 mStartTicks;

    //The ticks stored when the timer was paused
    Uint32 mPausedTicks;

    //The timer status
    bool mPaused;
    bool mStarted;
};

#endif