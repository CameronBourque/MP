/*
     File        : blocking_disk.c

     Author      : Cameron Bourque
     Modified    : 11/14/2020

     Description : Blocking disk class implementation

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
  beg_queue = NULL;
  end_queue = beg_queue;

  disk_id = _disk_id;
  size = _size;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {
  Machine::outportb(0x1F1, 0x00); //send NULL to port 0x1F1
  Machine::outportb(0x1F2, 0x01); //send sector count to port 0x1F2
  Machine::outportb(0x1F3, (unsigned char)_block_no); //send low 8 bits of block number
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8)); //send next 8 bits of block number
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16)); //send next 8 bits of block number

  //send drive indicator, some bits, and highest 4 bits of block no
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24) & 0x0F) | 0xE0 | (disk_id << 4));

  Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

}

void BlockingDisk::wait_until_ready() {
  //check if ready for access
  if(!is_ready()) {
    //get current thread
    Thread * curr = Thread::CurrentThread();

    //enqueue the thread on blocking queue
    if(beg_queue == NULL) {
      beg_queue = curr;
    }
    else {
      end_queue->next = curr;  
    }
    end_queue = curr;
    curr->next = NULL;

    //block the running thread to let something else run
    SYSTEM_SCHEDULER->block();
  }
}

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  //check if other threads are waiting on disk
  if(beg_queue != NULL) {
    //add thread to blocked queue
    Thread * curr = Thread::CurrentThread();
    end_queue->next = curr;
    curr->next = NULL;
    
    //block thread
    SYSTEM_SCHEDULER->block();
  }

  //issue read operation
  issue_operation(READ, _block_no);

  //wait for disk to be ready
  wait_until_ready();

  //read data from port
  int i;
  unsigned short tmpw;
  for(i = 0; i < 256; i++) {
    tmpw = Machine::inportw(0x1F0);
    _buf[i*2] = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  //check if other threads are waiting on disk
  if(beg_queue != NULL) {
    //add thread to blocked queue
    Thread * curr = Thread::CurrentThread();
    end_queue->next = curr;
    curr->next = NULL;
    
    //block thread
    SYSTEM_SCHEDULER->block();
  }

  //issue write operation
  issue_operation(WRITE, _block_no);

  //wait for disk to be ready
  wait_until_ready();

  //write data to port
  int i;
  unsigned short tmpw;
  for(i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }
}
