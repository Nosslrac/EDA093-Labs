# Lab3 report
Report for lab3 regarding the implementation of an edited ```batch_scheduler.c```

## Implementation
This implementation of a batch scheduler limits the number of current tasks on the bus at the same time, given by the value ```BUS_CAPACITY```. To ensure that the number of current running tasks does not exceed this, the scheduler keeps track of the current number of tasks on the bus. When a task is waiting to enter, it will only have a chance of this if tasks in transmission < ```BUS_CAPACITY```. When a task enters, a variable ```in_transmission``` is incremented. When it leaves the bus, it is decremented.

The tasks in transmission should all only "go in the same direction," meaning that they are in a state of either ```SEND``` or ```RECEIVE```. To ensure this, a variable ```bus_direction``` keeps track of the current direction of the bus. This is first set when the first task enters the bus. The direction of the bus will only change when there are no tasks using it, and there are other tasks of the opposite direction waiting to enter. All tasks waiting are tracked and can be awakened using a condition variable. When a slot on the bus is released, it first checks if there are any tasks waiting "in the same direction" as the ones already on the bus. If not, try to awake any other tasks.

For the batch scheduler, different tasks can also be of different priorities. It can be set to either ```NORMAL``` or ```PRIORITY```. Priority of the tasks waiting take precedence over the direction of the tasks waiting, i.e. if the direction of the bus is currently send, tasks set with ```PRIORITY``` but receive will have priority over ```NORMAL```` with send. ...

To prioritize ```PRIORITY``` tasks, every time a new task arrives and it cannot send immediately, we will sort the list of waiting tasks such that ```PRIORITY``` tasks are first in line. ```NORMAL``` tasks will only enter the bus if there is no ```PRIORITY``` task waiting in the same or other direction. This check is done by ```has_prio_in_direction(direction_t dir)```, which only checks the first waiting task of the other direction since the waiting queue is sorted based on priority.

## Scheduling
This implementation of a batch scheduler does not exert fair scheduling. Since it focuses more on priority than the current direction of the bus, ```NORMAL``` tasks can be kept in endless waiting. For example, say there are 3 ```PRIORITY SEND```, 3 ```PRIORITY RECEIVE```, and 1 ```NORMAL SEND```. The bus is first occupied by the ```PRIORITY SEND```, and when these tasks are done, it invites the ```PRIORITY RECEIVE``` to enter. If then there are new ```PRIORITY SEND``` or ```PRIORITY RECEIVE``` tasks generated and waiting to enter, they will have priority over the ```NORMAL SEND```. Additionally, it is also not fair between the directions either.

Three things must be true for the bus to change direction:
- There are no priority tasks waiting in the current bus direction
- There are priority tasks waiting in the other direction
- No tasks are currently in transmission

Thus, if there is an endless supply of ```PRIORITY SEND``` tasks and and the bus is set to ```SEND``` the bus will **NEVER** change direction.


With the current design it would be quite easy to modify our implementation to make it more fair in this aspect. We could keep track of the number of ```PRIORITY``` tasks that have been sent in one direction. When it surpasses some threshold and there are ```PRIORITY``` tasks in the other direction we could disallow more tasks from entering the bus, wait until the bus is empty and then broadcast to the other direction which will also change the direction of the bus. Under the assumption that ```PRIORITY``` tasks should take precedence, it wouldn't make much sense to solve the issue of fairness between ```PRIORITY``` and ```NORMAL``` tasks.