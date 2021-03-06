/*
     File        : blocking_disk.H

     Author      : Cameron Bourque

     Date        : 11/14/2020
     Description : Blocking disk class implementation

*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class BlockingDisk : public SimpleDisk {
private:
   //Gives access to blocking disk about disk id and size since they are private
   DISK_ID disk_id;
   unsigned int size;

   //Issue operation to read or write. Here since simple disk has this private
   void issue_operation(DISK_OPERATION _op, unsigned long _block_no);
   //New wait until ready implementation with no busy wait
   void wait_until_ready();

public:
   //Blocking queue beginning and end
   Thread* beg_queue;
   Thread* end_queue;

   BlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

   virtual bool is_ready() { return SimpleDisk::is_ready(); }
   /* Checks if the disk is ready to be operated on */

};

#endif
