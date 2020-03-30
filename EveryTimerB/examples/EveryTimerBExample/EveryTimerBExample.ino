// EveryTimerBExample.ino
// Author: Kees van der Oord <Kees.van.der.Oord@inter.nl.net>
// see comments in "~/Arduino/libraries/EveryTimerB/EveryTimerB.h for more info

#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB2    // use TimerB2 as a drop in replacement for Timer1
#else
#include "TimerOne.h"
#endif

#define BAUD_RATE 115200

#if defined(EveryTimerB_h_)
// the ATMega4809 has three Timers B: TCB0, TBC1, TBC2
// TCB0 and TCB1 are used to generate PWM on the pins 3, 5, 6, 9 and 10
// TCB2 is selected here by default to generate timer interupts

// the TimerB2 declaration is in the library cpp file
// can test also timers B0 and B1
// cycle through these timers with the 't' command
EveryTimerB   TimerB0;
EveryTimerB   TimerB1;
EveryTimerB * timers[3] = {&TimerB0,&TimerB1,&TimerB2};
const char *  timerNames[5] = {"TCB0","TCB1","TCB2","TCA","???"};
byte          timerIndex = 2;
EveryTimerB * current_timer = &TimerB2;
#define Timer1 (*current_timer)

// For the timers you can select three clocks: CLCKTA, CLCKDIV2, CLCKDIV1
// the clock frequency depends on the system clock frequency (20MHz or 16 MHz)
// CLOCK   F_CPU  DIV  TICK      OVERFLOW  OVERFLOW/s
// CLKTCA  16MHz  64   4000  ns  262144us    3.8 Hz
// CLKDIV2 16MHz   2    125  ns    8192us  122 Hz
// CLKDIV1 16MHz   1     62.5ns    4096us  244 Hz
// CLKTCA  20MHz  64   3200  ns  209716us    4.8 Hz
// CLKDIV2 20MHz   2    100  ns    6554us  153 Hz
// CLKDIV1 20MHz   1     50  ns    3277us  305 Hz
// you can cycle through these clocks with the 'c' comamnd
TCB_CLKSEL_t clocks[3] = {TCB_CLKSEL_CLKTCA_gc,TCB_CLKSEL_CLKDIV2_gc,TCB_CLKSEL_CLKDIV1_gc};
const char * clockNames[3] = {"CLKTCA","CLKDIV2","CLKDIV1"};
byte         clockIndex = 0;

#endif defined(EveryTimerB_h_)

// store current settings
unsigned long period = 1000000UL;
long          periodIndex = 0;
bool          clockEnabled = false;
bool          stutter = false;
bool          concurtest = false; // concurrency test: call micros() in loop

// measure the timing to the calls to the isr
volatile unsigned long isr_counter = 0;
unsigned long          last_tick_us = 0;
unsigned long          last_tick_ms = 0;
bool                   showMicros = true;

#define SHOW_PERIOD_LIMIT 100000UL   

// myisr is pass the TimerB objects as callback function when the timer expires
void myisr(void)
{
  
  if(period < SHOW_PERIOD_LIMIT) {
    // above 10 Hz, just count the number of interrupts per second
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
    if(stutter) {
      Timer1.stop();
      // The stutter_delay is the execution time of the code between calling micros() here 
      // and above at the next isr call.
      // at 20MHz it is much longer due the expensive implementation of micros()
#if (F_CPU == 20000000UL)
#define STUTTER_DELAY 48
#else
#define STUTTER_DELAY 16
#endif
      Timer1.setPeriod((last_tick_us + period) - micros() - STUTTER_DELAY); 
    }
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
void setPeriod(unsigned long newPeriod) {
  period = newPeriod;
  enableClock(true);  
#if defined(EveryTimerB_h_)
  Serial.print("overflowCounts "); Serial.print(Timer1.getOverflowCounts()); Serial.print(" remainder "); Serial.println(Timer1.getRemainder());
#endif defined(EveryTimerB_h_)
}

#if defined(EveryTimerB_h_)

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
  Timer1.stop();
  setPeriod(Timer1.getOverflowTime());
}

void setStutter(bool on) {
  stutter = on;
  showMicros = true;
  Serial.print("stutter: "); Serial.println(on?"on":"off");
}

void setCallMicrosInLoop(bool on) {
  concurtest = on;
  showMicros = true;
  Serial.print("call micros() in loop: "); Serial.println(on?"on":"off");
}

void showTimersOfPwmPin(int pin) {
  int timer = digitalPinToTimer(pin);
  timer -= 2; // convert to index in timerNames[];
  if(timer == -1) timer = 3;
  if((timer < 0) || (timer > 3)) timer = 4;
  Serial.print("pin "); Serial.print(pin); Serial.print(" timer: "); Serial.println(timerNames[timer]);
}

void showTimersOfPwmPins() {
  showTimersOfPwmPin(3);
  showTimersOfPwmPin(5);
  showTimersOfPwmPin(6);
  showTimersOfPwmPin(9);
  showTimersOfPwmPin(10);
}

// pin 6 pwm is done by TCB0
// pin 3 pwm is done by TCB1
// pins 5, 9, 10 done by TCA
void setToPwm(byte pin) {
  uint8_t timer_index = digitalPinToTimer(pin);
  if((timer_index >= TIMERB0) && (timer_index < TIMERB3)) {
    // must be pin 3 or 6
    timer_index -= TIMERB0;
    EveryTimerB * timer = timers[timer_index];
    if(timer->getMode() == TCB_CNTMODE_PWM8_gc) {
      Serial.print("timer "); Serial.print(timerNames[timer_index]); Serial.println(" set to timer compare mode");
      timer->setTimerMode();
      if(timer == current_timer) {
        setTimer(timer_index);
      }
      return;
    }
    Serial.print("timer "); Serial.print(timerNames[timer_index]); Serial.println(" set to PWM mode");
    timer->setPwmMode();
  }
  Serial.print("analogWrite("); Serial.print(pin); Serial.println(",60);");
  analogWrite(pin,60);
}

#endif defined(EveryTimerB_h_)

// setup
void setup() {
  Serial.begin(BAUD_RATE);

  Serial.print("commands: +: faster, -: slower, e: enable/disable");
#if defined(EveryTimerB_h_)
  Serial.print(" c: change clock source, t: change timer"); 
#endif defined(EveryTimerB_h_)
  Serial.println("");
  Serial.print("F_CPU  : "); Serial.println(F_CPU);

#if defined(EveryTimerB_h_)
  TimerB0.initialize(&TCB0);
  TimerB0.stop();
  TimerB0.attachInterrupt(myisr);
  TimerB1.initialize(&TCB1);
  TimerB1.stop();
  TimerB1.attachInterrupt(myisr);
  TimerB2.initialize(&TCB2);
  TimerB2.stop();
  TimerB2.attachInterrupt(myisr);
  // TCB3: used by arduino core for micros()
  showTimersOfPwmPins();
  setTimer(2);
#else defined(EveryTimerB_h_)
  Timer1.initialize();
  Timer1.attachInterrupt(myisr);
  setPeriodIndex(periodIndex);
#endif !defined(EveryTimerB_h_)
}

// use volatile to prevent the compiler to remove assignment of
// this variable that is not used anywhere else
volatile unsigned long concur_micros; // for concurtest (call micros in loop)

void loop() {
  if(concurtest) {
    concur_micros = micros();
  }
  if(period < SHOW_PERIOD_LIMIT) {
    long diff = next_tick - millis();
    if(diff < 0) {
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
      case '-': setPeriodIndex(periodIndex+1);     break; // slower
      case '+': setPeriodIndex(periodIndex-1);     break; // faster
      case 'e': enableClock(!clockEnabled);        break; // stop clock
      case 'm': showMicros = !showMicros;          break; // change micros <--> millis
      case 'u': setPeriod(period+4);               break; // period up
      case 'U': setPeriod(period+1);               break; // period up
      case 'd': setPeriod(period-4);               break; // period down
      case 'D': setPeriod(period-1);               break; // period down
      case 's': setStutter(!stutter);              break;
      case 'l': setCallMicrosInLoop(!concurtest);  break;
#if defined(EveryTimerB_h_)
      case 'c': setClock(clockIndex+1);            break; // switch clock
      case 't': setTimer(timerIndex+1);            break; // switch clock
      case 'r': roundPeriod();                     break; // set remainder to 0
      case 'o': matchOverflow();                   break; // no overflow
      case '3': setToPwm(3);                       break; // no overflow
      case '5': setToPwm(5);                       break; // no overflow
      case '6': setToPwm(6);                       break; // no overflow
      case '9': setToPwm(9);                       break; // no overflow
      case '0': setToPwm(10);                      break; // no overflow
#endif defined(EveryTimerB_h_)
    }
  }
}

#if defined(EveryTimerB_h_)

// code timer B0
ISR(TCB0_INT_vect)
{
  TimerB0.next_tick();
  TCB0.INTFLAGS = TCB_CAPT_bm; // writing to the INTFLAGS register clears the interrupt request flag
}

// code timer B1
ISR(TCB1_INT_vect)
{
  TimerB1.next_tick();
  TCB1.INTFLAGS = TCB_CAPT_bm; // writing to the INTFLAGS register clears the interrupt request flag
}

// for the TimerB2, this code is in the library cpp file

#endif defined(EveryTimerB_h_)
