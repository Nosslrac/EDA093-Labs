/*
 * Exercise on thread synchronization.
 *
 * Assume a half-duplex communication bus with limited capacity, measured in
 * tasks, and 2 priority levels:
 *
 * - tasks: A task signifies a unit of data communication over the bus
 *
 * - half-duplex: All tasks using the bus should have the same direction
 *
 * - limited capacity: There can be only 3 tasks using the bus at the same time.
 *                     In other words, the bus has only 3 slots.
 *
 *  - 2 priority levels: Priority tasks take precedence over non-priority tasks
 *
 *  Fill-in your code after the TODO comments
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/interrupt.h"
#include "timer.h"
#include <list.h>

/* This is where the API for the condition variables is defined */
#include "threads/synch.h"

/* This is the API for random number generation.
 * Random numbers are used to simulate a task's transfer duration
 */
#include "lib/random.h"

#define MAX_NUM_OF_TASKS 200

#define BUS_CAPACITY 3

typedef enum {
  SEND = 0,
  RECEIVE = 1,

  NUM_OF_DIRECTIONS
} direction_t;

typedef enum {
  NORMAL,
  PRIORITY,

  NUM_OF_PRIORITIES
} priority_t;

typedef struct task{
  direction_t direction;
  priority_t priority;
  struct list_elem task_elem;
  unsigned long transfer_duration;
} task_t;

typedef struct {
  direction_t bus_direction;
  uint32_t in_transmission;
  struct lock bus_lock;
  struct list waiting_tasks[2];
  struct condition transfer_condition[2];
} batch_scheduler_t;

batch_scheduler_t scheduler;

void init_bus (void);
void batch_scheduler (unsigned int num_priority_send,
                      unsigned int num_priority_receive,
                      unsigned int num_tasks_send,
                      unsigned int num_tasks_receive);

/* Thread function for running a task: Gets a slot, transfers data and finally
 * releases slot */
static void run_task (void *task_);

/* WARNING: This function may suspend the calling thread, depending on slot
 * availability */
static void get_slot (const task_t *task);

/* Simulates transfering of data */
static void transfer_data (const task_t *task);

/* Releases the slot */
static void release_slot (const task_t *task);

void init_bus (void) {

  random_init ((unsigned int)123456789);

  /* TODO: Initialize global/static variables,
     e.g. your condition variables, locks, counters etc */
  scheduler.bus_direction = SEND;
  scheduler.in_transmission = 0;
  list_init(&scheduler.waiting_tasks[SEND]);
  list_init(&scheduler.waiting_tasks[RECEIVE]);
  lock_init(&scheduler.bus_lock);
  cond_init(&scheduler.transfer_condition[SEND]);
  cond_init(&scheduler.transfer_condition[RECEIVE]);
}

void batch_scheduler (unsigned int num_priority_send,
                      unsigned int num_priority_receive,
                      unsigned int num_tasks_send,
                      unsigned int num_tasks_receive) {
  ASSERT (num_tasks_send + num_tasks_receive + num_priority_send +
             num_priority_receive <= MAX_NUM_OF_TASKS);

  static task_t tasks[MAX_NUM_OF_TASKS] = {0};

  char thread_name[32] = {0};

  unsigned long total_transfer_dur = 0;

  int j = 0;

  /* create priority sender threads */
  for (unsigned i = 0; i < num_priority_send; i++) {
    tasks[j].direction = SEND;
    tasks[j].priority = PRIORITY;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "sender-prio");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create priority receiver threads */
  for (unsigned i = 0; i < num_priority_receive; i++) {
    tasks[j].direction = RECEIVE;
    tasks[j].priority = PRIORITY;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "receiver-prio");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create normal sender threads */
  for (unsigned i = 0; i < num_tasks_send; i++) {
    tasks[j].direction = SEND;
    tasks[j].priority = NORMAL;
    tasks[j].transfer_duration = random_ulong () % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "sender");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create normal receiver threads */
  for (unsigned i = 0; i < num_tasks_receive; i++) {
    tasks[j].direction = RECEIVE;
    tasks[j].priority = NORMAL;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "receiver");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* Sleep until all tasks are complete */
  timer_sleep (2 * total_transfer_dur);
}

/* Thread function for the communication tasks */
void run_task(void *task_) {
  task_t *task = (task_t *)task_;

  get_slot (task);

  msg ("%s acquired slot", thread_name());
  transfer_data (task);

  release_slot (task);
}

static direction_t other_direction(direction_t this_direction) {
  return this_direction == SEND ? RECEIVE : SEND;
}

static bool has_prio_in_direction(direction_t dir){
  struct list_elem* e = list_begin(&scheduler.waiting_tasks[dir]);
  struct task* task_struct = list_entry(e, struct task, task_elem);
  return task_struct->priority == PRIORITY;
}

static bool prio_compare(const struct list_elem *a_, const struct list_elem *b_,
            void *aux UNUSED) 
{
  const struct task *a = list_entry (a_, struct task, task_elem);
  const struct task *b = list_entry (b_, struct task, task_elem);
  
  return a->priority < b->priority;
}



void get_slot (const task_t *task) {

  /* TODO: Try to get a slot, respect the following rules:
   *        1. There can be only BUS_CAPACITY tasks using the bus
   *        2. The bus is half-duplex: All tasks using the bus should be either
   * sending or receiving
   *        3. A normal task should not get the bus if there are priority tasks
   * waiting
   *
   * You do not need to guarantee fairness or freedom from starvation:
   * feel free to schedule priority tasks of the same direction,
   * even if there are priority tasks of the other direction waiting
   */
  enum intr_level old_level = intr_disable();
  struct list_elem* e = &task->task_elem;
  lock_acquire(&scheduler.bus_lock);
  while(scheduler.in_transmission > 0){
    if(scheduler.in_transmission < BUS_CAPACITY) {
      if(task->direction == scheduler.bus_direction && task->priority == PRIORITY){
          break;
      }
      if(!has_prio_in_direction(other_direction(task->direction))){
        break;
      }
    }
   
    list_push_back(&scheduler.waiting_tasks[task->direction], &task->task_elem);
    list_sort(&scheduler.transfer_condition[task->direction].waiters, prio_compare, NULL);
    cond_wait(&scheduler.transfer_condition[task->direction], &scheduler.bus_lock);
    list_remove(e);
  }
  scheduler.in_transmission++;
  scheduler.bus_direction = task->direction;

  lock_release(&scheduler.bus_lock);
  intr_set_level(old_level);
}

void transfer_data (const task_t *task) {
  /* Simulate bus send/receive */
  timer_sleep (task->transfer_duration);
}

void release_slot (const task_t *task) {

  /* TODO: Release the slot, think about the actions you need to perform:
   *       - Do you need to notify any waiting task?
   *       - Do you need to increment/decrement any counter?
   */
  lock_acquire(&scheduler.bus_lock);
  scheduler.in_transmission--;
  direction_t dir = task->direction;
  direction_t other_dir = other_direction(dir);
  bool switch_side = has_prio_in_direction(other_dir) && !has_prio_in_direction(dir);

  if(!list_empty(&scheduler.waiting_tasks[task->direction]) && !switch_side){
    cond_signal(&scheduler.transfer_condition[task->direction], &scheduler.bus_lock);
  }
  else if(scheduler.in_transmission == 0){
    cond_broadcast(&scheduler.transfer_condition[other_direction(task->direction)], &scheduler.bus_lock);
  }
  lock_release(&scheduler.bus_lock);
}
