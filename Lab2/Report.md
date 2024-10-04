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
If a thread calls ```timer_sleep()```, then it will get blocked immediately and later be unblocked when the specific amount of time passed to ```timer_sleep()``` has passed. This is implemented by setting ```sleep_to_ticks```, the time passed after the OS booted and time to sleep combined, when the thread is blocked. At each tick of the system, we will iterate over all threads using ```thread_foreach()```,, which is done with interrupts turned off. It checks if the time passed (which is checked using ```timer_ticks()```) is equal to or greater than ```sleep_to_ticks``` for a specific thread. The thread will be unblocked if that is the case. To prevent us from trying to unblock an already active or ready thread, the thread status has to be ```THREAD_BLOCKED```.

The algorithm for unblocking threads looks like this (note that ```aux``` is not used but is required for the interface of ```thread_foreach()```):
```c
/* Check if a blocked thread is ready to be unblocked */
void
check_threads(struct thread* th, void *aux)
{
  if(th->status == THREAD_BLOCKED && th->sleep_to_ticks <= timer_ticks())
  {
    ASSERT(th->status == THREAD_BLOCKED);
    thread_unblock(th);
  }
}
```

## Synchronization
As mentioned for the algorithm, interrupts are temporarily disabled when calling ```thread_foreach``` during a ```timer_interrupt```. This is to avoid a race condition where the function called ```check_threads``` tests threads at the same time from different calls. 
One important flaw to mention when using this method is the limit of threads that can run without creating a problem with consistent system tick. If going over threads takes too long, the call for ```timer_interrupt()``` during a tick might be missed.

## Complexity
Since the ```timer_interrupt()``` calls thread_foreach, the thread that calls for the interrupt traverses over all threads when checking for an opportunity to unblock. Because of this, the time complexity should be linear: ```O(n)``` in relation to the number of threads. The space complexity has not changed with our implementation. Only a constant amount of data was added to each thread.
