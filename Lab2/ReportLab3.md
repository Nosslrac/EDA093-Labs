# Lab3 report
Report for lab3 regarding the implementation of an edited ```batch_scheduler.c```

## Implementation
This implementation of a batch scheduler limits the number of current tasks on the bus at the same time, given by the value ```BUS_CAPACITY```. To ensure that the number of current running tasks does not exceed this, the scheduler keeps track of the current number of tasks on the bus. When a task is waiting to enter, it will only have a chance of this if tasks in transmission < ```BUS_CAPACITY```. When a task enters, a variable ```in_transmission``` is incremented. When it leaves the bus, it is decremented.
The tasks in transmission should all only "go in the same direction," meaning be in a state of either ```SEND``` or ```RECEIVE```. To ensure this, a variable ```bus_direction``` keeps track of the current direction of the bus. This is first set when the first task enters the bus. The direction of the bus will only change when there are no tasks using it, and there are other tasks of the opposite direction waiting to enter. All tasks waiting are tracked and can be awakened using a condition variable. This must be done for the bus to change direction.
For the batch scheduler, different tasks can also be of different priorities. It can be set to either ```NORMAL``` or ```PRIORITY```. ...

## Scheduling
This implementation of a batch scheduler does not exert fair scheduling. Since it focuses more on priority than the current direction of the bus, ```NORMAL``` tasks can be kept in endless waiting. For example, say there are 3 ```PRIORITY SEND```, 3 ```PRIORITY RECEIVE```, and 1 ```NORMAL SEND```. The bus is first occupied by the ```PRIORITY SEND```, and when these tasks are done, it invites the ```PRIORITY RECEIVE``` to enter. If then there are new ```PRIORITY SEND``` or ```PRIORITY RECEIVE``` tasks generated and waiting to enter, they will have priority over the ```NORMAL SEND```.
Modified design...
