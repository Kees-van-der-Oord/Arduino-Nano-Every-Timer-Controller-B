// EveryTimerB library.
// by Kees van der Oord Kees.van.der.Oord@inter.nl.net

// Timer library for the TCB timer of the AtMega4809 processor.
// tested on the Arduino Nano Every (AtMega4809) and the Arduino 1.8.12 IDE
// support for the Every is the 'Arduino MegaAVR' boards module (Tools | Board | Boards Manager)

// usage:
/*
#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB0    // use TimerB0 as a drop in replacement for Timer1
#else // assume architecture supported by TimerOne library ....
#include "TimerOne.h"
#endif

// code below will now work both on the MegaAVR and AVR processors

void setup() {
  Timer1.initialize();
  Timer1.attachInterrupt(myisr);
  Timer1.setPeriod(1000000);     // like the TimerOne library this will start the timer as well
}

void myisr() {
  // do something useful every second	
}
*/
// clock source options:
// The TCB clock source is specified in the initialize() function with default value EveryTimerB_CLOCMODE.
// define this macro before including this file to use a different default clock mode
// e.g.:
// #define EveryTimerB_CLOCMODE TCB_CLKSEL_CLKTCA_gc  // 250 kHz ~ 4 us
// #define EveryTimerB_CLOCMODE TCB_CLKSEL_CLKDIV2_gc //   8 MHz ~ 0.125 us
// #define EveryTimerB_CLOCMODE TCB_CLKSEL_CLKDIV_gc  //  16 MHz ~ 0.0625 us  

// timer options
// The 4809 has one A timer (TCA) and four B timers (TCB).
// TCA is used by the arduino core to generate the clock used by millis() and micros().
// By default Timer Control B0 is defined as TimerB0 in the EveryTimerB library
// If you would like to use the TCB1 and TCB2 as well you have to copy the code
// from the EveryTimerB.cpp into your product file and adapt for B1 and B2 timers.
// It looks like that the Arduino Core is using timer B3 (for what ?) so don't
// mess with that if you don't want to break something.
// Note that the Timer B also implements PWM on several pins, so there might be
// some conflicts there too.  
//
// for information on the 4809 TCA and TCB timers:
// http://ww1.microchip.com/downloads/en/AppNotes/TB3217-Getting-Started-with-TCA-90003217A.pdf
// http://ww1.microchip.com/downloads/en/Appnotes/TB3214-Getting-Started-with-TCB-90003214A.pdf
// %LOCALAPPDATA%\Arduino15\packages\arduino\hardware\megaavr\1.8.5\cores\arduino\wiring.c
// %LOCALAPPDATA%\Arduino15\packages\arduino\hardware\megaavr\1.8.5\variants\nona4809\variant.c
// %LOCALAPPDATA%\Arduino15\packages\arduino\tools\avr-gcc\7.3.0-atmel3.6.1-arduino5\avr\include\avr\iom4809.h

// 20 MHz system clock
// to run the Every at 20 MHz, add the lines below to the nona4809 section of the boards.txt file
// in %LOCALAPPDATA%\Arduino15\packages\arduino\hardware\megaavr\1.8.5.
// they add the sub menu 'Tools | Clock' to choose between 16MHz and 20MHz.
/*
menu.clock=Clock
nona4809.menu.clock.16internal=16MHz
nona4809.menu.clock.16internal.build.f_cpu=16000000L
nona4809.menu.clock.16internal.bootloader.OSCCFG=0x01
nona4809.menu.clock.20internal=20MHz
nona4809.menu.clock.20internal.build.f_cpu=20000000L
nona4809.menu.clock.20internal.bootloader.OSCCFG=0x02
*/
// On 20Mhz, the 1.8.12 IDE MegaAvr core library implementation
// of the millis() and micros() functions is not accurate.
// the file "MegaAvr20MHz.h" implements a quick hack to correct for this 
//
// to do:
// there is no range check on the 'period' arguments of setPeriod ...
// check if it is necessary to set the CNT register to 0 in start()
// add PWM support

#ifndef EveryTimerB_h_
#define EveryTimerB_h_
#ifdef ARDUINO_ARCH_MEGAAVR

#ifndef EveryTimerB_CLOCMODE 
#define EveryTimerB_CLOCMODE TCB_CLKSEL_CLKTCA_gc
#endif

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "pins_arduino.h"

#include "MegaAvr20MHz.h"

#define TCB_RESOLUTION 65536UL // TCB is 16 bit
// CLOCK   F_CPU  DIV  TICK      OVERFLOW  OVERFLOW/s
// CLKTCA  16MHz  64   4000  ns  262144us    3.8 Hz
// CLKDIV2 16MHz   2    125  ns    8192us  122 Hz
// CLKDIV1 16MHz   1     62.5ns    4096us  244 Hz
// CLKTCA  20MHz  64   3200  ns  209715us    4.8 Hz
// CLKDIV2 20MHz   2    100  ns    6554us  153 Hz
// CLKDIV1 20MHz   1     50  ns    3277us  305 Hz

class EveryTimerB
{
  public:
    // The AtMega Timer Control B clock sources selection:
    // TCB_CLKSEL_CLKTCA_gc,  // Timer A, Arduino framework sets TCA to F_CPU/64 = 250kHz (4us) @ 16MHz or 312.5kHz (3.2us) @ 20MHz
    // TCB_CLKSEL_CLKDIV2_gc, // CLK_PER/2 Peripheral Clock / 2: 8MHz @ 16Mhz or 10MHz @ 20MHz 
    // TCB_CLKSEL_CLKDIV1_gc  // CLK_PER Peripheral Clock: 16MHz @ 16Mhz or 20MHz @ 20MHz 

    // intialize: sets the timer compare mode and the clock source
    void initialize(TCB_t * timer_ = &TCB0, TCB_CLKSEL_t clockSource = EveryTimerB_CLOCMODE, unsigned long period = 1000000UL) __attribute__((always_inline)) {
#if defined(ARDUINO_ARCH_MEGAAVR) && (F_CPU == 20000000UL)
		corrected20MHzInit(); // see commment in MegaAvr20MHz_h
#endif        
      timer = timer_;
      timer->CTRLB = TCB_CNTMODE_INT_gc; // Use timer compare mode
      isrCallback = isrDefaultUnused;
      if(clockSource) setClockSource(clockSource);
      if(period) setPeriod(period);
    }
    
    void setClockSource(TCB_CLKSEL_t clockSource) __attribute__((always_inline)) {
      timer->CTRLA = clockSource; 
    }

    // setPeriod: sets the period
    // note: max and min values are different for each clock 
    // CLKTCA: conversion from us to ticks is a /3 division, so max value is 4.2G us (~ 1 hour, 11 minutes)
    // CLKDIV2: conversion from us to ticks is a *10 multiplication, so max value is 420M us (~ 7 minutes)
    // CLKDIV1: conversion from us to ticks is a *20 multiplication, so max value is 210M us (~ 3.5 minutes)
    void setPeriod(unsigned long period /* us */) __attribute__((always_inline)) {
      timer->CTRLA &= ~TCB_ENABLE_bm;
      // conversion from us to ticks depends on the clock
      switch(timer->CTRLA & TCB_CLKSEL_gm)
      {
        case TCB_CLKSEL_CLKTCA_gc:
#if F_CPU == 20000000UL
          period = (period * 10) / 32;
#else // 16000000UL
          period /= 4;
#endif        
          break;
        case TCB_CLKSEL_CLKDIV2_gc:
#if F_CPU == 20000000UL
          period *= 10;
#else // 16000000UL
          period *= 8;
#endif        
          break;
        case TCB_CLKSEL_CLKDIV1_gc:
#if F_CPU == 20000000UL
          period *= 20;
#else // 16000000UL
          period *= 16;
#endif        
          break;
      }
      period -= 1; // you always get one timer tick for free ...
      overflowCounts = period / TCB_RESOLUTION;
      remainder = period % TCB_RESOLUTION;
      start();
  }
    
    void start() __attribute__((always_inline)) {
      timer->CTRLA &= ~TCB_ENABLE_bm;
      overflowCounter = overflowCounts;
      timer->CNT = 0;
      timer->CCMP = overflowCounts ? (TCB_RESOLUTION - 1) : remainder;
      timer->CTRLA |= TCB_ENABLE_bm;
    }
    
    void stop() __attribute__((always_inline)) {
      timer->CTRLA &= ~TCB_ENABLE_bm;
    }
    
    bool isEnabled(void) {
      return timer->CTRLA & TCB_ENABLE_bm ? true : false;
    }

    void attachInterrupt(void (*isr)()) __attribute__((always_inline)) {
      isrCallback = isr;
	  timer->INTFLAGS = TCB_CAPT_bm; // clear interrupt request flag
      timer->INTCTRL = TCB_CAPT_bm;  // Enable the interrupt
    }
    
    void attachInterrupt(void (*isr)(), unsigned long microseconds) __attribute__((always_inline)) {
      if(microseconds > 0) stop();
      attachInterrupt(isr);
      if (microseconds > 0) setPeriod(microseconds);
    }
    
    void detachInterrupt() __attribute__((always_inline)) {
      timer->INTCTRL &= ~TCB_CAPT_bm; // Disable the interrupt
      isrCallback = isrDefaultUnused;
    }

    TCB_t * getTimer() { return timer; }
    long getOverflowCounts() { return overflowCounts; }
	  long getRemainder() { return remainder; }
	  long getOverflowCounter() { return overflowCounter; }
	
//protected:
    // the next_tick function is called by the interrupt service routine TCB0_INT_vect
	  //friend extern "C" void TCB0_INT_vect() __attribute__((interrupt));
	  //friend extern "C" void TCB1_INT_vect() __attribute__((interrupt));
	  //friend extern "C" void TCB2_INT_vect() __attribute__((interrupt));
    void next_tick() __attribute__((always_inline)) {
      --overflowCounter;
      if (overflowCounter < 0) {
        if (overflowCounts) {
          // restart with a max counter
          overflowCounter = overflowCounts;
          timer->CTRLA &= ~TCB_ENABLE_bm;
          //timer->CNT = 0;
          timer->CCMP = (TCB_RESOLUTION  - 1);
          timer->CTRLA |= TCB_ENABLE_bm;
		    }
        (*isrCallback)();
        return;
      }
      if (overflowCounter == 0) {
        // the max counter series has finished: to the remainder if any
        if(remainder) {
          timer->CTRLA &= ~TCB_ENABLE_bm;
          //timer->CNT = 0;
          timer->CCMP = remainder;
          timer->CTRLA |= TCB_ENABLE_bm;
        } else {
          // no remainder series: reset the counter and do the callback
          overflowCounter = overflowCounts;
          (*isrCallback)();
        }
      }
    }

private:
    TCB_t * timer;
    long overflowCounts;
    long remainder;
    long overflowCounter;
    void (*isrCallback)();
    static void isrDefaultUnused();
	
}; // EveryTimerB

extern EveryTimerB TimerB0;

#endif ARDUINO_ARCH_MEGAAVR
#endif EveryTimerB_h_
