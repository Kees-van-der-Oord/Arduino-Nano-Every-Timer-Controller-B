# Arduino EveryTimerB library.
// a drop in replacement for the TimerOne library for the Nano Every
// https://github.com/Kees-van-der-Oord/Arduino-Nano-Every-Timer-Controller-B

// EveryTimerB is a library for the TCB timer of the AtMega4809 processor.
// tested on the Arduino Nano Every (AtMega4809) and the Arduino 1.8.12 IDE
// support for the Every is the 'Arduino MegaAVR' boards module (Tools | Board | Boards Manager)

// usage:
/*
#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB2    // use TimerB2 as a drop in replacement for Timer1
#else // assume architecture supported by TimerOne ....
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
// By default Timer Control B2 is defined as TimerB2 in the EveryTimerB library
// TCB0 and TCB1 control PWM of pins 6 and 3. If you don't use PWM on these pins
// you can define a TimerB object for TCB0 and TCB1 and copy the ISR code from the
// EveryTimerB.cpp file.
// The Arduino Core is using timer B3 for millis() and micros() so don't mess with
// TCB3 if you don't want to break the functionality of these functions.
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
// On 20Mhz, the milis() and micros() implementation of the arduino core is inaccurate.
// The functions in MegaAvr20Mhz.h correct for that.
//
// to do:
// there is no range check on the 'period' arguments of setPeriod ...
