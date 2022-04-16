#ifndef CLOCK_H
#define CLOCK_H

struct clock {
    unsigned long seconds;
    unsigned long nanoseconds;
};
typedef struct clock Clock;

Clock addClock(Clock c1, Clock c2);
Clock clockFromSeconds(long double seconds);
Clock clockDiff(Clock c1, Clock c2);
Clock getClock();
void incrementClock_ns(Clock* clock, int increment);
void printClock(char* name, Clock clk);
void reset_clock(Clock* clk);
int clocksAreEqual(Clock c1, Clock c2);
long double getSeconds(Clock c);

#endif
