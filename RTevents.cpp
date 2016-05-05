/*
 * RTevents.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: hodai
 */

#include "RTevents.h"

/* constants */
#define RT_MAX_MILLIS       0xFFFFFFFF  // ~50 days

/* flags */
#define RT_FLAG_ACTIVE     1      // without this flag the task is not scheduled
#define RT_FLAG_USED       2
#define RT_FLAG_ONE_TIME   4      // if set, this task will occur once and removed
#define RT_FLAG_OVERFLOW   8      // the next occurrence is after the counter is overflow ( > 2^32)
#define RT_FLAG_UNIT_US    16     // if set, the period / delay is in uSec - TODO - in development, don't use!
#define RT_FLAG_NESTED     32     // this task can be called nested - TODO - in development, don't use!

/* macros */
#define RT_FLAG_ADD(var, flag) var = (var | (flag))
#define RT_FLAG_DEL(var, flag) var = (var & ~flag)
#define RT_CHECK_FLAG(var, flag) (var & flag)

#define RT_CALC_NEXT_TIME(from, delay) ((unsigned long)((unsigned long)from + (unsigned long)delay))   // TODO - add check for overflow

#define RT_LOCK_TABLE()        cli();
#define RT_RELEASE_TABLE()     sei();

#ifndef MIN
#define MIN(a, b) ((a<b) ? a : b)
#endif

/* static variables */
RTtask_t RTevents::_tasksQueue[RT_QUEUE_SIZE];

unsigned long RTevents::_curentNextIntrerupt;

#ifdef RT_LITE_MEM_MODE
unsigned long RTevents::_lastUpdate;
#endif

bool RTevents::_interuptIsActivale;
    

void RTevents::begin(){
    _curentNextIntrerupt = RT_MAX_MILLIS;
    _interuptIsActivale = false;

#ifdef RT_LITE_MEM_MODE
    _lastUpdate = millis();
#endif

    memset(&_tasksQueue, 0x0, sizeof(_tasksQueue));
    // TODO - change to direct use of the timer (define the timer and attach the interrupt handler here)

}

#ifdef RT_LITE_MEM_MODE
uint8_t RTevents::addTask(RTfunc_t func, uint16_t delay, uint16_t period){
#else
uint8_t RTevents::addTask(RTfunc_t func, unsigned long delay, uint16_t period){
#endif
    // basic validation
    if (delay < 0 || delay > RT_MAX_DELAY ||
                period < 0 || period > RT_MAX_PERIOD){
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
            _tasksQueue[i].leftForNextOccur = delay + (millis()-_lastUpdate);
#else
            _tasksQueue[i].nextOccurrence = RT_CALC_NEXT_TIME(millis(), delay);
#endif

            RT_FLAG_ADD(_tasksQueue[i].flags, RT_FLAG_ACTIVE);
            // now this task can be scheduled

#ifdef RT_LITE_MEM_MODE
            if (0 == _curentNextIntrerupt || RT_CALC_NEXT_TIME(millis(), delay) < _curentNextIntrerupt){
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

    _interuptIsActivale = true;
    _curentNextIntrerupt = millis() + delay;

    MsTimer2::set(delay, RTevents::RTinteruptHandler);
    MsTimer2::start();

}

void RTevents::RTdetachInterrupt(){

    _interuptIsActivale = false;
    _curentNextIntrerupt = RT_MAX_MILLIS;

    MsTimer2::stop();

}

void RTevents::RTinteruptHandler(){
    unsigned long nextInterrupt = RT_MAX_MILLIS;
    unsigned long localMillis;
    // TODO - make 'i' static to solve starvation problems
    // TODO - wrap all this with 'while' loop instead using recursion to solve stack overflow

    // first stop the next interrupts
    RTdetachInterrupt();

#ifdef RT_LITE_MEM_MODE
    localMillis = millis();
    unsigned long deltaTime= localMillis - _lastUpdate;
    // TODO - if negative millis overflow occur
#endif

    for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
#ifdef RT_LITE_MEM_MODE
        if (_tasksQueue[i].leftForNextOccur <= deltaTime &&                     /* need to be executed now */
#else
        if (_tasksQueue[i].nextOccurrence <= millis() &&                     /* need to be executed now */
#endif
                    RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE) &&        /* is an active task */
                    (!RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_OVERFLOW)) ){  /* not need to wait for overflow */
            RTexecuteTask(&_tasksQueue[i]);
        }
    }

    // schedule the next interrupt
#ifdef RT_LITE_MEM_MODE
    RT_LOCK_TABLE();      // lock the tasks table
#else
    localMillis = millis();
#endif

    for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
        if (RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE) ||
                    RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_USED) ){  /* not need to wait for overflow */

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
    deltaTime = millis() - _lastUpdate;     // how much time we lost during that function
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
    if (nextInterrupt < localMillis){
        RTattachInterrupt(0);
    } else {
        if (nextInterrupt != RT_MAX_MILLIS){
            RTattachInterrupt(nextInterrupt - localMillis);
        }
    }
#endif

}

void RTevents::RTexecuteTask(RTtask_t* theTask){
    RTfunc_t func = theTask->func;

    // halt next execution of this task
    RT_FLAG_DEL(theTask->flags, RT_FLAG_ACTIVE);

    if (RT_CHECK_FLAG(theTask->flags, RT_FLAG_NESTED)) {
    	sei(); // TODO - allow nested interupt
    }
    
    // execute the task 
    func();

    // check if this task was deleted
    if (func != theTask->func ||
                RT_CHECK_FLAG(theTask->flags, RT_FLAG_ACTIVE)){
        return;
    }

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







