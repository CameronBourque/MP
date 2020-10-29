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

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

//This is for the idle thread
void idle()
{
  while(true)
  {
    SYSTEM_SCHEDULER->yield();
  }
}

Scheduler::Scheduler() {
  //set up idle thread
  idle_stack = new char[1024];
  idle_thread = new Thread(idle, idle_stack, 1024);
  Console::puts("idle thread esp");Console::putui((unsigned long)idle_thread);Console::puts("\n");

  //set up ready queue
  beg_queue = NULL;
  end_queue = beg_queue;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  //get current thread
  Thread* curr = Thread::CurrentThread();

  //if idle and nothing in queue then return
  if(curr == idle_thread && !beg_queue)
  {
    return;
  }

  //take front of queue and move to back
  end_queue->next = curr;
  end_queue = curr;
  curr->next = NULL;

  Console::puts("yielded\n");
  Console::puts("dispatching to: ");Console::putui((unsigned long)beg_queue);Console::puts("\n");
  //dispatch next thread
  curr = beg_queue;
  beg_queue = beg_queue->next;
  Thread::dispatch_to(curr);
}

void Scheduler::resume(Thread * _thread) {
  //check if queue is empty
  if(!beg_queue)
  {
    beg_queue = _thread;
    end_queue = _thread;
  }
  else
  {
    //place thread at back of queue
    end_queue->next = _thread;
    end_queue = end_queue->next;
  }

  //make sure this thread's next is null
  _thread->next = NULL;

  Console::puts("resumed\n");
}

void Scheduler::add(Thread * _thread) {
  //call resume
  resume(_thread);
  Console::puts("added\nQueue list:\n");

  Thread * iter = beg_queue;
  while(iter)
  {
    Console::putui((unsigned long)iter);Console::puts("\n");
    iter = iter->next;
  }
}

void Scheduler::terminate(Thread * _thread) {
  //if thread is current thread then dispatch and don't replace at back
  if(_thread == Thread::CurrentThread())
  {
    _thread->next == NULL;
    Thread* ready = beg_queue;
    beg_queue = beg_queue->next;
    if(end_queue == ready)
    {
      end_queue = beg_queue;
    }

    Thread::dispatch_to(ready);
    Console::puts("terminated\n");
    return;
  }

  //if thread is at front of queue then remove it
  if(beg_queue == _thread)
  {
    if(end_queue == beg_queue)
    {
      end_queue = beg_queue->next;
    }

    beg_queue = beg_queue->next;
  }
  //otherwise find the thread
  else
  {
    Thread* iter = beg_queue;
    while(iter->next != _thread)
    {
      iter = iter->next;
    }

    //remove thread from queue
    iter->next = _thread->next;
  }

  //make sure it has no ties to the queue
  _thread->next = NULL;
  Console::puts("terminated\n");
}
