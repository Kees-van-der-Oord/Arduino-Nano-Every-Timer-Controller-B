// EveryTimerBExample.ino
// Author: Kees van der Oord <Kees.van.der.Oord@inter.nl.net>
// see comments in "libraries/EveryTimerB/EveryTimerB.h for more info

#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB0    // use TimerB0 as a drop in replacement for Timer1
#else
#include "TimerOne.h"
#endif

#define BAUD_RATE 115200

#ifdef ARDUINO_ARCH_MEGAAVR

// can cycle through these clocks with the 'c' comamnd
TCB_CLKSEL_t clocks[3] = {TCB_CLKSEL_CLKTCA_gc,TCB_CLKSEL_CLKDIV2_gc,TCB_CLKSEL_CLKDIV1_gc};
const char * clockNames[3] = {"CLKTCA","CLKDIV2","CLKDIV1"};
byte clockIndex = 0;

// can test also timers B1 and B2
// the TimerB0 declaration is in the library cpp file
EveryTimerB TimerB1;
EveryTimerB TimerB2;
EveryTimerB * timers[3] = {&TimerB0,&TimerB1,&TimerB2};
const char *  timerNames[3] = {"TimerB0","TimerB1","TimerB2"};
byte timerIndex = 0;
EveryTimerB * current_timer = &TimerB0;
#define Timer1 (*current_timer)

#endif ARDUINO_ARCH_MEGAAVR

// store current settings
unsigned long period = 1000000UL;
long periodIndex = 0;
bool clockEnabled = false;

// measure the timing to the calls to the isr
volatile unsigned long isr_counter = 0;
unsigned long last_tick_us = 0;
unsigned long last_tick_ms = 0;

bool showMicros = true;

#define SHOW_PERIOD_LIMIT 100000UL   // above 10 Hz, just count the number of interrupts per second
void myisr(void)
{
  if(period < SHOW_PERIOD_LIMIT) {
    ++isr_counter;
    return;
  }

  // yes, I know an isr should be quick and not use Serial
  // but, this is a test and no other tasks are running
  // and, it is easily completed before the next interrupt comes
  unsigned long now;
  long dif;

  if(showMicros) {
    now = micros();
    dif = now - last_tick_us;
    last_tick_us = now;
    // output micros(), period since last tick and difference with expected period
    Serial.print(now);
    Serial.print("us, "); Serial.print(dif);
    dif = dif - period; // negative is too early: positive is too late
    Serial.print(", "); Serial.println(dif);
  } else {
    now = millis();
    dif = now - last_tick_ms;
    last_tick_ms = now;
    // output millis(), period since last tick and difference with expected period
    Serial.print(now);
    Serial.print("ms, "); Serial.print(dif);
    dif = dif - (period/1000); // negative is too early: positive is too late
    Serial.print(", "); Serial.println(dif);
  }
}

#define UPDATE_PERIOD 1000 // show the interrupt rate every second
unsigned long next_tick = millis() + UPDATE_PERIOD;
unsigned long prev_count = 0;

// start/stop the clock
void enableClock(bool on) {
  if(!on) {
    // disable
    Timer1.stop();
    clockEnabled = false;
    return;
  }
  // enable
  next_tick = millis() + UPDATE_PERIOD;
  prev_count = isr_counter;
  Serial.print("period : "); Serial.println(period);
  Timer1.setPeriod(period);
  last_tick_us = micros();
  last_tick_ms = millis();
  clockEnabled = true;
}

// set the period index
void setPeriodIndex(long newIndex) {
  periodIndex = newIndex;
  unsigned long period = 1000000UL;
  while(newIndex > 0) { period *= 2; --newIndex; }
  while(newIndex < 0) { period /= 2; ++newIndex; }
  setPeriod(period);
}

// set the period 
void setPeriod(unsigned long newPeriod)
{
  period = newPeriod;
  enableClock(true);  
#ifdef ARDUINO_ARCH_MEGAAVR
  Serial.print("overflowCounts "); Serial.print(Timer1.getOverflowCounts()); Serial.print(" remainder "); Serial.println(Timer1.getRemainder());
#endif ARDUINO_ARCH_MEGAAVR
}

#ifdef ARDUINO_ARCH_MEGAAVR

// cycle the clock source
void setClock(byte newClock) {
  Timer1.stop();
  clockIndex = newClock % 3;
  Serial.print("clock  : "); Serial.println(clockNames[clockIndex]);
  Timer1.setClockSource(clocks[clockIndex]);
  setPeriodIndex(periodIndex);
}

void setTimer(byte newTimer) {
  Timer1.stop();
  timerIndex = newTimer % 3;
  current_timer = timers[timerIndex];
  Serial.print("timer  : "); Serial.println(timerNames[timerIndex]);
  setClock(clockIndex);
}

// test to see if the timing is also correct if the remainder is 0
// only likely with the 250ns clock
// result: when the remainder is 0, the timer is 3 us too short
// conclusion: reprogramming the timer for the remainder/overflow costs 2 us.
void roundPeriod() {
  if(clockIndex != 0) return; // not possible with faster clocks sources
  Timer1.stop();
  long overflows = Timer1.getOverflowCounts();
  if(overflows == 0) return; // not possible with short periods
  while(overflows == Timer1.getOverflowCounts()) {
    --period;
    Timer1.setPeriod(period);
  }
  ++period;
  setPeriod(period);
}

void matchOverflow() {
  if(clockIndex != 0) return; // not possible with faster clocks sources
  Timer1.stop();
  setPeriod(Timer1.getOverflowTime()-1);
}

#endif ARDUINO_ARCH_MEGAAVR

// setup
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.print("commands: +: faster, -: slower, e: enable/disable");
#ifdef ARDUINO_ARCH_MEGAAVR
  Serial.print(" c: change clock source, t: change timer"); 
#endif
  Serial.println("");
  Serial.print("F_CPU  : "); Serial.println(F_CPU);
  //TimerB0.initialize(&TCB0,clocks[clockIndex],period);
  Timer1.initialize();
  Timer1.attachInterrupt(myisr);
#ifdef ARDUINO_ARCH_MEGAAVR
  setTimer(timerIndex);
  TimerB1.initialize(&TCB1);
  TimerB1.stop(); // on by default ?
  TimerB1.attachInterrupt(myisr);
  TimerB2.initialize(&TCB2);
  TimerB2.stop(); // on by default ?
  TimerB2.attachInterrupt(myisr);
  // TCB3: used by arduino core ????
#endif
}

void loop() {
  if(period < SHOW_PERIOD_LIMIT) {
    long diff = next_tick - millis();    if(diff < 0) {
      next_tick += UPDATE_PERIOD;
      unsigned long cnt = isr_counter;
      diff = cnt - prev_count;
      prev_count = cnt;
      // print out the number of interrupts and the expected number
      Serial.print("ticks: "); Serial.print(diff); Serial.print(" ?= "); Serial.println(1000000./(float)period) ; 
    }
  }
  if(Serial.available())
  {
    char c = Serial.read();
    switch(c)
    {
      case '-': setPeriodIndex(periodIndex+1);   break; // slower
      case '+': setPeriodIndex(periodIndex-1);   break; // faster
      case 'e': enableClock(!clockEnabled);      break; // stop clock
      case 'm': showMicros = !showMicros;        break; // change micros <--> millis
      case 'u': setPeriod(period+1);             break; // period up
      case 'd': setPeriod(period-1);             break; // period down
#ifdef ARDUINO_ARCH_MEGAAVR
      case 'c': setClock(clockIndex+1);          break; // switch clock
      case 't': setTimer(timerIndex+1);          break; // switch clock
      case 'r': roundPeriod();                   break; // set remainder to 0
      case 'o': matchOverflow();                 break; // no overflow
#endif
    }
  }
}

#ifdef ARDUINO_ARCH_MEGAAVR
// for the TimerB0, this code is in the library cpp file

// code timer B1
ISR(TCB1_INT_vect)
{
  TimerB1.next_tick();
  TCB1.INTFLAGS = TCB_CAPT_bm;
}

// code timer B2
ISR(TCB2_INT_vect)
{
  TimerB2.next_tick();
  TCB2.INTFLAGS = TCB_CAPT_bm;
}

#endif ARDUINO_ARCH_MEGAAVR
