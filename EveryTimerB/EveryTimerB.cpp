#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"

void EveryTimerB::isrDefaultUnused(void) {}

// code timer B0. For B1 and B2 copy this code and change the '0' to '1' and '2'
EveryTimerB TimerB0;
ISR(TCB0_INT_vect)
{
  TimerB0.next_tick();
  TCB0.INTFLAGS = TCB_CAPT_bm;
}

#endif ARDUINO_ARCH_MEGAAVR
