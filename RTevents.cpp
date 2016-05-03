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
#define RT_FLAG_NESTED     16     // this task can be called nested - TODO - in development, don't use!

/* macros */
#define RT_FLAG_ADD(var, flag) var = (var | (flag))
#define RT_FLAG_DEL(var, flag) var = (var & ~flag)
#define RT_CHECK_FLAG(var, flag) (var & flag)

#define RT_CALC_NEXT_TIME(from, delay) ((long)((long)from + (long)delay))   // TODO - add check for overflow

#ifndef MIN
#define MIN(a, b) ((a<b) ? a : b)
#endif

/* static variables */
RTtask RTevents::_tasksQueue[RT_QUEUE_SIZE];
unsigned long RTevents::_curentNextIntrerupt;
bool RTevents::_interuptIsActivale;
    

void RTevents::begin(){
    _curentNextIntrerupt = RT_MAX_MILLIS;
    _interuptIsActivale = false;
    memset(&_tasksQueue, 0x0, sizeof(_tasksQueue));
    // TODO - change to direct use of the timer (define the timer and attach the interrupt handler here)

}

uint8_t RTevents::addTask(RTfunc func, long delay, uint16_t period){
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

            _tasksQueue[i].nextOccurrence = RT_CALC_NEXT_TIME(millis(), delay);

            RT_FLAG_ADD(_tasksQueue[i].flags, RT_FLAG_ACTIVE);
            // now this task can be scheduled

            if (0 == _curentNextIntrerupt || _tasksQueue[i].nextOccurrence < _curentNextIntrerupt){
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
    memset(&_tasksQueue[taskID], 0x0, sizeof(RTtask));
    return true;
}

void RTevents::RTattachInterrupt(unsigned long delay){

    _interuptIsActivale = true;
    _curentNextIntrerupt = millis() + delay;

    MsTimer2::set(delay, RTevents::RTinteruptHandler);
    MsTimer2::start();

}

void RTevents::RTdeAttachInterrupt(){

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
    RTdeAttachInterrupt();

    for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
        if (_tasksQueue[i].nextOccurrence <= millis() &&                     /* need to be executed now */
                    RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE) &&        /* is an active task */
                    (!RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_OVERFLOW)) ){  /* not need to wait for overflow */
            RTexecuteTask(&_tasksQueue[i]);
        }
    }

    // schedule the next interrupt
    localMillis = millis();
    for (int i=0 ; i<RT_QUEUE_SIZE ; i++){
        if (RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_ACTIVE) ||
                    RT_CHECK_FLAG(_tasksQueue[i].flags, RT_FLAG_USED) ){  /* not need to wait for overflow */
            nextInterrupt = MIN(nextInterrupt, _tasksQueue[i].nextOccurrence);
        }
    }

    if (nextInterrupt < localMillis){
        RTattachInterrupt(0);
    } else {
        if (nextInterrupt != RT_MAX_MILLIS){
            RTattachInterrupt(nextInterrupt - localMillis);
        }
    }
}

void RTevents::RTexecuteTask(RTtask* theTask){
    RTfunc func = theTask->func;

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
        memset(theTask, 0x0, sizeof(RTtask));
    } else {
        // re-schedule this task for the next occurrence
        theTask->nextOccurrence = RT_CALC_NEXT_TIME(theTask->nextOccurrence, theTask->period);
        RT_FLAG_ADD(theTask->flags, RT_FLAG_ACTIVE);
    }
}







