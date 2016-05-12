/*
 * RTtimer.h
 *
 *  Created on: May 9, 2016
 *      Author: hodai
 */

#ifndef RTTIMER_H_
#define RTTIMER_H_

#ifndef ISR
//#define ISR(...) void isr()
#endif


#define RT_TIMER_RESOLUTION 0xFFFF


#define RT_TIMER_CLI() RTtimer_oldSREG = SREG; cli();

#define RT_TIMER_STI() SREG = RTtimer_oldSREG;

uint8_t RTtimer_oldSREG;
void (* volatile RTtimer_isr)();

__inline__ void RTtimer_begin(){
    TCCR1A = 0;                 // clear control register A
    TCCR1B = 0;                 // timer disable
    TIMSK1 = _BV(OCIE1A);       // enable timer compare interrupt
}

__inline__ void RTtimer_attachInterrupt(void (*f)()){
    RTtimer_isr = f;
}

__inline__ void RTtimer_schedNext_us(unsigned long us){

    TCCR1B = 0;                                  // first disable the timer

    uint8_t clockSelectBits;

    if (us == 0) us = 1;

    long ocr = (F_CPU / 1000000) * us;                                          // num of tics needed
    if(ocr < RT_TIMER_RESOLUTION)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
    else if((ocr >>= 3) < RT_TIMER_RESOLUTION) clockSelectBits = _BV(CS11);              // prescale by /8
    else if((ocr >>= 3) < RT_TIMER_RESOLUTION) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
    else if((ocr >>= 2) < RT_TIMER_RESOLUTION) clockSelectBits = _BV(CS12);              // prescale by /256
    else if((ocr >>= 2) < RT_TIMER_RESOLUTION) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
    else        ocr = RT_TIMER_RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum

    RT_TIMER_CLI();                              // Disable interrupts for 16 bit register access
    OCR1A = ocr;
    TCNT1 = 0;                                   // start from 0
    TCCR1B = _BV(WGM12) | clockSelectBits;       // reset clock select register, and starts the clock
    RT_TIMER_STI();

}

__inline__ void RTtimer_stop(){
    TCCR1B = 0;
    OCR1A = 0xEFFF;
}

ISR(TIMER1_COMPA_vect) {
#ifdef RT_TIMER_AUTOSTOP
    RTtimer_stop();
#endif

    if (RTtimer_isr) {
        (RTtimer_isr)();
    }
}


#endif /* RTTIMER_H_ */
