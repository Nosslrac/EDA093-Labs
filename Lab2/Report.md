# Lab2 report
Introduce lab2

## Data structures
Making a small modification to the thread structure to keep track of when a thread should wake up from a blocked state:
```c
struct thread {
    ...
    int sleep_to_ticks;
    ...
}
```
## Algorithms
If a thread calls ```timer_sleep()``` then it will get blocked imediately and then unblocked when the amount of time passed to ```timer_sleep()``` has passed. This is implemented by setting ```sleep_to_ticks``` when the thread is blocked. At each tick of the system we will iterate over all threads and check if the returned value by ```timer_ticks()``` is greater or equal to ```sleep_to_ticks``` for that thread. If that is the case the thread is unblocked. To prevent us from trying to unblock a already active/ready thread the thread status has to be ```THREAD_BLOCKED```.

## Syncronization
To prevent race conditions interrupts are disabled during the time when threads are being blocked and during the checks if a thread should be awoken.