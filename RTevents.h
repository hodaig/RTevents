/*
 * RTevents.h
 *
 *  Created on: Apr 29, 2016
 *      Author: hodai
 */

#ifndef _RTEVENTS_H_
#define _RTEVENTS_H_

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include "MsTimer2.h"

// un-commernt this to use less memory - TODO - experimental mode
// #define RT_LITE_MEM_MODE

/* constants */
#define RT_MAX_DELAY        0xFFFF      // 65536-ms ~~ 1-minute
#define RT_MAX_PERIOD       0xFFFF      // 65536-ms ~~ 1-minute

// change this to the expected maximum number of task at the same time
// each task take sizeof(RTtask) bytes of RAM (9byte)
#define RT_QUEUE_SIZE          20		


/* public types */
typedef void (*RTfunc_t) (void);


/* internal tasks struct */
typedef struct {
    RTfunc_t func;                      // void (*func) (void);
#ifdef RT_LITE_MEM_MODE
    uint16_t leftForNextOccur;        // the time left till the next execution
#else
    unsigned long nextOccurrence;     // the next time this task will execute
#endif
    uint8_t flags;                    // see:  RT_FLAG_...
    uint16_t period;                  // time in milliseconds before every occurrence
} RTtask_t;


class RTevents {
private:

    static RTtask_t _tasksQueue[RT_QUEUE_SIZE];

    static unsigned long _curentNextIntrerupt;

#ifdef RT_LITE_MEM_MODE
    static unsigned long _lastUpdate;
#endif

    static bool _interuptIsActivale;

public:

    static void begin();

#ifdef RT_LITE_MEM_MODE
    static uint8_t addTask(RTfunc_t func, uint16_t delay, uint16_t period);
#else
    static uint8_t addTask(RTfunc_t func, unsigned long delay, uint16_t period);
#endif

    static bool removeTask(uint8_t taskID);

private:

#ifdef RT_LITE_MEM_MODE
    static void RTattachInterrupt(uint16_t delay);
#else
    static void RTattachInterrupt(unsigned long delay);
#endif

    static void RTdetachInterrupt();

    static void RTinteruptHandler();

    static void RTexecuteTask(RTtask_t* theTask);
};

#endif /* _RTEVENTS_H_ */
