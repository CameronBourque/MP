/*
 File: scheduler.C
 
 Author: Cameron Bourque
 Date  : 10/23/2020
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  ready_queue = NULL;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  Thread* iter = ready_queue;
  ready_queue = ready_queue->next;
  iter->next = NULL;
}

void Scheduler::resume(Thread * _thread) {
  if(ready_queue == NULL)
  {
    ready_queue = _thread;
  }
  else
  {
    Thread* iter = ready_queue;
    while(iter->next)
    {
      iter = iter->next;
    }
    iter->next = _thread;
  }
}

void Scheduler::add(Thread * _thread) {
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  //if thread is at front of queue
  if(ready_queue == _thread)
  {
    ready_queue = ready_queue->next;
    _thread->next = NULL;
  }
  //otherwise find the thread
  else
  {
    Thread* iter = ready_queue;
    while(iter->next != _thread)
    {
      iter = iter->next;
    }
    iter->next = _thread->next;
    _thread->next = NULL;
  }
}
