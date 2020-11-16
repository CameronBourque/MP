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
#include "blocking_disk.H"

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

extern Scheduler * SYSTEM_SCHEDULER; //extern for kernel defined scheduler
extern BlockingDisk * SYSTEM_DISK; //extern for system disk

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
  //2098116

  //set up ready queue
  beg_queue = NULL;
  end_queue = beg_queue;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  //need to disable interrupts if they are on
  if(Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

  //check if disk is ready
  if(SYSTEM_DISK->is_ready() && SYSTEM_DISK->beg_queue != NULL)
  {
    //pop off blocked queue
    Thread * ready = SYSTEM_DISK->beg_queue;
    if(SYSTEM_DISK->end_queue == SYSTEM_DISK->beg_queue)
    {
      SYSTEM_DISK->end_queue = SYSTEM_DISK->end_queue->next;
    }
    SYSTEM_DISK->beg_queue = SYSTEM_DISK->beg_queue->next;

    //place thread on ready queue
    if(!beg_queue)
    {
      beg_queue = ready;
      end_queue = ready;
    }
    else
    {
      end_queue->next = ready;
      end_queue = end_queue->next;
    }
    ready->next = NULL;
  }

  //get current thread
  Thread* curr = Thread::CurrentThread();

  //if idle and nothing in queue then return
  if(curr == idle_thread && !beg_queue)
  {
    //need to enable interrupts before returning
    Machine::enable_interrupts();
    return;
  }

  //if the ready queue is empty
  if(!beg_queue)
  {
    //fill ready queue
    beg_queue = curr;
    end_queue = curr;
    curr->next = NULL;

    //enable interrupts and dispatch idle thread
    Thread::dispatch_to(idle_thread);
    Machine::enable_interrupts();
    return;
  }

  //move current thread to back of queue
  end_queue->next = curr;
  end_queue = curr;
  curr->next = NULL;

  //dispatch next thread
  curr = beg_queue;
  beg_queue = beg_queue->next;
  Thread::dispatch_to(curr);

  //reenable the interrupts()
  Machine::enable_interrupts();
}

void Scheduler::block() {
  //need to disable interrupts if they are on
  if(Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

  //if the ready queue is empty
  if(!beg_queue)
  {
    //enable interrupts and dispatch idle thread
    Thread::dispatch_to(idle_thread);
    Machine::enable_interrupts();
    return;
  }

  //dispatch next thread
  Thread * run = beg_queue;
  if(end_queue == beg_queue)
  {
    end_queue = end_queue->next;
  }
  beg_queue = beg_queue->next;
  Thread::dispatch_to(run);

  //reenable the interrupts()
  Machine::enable_interrupts();
}

void Scheduler::resume(Thread * _thread) {
  //need to disable interrupts if they are on
  if(Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

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

  //reenable interrupts
  Machine::enable_interrupts();
}

void Scheduler::add(Thread * _thread) {
  //need to disable interrupts if they are on
  if(Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

  //call resume
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  //need to disable interrupts if they are on
  if(Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

  //if thread is current thread then yield but don't replace at back
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

    //reenable interrupts
    Machine::enable_interrupts();
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

  //reenable interrupts
  Machine::enable_interrupts();
}
