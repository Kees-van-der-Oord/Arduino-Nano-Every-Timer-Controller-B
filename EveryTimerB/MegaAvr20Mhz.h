#if !defined(MegaAvr20MHz_h)
// Quick hack to correct the millis() and micros() functions for 20MHz MegaAVR boards.
// by Kees van der Oord <Kees.van.der.Oord@inter.nl.net>

#if defined(ARDUINO_ARCH_MEGAAVR) && (F_CPU == 20000000UL)
// in the IDE 1.8.5 the implementation of millis() and micros() is not accurate
// for the MegaAvr achitecture board clocked at 20 MHz:
// 1)
// in ~\Arduino15\packages\arduino\hardware\megaavr\1.8.5\cores\arduino\wiring.c(386)
// microseconds_per_timer_overflow is initialized as:
// microseconds_per_timer_overflow = clockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF);
// this evaluates to (256 * 64) / (20000000/1000000)) = 819.2 which is rounded 819.
// the rounding causes millis() and micros() to report times that are 0.2/819.2 = 0.024 % too short 
// 2)
// in ~\Arduino15\packages\arduino\hardware\megaavr\1.8.5\cores\arduino\wiring.c(387)
// microseconds_per_timer_tick is defined as:
// microseconds_per_timer_tick = microseconds_per_timer_overflow/TIME_TRACKING_TIMER_PERIOD;
// which evaluates to 819.2 / 255 = 3.21254901960784 which is rounded to 3
// this is wrong in two ways:
// - the TIME_TRACKING_TIMER_PERIOD constant is wrong: this should be TIME_TRACKING_TICKS_PER_OVF
//   so the correct value is 3.2 ns/tick
// - the rounding causes micros() to return times that are 0.2/3 = 6.25 % too short
// as a quick hack, initialize these variables with settings a factor 5 larger
// and redefine the millis() and  micros() functions to return the corrected values

// from wiring.c:
extern volatile uint16_t microseconds_per_timer_overflow;
extern volatile uint16_t microseconds_per_timer_tick;
extern uint16_t millis_inc;
extern uint16_t fract_inc;

// call this method from your sketch if you include this file !
inline void corrected20MHzInit(void) {
	if(microseconds_per_timer_tick == 16) return;
	// for micros()
	microseconds_per_timer_tick     = 16;   // 5 * 3.2
	microseconds_per_timer_overflow = 4096; // 5 * 819.2
	// for millis()
	fract_inc = 96; // (5 * 819.2) % 1000
	millis_inc = 4; // (5 * 819.2) / 1000
}
inline unsigned long corrected20MHzMillis() { //__attribute__((always_inline)) {
	return millis() / 5;
}
inline unsigned long corrected20MHzMicros() { //__attribute__((always_inline)) {
	return micros() / 5;
}
#define millis corrected20MHzMillis
#define micros corrected20MHzMicros

#endif defined(ARDUINO_ARCH_MEGAAVR) && (F_CPU == 20000000UL)

#endif !defined(MegaAvr20MHz_h)