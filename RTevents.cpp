/*
 * RTevents.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: hodai
 */

#include "RTevents.h"

#define RT_TIMER_AUTOSTOP
#include "RTtimer.h"

/* constants */
#define RT_MAX_MILLIS       0xFFFFFFFF  // ~50 days
#define RT_MAX_MICROS       0xFFFFFFFF  // ~70 minutes

/* flags */
#define RT_FLAG_ACTIVE     1      // without this flag the task is not scheduled
#define RT_FLAG_USED       2
#define RT_FLAG_ONE_TIME   4      // if set, this task will occur once and removed
#define RT_FLAG_OVERFLOW   8      // the next occurrence is after the counter is overflow ( > 2^32)

#define RT_FLAG_NESTED     32     // this task can be called nested - TODO - in development, don't use!

/* macros */
#define RT_FLAG_ADD(var, flag) var = (var | (flag))
#define RT_FLAG_DEL(var, flag) var = (var & ~flag)
#define RT_CHECK_FLAG(var, flag) (var & flag)

#define RT_CALC_NEXT_TIME(from, delay) ((unsigned long)((unsigned long)from + (unsigned long)delay))   // TODO - add check for overflow

#ifdef RT_UNITS_US
#  define RT_GET_TIME() micros()
#else
#  define RT_GET_TIME() millis()
#endif

#define RT_LOCK_TABLE()        cli();
#define RT_RELEASE_TABLE()     sei();

#ifndef MIN
#define MIN(a, b) ((a<b) ? a : b)
#endif

/* static variables */
RTtask_t RTevents::_tasksQueue[RT_QUEUE_SIZE];

volatile unsigned long RTevents::_curentNextIntrerupt;

#ifdef RT_LITE_MEM_MODE
unsigned long RTevents::_lastUpdate;
#endif

volatile bool RTevents::_interuptIsActivate;
    

void RTevents::begin(){
#ifdef RT_UNITS_US
    _curentNextIntrerupt = RT_MAX_MICROS;
#else
    _curentNextIntrerupt = RT_MAX_MILLIS;
#endif
    _interuptIsActivate = false;

#ifdef RT_LITE_MEM_MODE
    _lastUpdate = RT_GET_TIME();
#endif

    memset(&_tasksQueue, 0x0, sizeof(_tasksQueue));

    // define the timer and attach the interrupt handler
    RTtimer_begin();
    RTtimer_attachInterrupt(RTevents::RTinteruptHandler);

}

#ifdef RT_LITE_MEM_MODE
uint8_t RTevents::addTask(RTfunc_t func, uint16_t delay, uint16_t period){
#else
uint8_t RTevents::addTask(RTfunc_t func, uint32_t delay, uint32_t period){
#endif
    // basic validation
    if (delay > RT_MAX_DELAY || period > RT_MAX_PERIOD){
        return 0;
    }

    // search for free task cell
    for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
        if (!RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_USED)){
            // found! use it
            RT_FLAG_ADD(_tasksQueue[i].flags, RT_FLAG_USED);
            _tasksQueue[i].func = func;

            if (period > 0){
                // is continues task
                _tasksQueue[i].period = period;
                if(0 == delay){
                    // by default first occurrence will be after 1 period
                    delay = period;
                }
            } else {
                RT_FLAG_ADD(_tasksQueue[i].flags, RT_FLAG_ONE_TIME);
            }

#ifdef RT_LITE_MEM_MODE
            _tasksQueue[i].leftForNextOccur = delay + (RT_GET_TIME()-_lastUpdate);
#else
            _tasksQueue[i].nextOccurrence = RT_CALC_NEXT_TIME(RT_GET_TIME(), delay);
#endif

            RT_FLAG_ADD(_tasksQueue[i].flags, RT_FLAG_ACTIVE);
            // now this task can be scheduled

#ifdef RT_LITE_MEM_MODE
            if (0 == _curentNextIntrerupt || RT_CALC_NEXT_TIME(RT_GET_TIME(), delay) < _curentNextIntrerupt){
#else
            if (0 == _curentNextIntrerupt || _tasksQueue[i].nextOccurrence < _curentNextIntrerupt){
#endif
                RTattachInterrupt(delay);
            }

            return i;
        }
    }

    // no free cells!
    return 0;
}

bool RTevents::removeTask(uint8_t taskID){
    if (taskID < 0 || taskID >= RT_QUEUE_SIZE) {
        return false;
    }
    memset(&_tasksQueue[taskID], 0x0, sizeof(RTtask_t));
    return true;
}

#ifdef RT_LITE_MEM_MODE
void RTevents::RTattachInterrupt(uint16_t delay){
#else
void RTevents::RTattachInterrupt(unsigned long delay){
#endif

    _interuptIsActivate = true;
    _curentNextIntrerupt = RT_GET_TIME() + delay;

#ifdef RT_UNITS_US
    if (delay < RT_MIN_TIMER) delay = RT_MIN_TIMER;
    RTtimer_schedNext_us(delay);
#else
    RTtimer_schedNext_us(delay*1000);
#endif

}

void RTevents::RTdetachInterrupt(){

    _interuptIsActivate = false;
#ifdef RT_UNITS_US
    _curentNextIntrerupt = RT_MAX_MICROS;
#else
    _curentNextIntrerupt = RT_MAX_MILLIS;
#endif

    RTtimer_stop();

}

void RTevents::RTinteruptHandler(){
#ifdef RT_UNITS_US
    unsigned long nextInterrupt = RT_MAX_MICROS;
#else
    unsigned long nextInterrupt = RT_MAX_MILLIS;
#endif

    unsigned long localMicros;
    // TODO - make 'i' static to solve starvation problems
    // TODO - wrap all this with 'while' loop instead using recursion to solve stack overflow

    static volatile bool lock = false;
    if (lock){
        return;
    } else {
        lock = true;
    }
    // first stop the next interrupts
    //interrupts();           // release all the rest interrupts
    //RTdetachInterrupt();    // already done by RTtimer

    //while(true) {
        localMicros = RT_GET_TIME();
#ifdef RT_LITE_MEM_MODE
        unsigned long deltaTime= localMicros - _lastUpdate;
        // TODO - if negative millis overflow occur
#endif

        for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
            if (RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE)) {               /* is an active task */
#ifdef RT_LITE_MEM_MODE
                if(_tasksQueue[i].leftForNextOccur <= deltaTime) {                   /* need to be executed now */
#else
                if(_tasksQueue[i].nextOccurrence <= localMicros) {                   /* need to be executed now */
#endif
                    RTexecuteTask(&_tasksQueue[i]);
                }

                if (RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE)) {          /* is still activate */
                    nextInterrupt = MIN(nextInterrupt, _tasksQueue[i].nextOccurrence);
                }
            }
        }

#if 0
            // schedule the next interrupt
#ifdef RT_LITE_MEM_MODE
            RT_LOCK_TABLE();      // lock the tasks table
#else
        localMicros = RT_GET_TIME();
        nextInterrupt = RT_MAX_MICROS;
#endif

        for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
            if (RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE)){  /* is activate */

#ifdef RT_LITE_MEM_MODE
                if (_tasksQueue[i].leftForNextOccur < deltaTime){
                    _tasksQueue[i].leftForNextOccur = 0;
                } else {
                    _tasksQueue[i].leftForNextOccur -= deltaTime;
                }
                nextInterrupt = MIN(nextInterrupt, _tasksQueue[i].leftForNextOccur);
#else
                nextInterrupt = MIN(nextInterrupt, _tasksQueue[i].nextOccurrence);
#endif

            }
        }

#ifdef RT_LITE_MEM_MODE
        _lastUpdate+=deltaTime;                 // the time we start that function
        RT_RELEASE_TABLE();                     // release the table
        deltaTime = RT_GET_TIME() - _lastUpdate;     // how much time we lost during that function
#endif

#endif

#ifdef RT_LITE_MEM_MODE
        if (nextInterrupt < deltaTime){
            // still have tasks to execute
            RTattachInterrupt(0);
        } else {
            if (nextInterrupt != RT_MAX_MILLIS){
                RTattachInterrupt(nextInterrupt - deltaTime);
            }
        }
#else
        if (nextInterrupt != RT_MAX_MICROS){
            //nextInterrupt += localMicros;
            //localMicros = RT_GET_TIME();
            //nextInterrupt -=localMicros;
            if (nextInterrupt < localMicros){
                RTattachInterrupt(0);
            } else {
                RTattachInterrupt(nextInterrupt - localMicros);
            }
        }

#endif
    lock = false;
}

void RTevents::RTexecuteTask(RTtask_t* theTask){
    RTfunc_t func = theTask->func;

    // halt next execution of this task
    RT_FLAG_DEL(theTask->flags, RT_FLAG_ACTIVE);

#if 0
    if (RT_CHECK_FLAG(theTask->flags, RT_FLAG_NESTED)) {
        sei(); // TODO - allow nested interupt
    }
#endif

    // execute the task 
    func();
#if 1
    // check if this task was deleted
    if (func != theTask->func ||
                RT_CHECK_FLAG(theTask->flags, RT_FLAG_ACTIVE)){
        return;
    }
#endif

    if (RT_CHECK_FLAG(theTask->flags, RT_FLAG_ONE_TIME)){
        // free this task (not needed any more
        memset(theTask, 0x0, sizeof(RTtask_t));
    } else {
        // re-schedule this task for the next occurrence
#ifdef RT_LITE_MEM_MODE
        RT_LOCK_TABLE();
        // calculate how much time after the _lastUpdate will be the next occurrence
        theTask->leftForNextOccur = theTask->leftForNextOccur + theTask->period;
        RT_RELEASE_TABLE();
#else
        theTask->nextOccurrence = RT_CALC_NEXT_TIME(theTask->nextOccurrence, theTask->period);
#endif
        RT_FLAG_ADD(theTask->flags, RT_FLAG_ACTIVE);
    }
}







