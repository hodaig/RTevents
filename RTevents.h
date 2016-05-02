/*
 * RTsched.h
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

/* constants */
#define RT_MAX_DELAY        0xFFFF      // 65536 ms ~~ 1hour
#define RT_MAX_PERIOD       0xFFFF      // 65536 ms ~~ 1hour

// change this to the expected maximum number of task at the same time
// each task take sizeof(RTtask) bytes of RAM (9byte)
#define RT_QUEUE_SIZE          20		


/* public types */
typedef void (*RTfunc) (void);


/* internal tasks struct */
typedef struct {
    RTfunc func;                      // void (*func) (void);
    unsigned long nextOccurrence;     // the next time this task will execute
    uint8_t flags;                    // see:  RT_FLAG_...
    uint16_t period;                  // time in milliseconds before every occurrence
} RTtask;


class RTevents {
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

#endif /* _RTEVENTS_H_ */
