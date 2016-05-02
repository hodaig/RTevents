/*
 * RTsched.h
 *
 *  Created on: Apr 29, 2016
 *      Author: hodai
 */

#ifndef _RTSCHED_H_
#define _RTSCHED_H_

// TODO
#ifdef ARDUINO
#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif
#include "MsTimer2.h"
#else

typedef unsigned short uint8_t;
typedef unsigned int uint16_t;
#define millis() (0)
void memset(void*,char,int);
class MsTimer2{
public:
void set(unsigned long ms, void (*f)());
    void start();
    void stop();
};

#endif

/* constants */
#define RT_MAX_DELAY        0xFFFF      // 65536 ~~ 1hour
#define RT_MAX_PERIOD       0xFFFF      // 65536 ~~ 1hour
#define RT_QUEUE_SIZE          20

/* public types */
typedef void (*RTfunc) (void);

/* internal tasks struct */
typedef struct {
    RTfunc func;                      //void (*func) (void);
    unsigned long nextOccurrence;
    uint8_t flags;                    // see:  RT_FLAG_...
    uint16_t period;                  // time in milliseconds before every occurrence
} RTtask;

class RTsched {
private:
    static RTtask _tasksQueue[RT_QUEUE_SIZE];
    static unsigned long _curentNextIntrerupt;
    static bool _interuptIsActivale;

public:

    static void begin();

    static uint8_t addTask(RTfunc func, long delay, uint16_t period);

    static bool removeTask(uint8_t taskID);

private:

    static void RTattachInterrupt(unsigned long delay);

    static void RTdeAttachInterrupt();

    static void RTinteruptHandler();

    static void RTexecuteTask(RTtask* theTask);

};



#endif /* ARDUINO_LIBS_RTEVENTS_RTSCHED_H_ */
