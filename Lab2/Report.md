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
If a thread calls ```timer_sleep()``` then it will get blocked imediately and then unblocked when the amount of time passed to ```timer_sleep()``` has passed. This is implemented by setting ```sleep_to_ticks``` when the thread is blocked. At each tick of the system we will iterate over all threads and check if the returned value by ```timer_ticks()``` is greater or equal to ```sleep_to_ticks``` for that thread. If that is the case the thread is unblocked. To prevent us from trying to unblock a already active/ready thread the thread status has to be ```THREAD_BLOCKED```. To ensure correct functionality of our implementation we added several ```ASSERT()```, to make sure that the behavior of the system is what we expect.

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

## Syncronization
To prevent race conditions interrupts are disabled during the time when threads are being blocked and during the checks if a thread should be awoken.
One important flaw to mention when using this method is the limit of threads that can run without creating a problem with consistent system tick. If going over threads takes too long, the call for ```timer_interrupt()``` during a tick might be missed.

## Complexity
Since the ```timer_interrupt()``` calls thread_foreach, the thread that calls for the interrupt traverses over all threads when checking for an opportunity to unblock. Because of this, the time complexity should be linear: ```O(n)``` in relation to the number of threads. The space complexity has not changed with our implementation. Only a constant amount of data was added to each thread.
