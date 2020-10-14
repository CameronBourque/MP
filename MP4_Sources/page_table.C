#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
  //Make sure pointers aren't null
  assert(_kernel_mem_pool != NULL && _process_mem_pool != NULL);

  //Initialize variables
  shared_size = _shared_size;
  kernel_mem_pool = _kernel_mem_pool;
  process_mem_pool = _process_mem_pool;

  Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
  //Allocate the page directory in the kernel frame pool
  unsigned long info_frame_number = (unsigned long) process_mem_pool->get_frames(1);
  page_directory = (unsigned long*)(info_frame_number * PAGE_SIZE);

  //Allocate the page table
  info_frame_number = (unsigned long) process_mem_pool->get_frames(1);
  unsigned long* page_table = (unsigned long*)(info_frame_number * PAGE_SIZE);

  //Fill the page table
  unsigned long address = 0;
  for(int i = 0; i < 1024; i++)
  {
    page_table[i] = address | 0x3; //set to supervisor lvl, r/w, present
    address += 4096;
  }

  //Fill the page directory
  page_directory[0] = ((unsigned long)page_table) | 0x3; //set to supervisor lvl, r/w, present
  for(int i = 1; i < 1023; i++)
  {
    page_directory[i] = 0x2; //set to supervisor lvl, r/w, not present
  }
  //Make the last entry of page directory point to itself
  page_directory[1023] = ((unsigned long)page_directory) | 0x3; //set to supervisor lvl, r/w, present

  Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
  //set the current page table to this one and write the page dir to CR3
  current_page_table = this;
  write_cr3((unsigned long)page_directory);
  Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
  //write to CR0 and set our bool value to true
  write_cr0(read_cr0() | 0x80000000);
  paging_enabled  = 1;
  Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  //only need 3 least significant bits for error determination
  unsigned int err = _r->err_code & 0x7;

  bool supervisor_mode = !(err & 4);
  bool read_only = !(err & 2);
  bool page_not_present = !(err & 1);

  //masks for CR2 and table entries
  unsigned long pte_mask = 0x3FF000;
  unsigned long val_mask = 0xFFF;

  //is the page not present?
  if(page_not_present)
  {
    //update page table
    unsigned long fault_addr = read_cr2();
    unsigned long* page_dir = (unsigned long*) (0xFFFFF000 | ((unsigned long)current_page_table->page_directory));
    unsigned long dir_index = (unsigned long) (fault_addr & 0xFFC00000) >> 22;
    unsigned long pde = page_dir[dir_index];
    unsigned long* page_table = (unsigned long*) (0xFFC00000 | (dir_index << 12)); //page table gets assigned here!!!
    unsigned long pt_index = (fault_addr & 0x3FF000) >> 12;

    Console::puts("dir index = ");Console::putui(dir_index);Console::puts("\n");
    Console::puts("pt index  = ");Console::putui(pt_index);Console::puts("\n");
    Console::puts("pte addr  =");Console::putui((unsigned long)&page_table[pt_index]);Console::puts("\n");
    Console::puts("pde  =");Console::putui(pde);Console::puts("\n");

    //do we need to allocate a page table?
    if(!pde & 1)
    {
      //request a page for page table
      unsigned long frame_no = process_mem_pool->get_frames(1);
      page_dir[dir_index] = (unsigned long)(frame_no << 12);
      page_dir[dir_index] |= 0x3;
      Console::puts("pde  =");Console::putui(pde);Console::puts("\n");
    }
    
    //get a new frame
    unsigned long new_frame_no = 0;
    if(!dir_index)
    {
      //directly mapped frame
      new_frame_no = kernel_mem_pool->get_frames(1);
    }
    else
    {
      new_frame_no = process_mem_pool->get_frames(1);
    }

    Console::puts("here\n");
    Console::puts("pte =");Console::putui(page_table[pt_index]);Console::puts("\n");
    //set page table entry to new frame
    page_table[pt_index] = (unsigned long)(new_frame_no << 12);
    Console::puts("pte =");Console::putui(page_table[pt_index]);Console::puts("\n");

    //do we set r/w or just read only? (also mark present)
    if(read_only)
    {
      page_table[pt_index] |= 1;
    }
    else
    {
      page_table[pt_index] |= 3;
    }

    //do we need to set user mode
    if(!supervisor_mode)
    {
      page_table[pt_index] |= 4;
    }
  }
  //or is it a protection fault?
  else
  {
    Console::puts("Protection Fault Occurred!!!");Console::putui(err);Console::puts("\n");
    assert(0);
  }

  Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
  //
}

void PageTable::free_page(unsigned long _page_no)
{
  //
}
