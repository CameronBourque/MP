/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    //make sure we don't have null values
    assert(_page_table != NULL && _frame_pool != NULL);

    //make sure base address is above 4MB
    assert(_base_address > (4 << 20));

    //register this vm_pool
    _page_table->register_pool(this);

    next = NULL;
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;

    //set up region list
    region_list = (Region*) base_address;    
    region_list[0].address = base_address;
    region_list[0].size = 1;

    for(int i = 1; i < REGION_SIZE; i++)
    {
      region_list[i].address = 0;
      region_list[i].size = 0;
    }

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    //add address to region
    int index = 0;
    unsigned long addr = 0;

    while(index < REGION_SIZE)
    {
      //find empty region
      while(region_list[index].size != 0)
      {
        index++;
      }

      //check if frame can fit
      unsigned long temp_addr = region_list[index-1].address;
      temp_addr += (region_list[index-1].size * PageTable::PAGE_SIZE);

      //is this the last available address?
      if(index + 1 == REGION_SIZE)
      {
        //can it fit?
        if(temp_addr + _size < base_address + size)
        {
          addr = temp_addr;
          break;
        }
      }
      else
      {
        //find next allocated address
        int temp_index = index+1;
        while(temp_index < REGION_SIZE)
        {
          if(region_list[temp_index].address != 0)
          {
            temp_addr = region_list[temp_index].address;
            break;
          }
          temp_index++;
        }

        //can it fit?
        if(temp_index >= REGION_SIZE)
        {
          addr = temp_addr;
          break;
        }
        else if(temp_addr + _size < region_list[temp_index].address)
        {
          addr = temp_addr;
          break;
        }
      }
    }

    //set region index
    if(addr)
    {
      region_list[index].address = addr;
      region_list[index].size = _size;
    }

    Console::puts("Allocated region of memory.\n");
    return addr;
}

void VMPool::release(unsigned long _start_address) {
    //make sure address is in range
    assert(_start_address >= base_address && _start_address < (base_address + size));

    //find region index
    int i = 0;
    while(region_list[i].address != _start_address)
    {
      i++;
    }

    //fill region list at index
    region_list[i].address = 0;
    region_list[i].size = 0;
    
    //free page table entry
    page_table->free_page(_start_address);

    Console::puts("Released region of memory.\n");
}

//should be above first 4MB!!!
bool VMPool::is_legitimate(unsigned long _address) {
    bool retval = false;
    //if address is the address of the regions list then return true
    if(_address == base_address)
    {
      retval = true;
    }
    //address must not be in first 4MB and is within base_address and base_address + size
    else if(_address < (4 << 20) || _address < base_address || _address >= base_address + size)
    {
      Console::puts("Checked whether address is part of an allocated region.\n");
      return false;
    }
    else
    {
      //check allocated addresses
      for(unsigned long i = 0; i < REGION_SIZE; i++)
      {
        if(_address >= region_list[i].address &&
           _address < (region_list[i].address + region_list[i].size * PageTable::PAGE_SIZE))
        {
          retval = true;
        }
      }
    }

    Console::puts("Checked whether address is part of an allocated region.\n");
    return retval;
}

