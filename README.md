# RTevents
Arduino library for schedule multiple number of independent task after constant delay or period

## summary
this library make use of the hardware timer2 of the arduino device to schedule multiple RT tasks.

- can easily replace the use of the standard delay (no waste of CPU)
- an long (non RT) task during the loop do not affect the RT tasks 

## how to use
in general, for each task write **short** function (with no parameters) that make the logic of the task

to schedue the task use the static function *RTevents::addTask(func, delay, period)* 
for execute the task *func* every *period* milliseconds starting *delay* ms from now (*period = 0* mean execute only once). 
### limited number of tasks
in this library there is limited number of tasks that can be scheduled at the same time, that mean it can remember limited number of tasks.

to change that limit you can edit the define *RT_QUEUE_SIZE*, 
smaller number will reduce the RAM consumed by the library.


## based on MsTimer2 Library:
Originally written by Javier Valencia

MsTimer2 must be installed in the arduino IDE,
for more details: https://github.com/PaulStoffregen/MsTimer2


