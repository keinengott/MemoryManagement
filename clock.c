#include "clock.h"
#include <stdio.h>

#define ONE_BILLION 1000000000

Clock addClock(Clock c1, Clock c2)
{
    Clock out = {
        .seconds = 0,
        .nanoseconds = 0
    };
    out.seconds = c1.seconds + c2.seconds;
    incrementClock_ns(&out, c1.nanoseconds + c2.nanoseconds);
    return out;
}

int clocksAreEqual(Clock c1, Clock c2)
{
    if (c1.seconds > c2.seconds) return 1;
    if ((c1.seconds == c2.seconds) && (c1.nanoseconds > c2.nanoseconds)) return 1;
    if ((c1.seconds == c2.seconds) && (c1.nanoseconds == c2.nanoseconds)) return 0;
    return -1;
}

void incrementClock_ns(Clock* clock, int increment)
{
    clock->nanoseconds += increment;
    if (clock->nanoseconds >= ONE_BILLION)
    {
        clock->seconds += 1;
        clock->nanoseconds -= ONE_BILLION;
    }
}


long double getSeconds(Clock c)
{
    long double seconds = c.seconds;
    long double nanoseconds = (long double)c.nanoseconds / ONE_BILLION; 
    seconds += nanoseconds;
    return seconds;
}

Clock clockFromSeconds(long double seconds)
{
    Clock clk = { .seconds = (int)seconds };
    seconds -= clk.seconds;
    clk.nanoseconds = seconds * ONE_BILLION;
    return clk;
}

Clock clockDiff(Clock c1, Clock c2)
{
    long double result = getSeconds(c1) - getSeconds(c2);
    return clockFromSeconds(result);
}

void printClock(char* name, Clock clk)
{
    printf("%-15s: %'ld:%'ld\n", name, clk.seconds, clk.nanoseconds);
}

void reset_clock(Clock* clk)
{
    clk->seconds = 0;
    clk->nanoseconds = 0;
}

Clock getClock()
{
    Clock out = {
        .seconds = 0,
        .nanoseconds = 0
    };
    return out;
}
