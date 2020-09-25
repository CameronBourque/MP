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
//  assert(kernel_mem_pool != NULL && process_mem_pool != NULL);

  //Initialize variables
  shared_size = _shared_size;
  kernel_mem_pool = _kernel_mem_pool;
  process_mem_pool = _process_mem_pool;

  Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
  //Allocate the space for the page table
  page_directory = (unsigned long*) kernel_mem_pool->get_frames(ContFramePool::needed_info_frames(PAGE_SIZE * ENTRIES_PER_PAGE));

  //Set the bits of the page entries
  page_directory[0] |= 0x3; //set to supervisor lvl, r/w, present
  for(int i = 1; i < 1024; i++)
  {
    page_directory[i] |= 0x2; //set to supervisor lvl, r/w, not present
  }

  Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
  current_page_table = this;
  Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
  write_cr3(current_page_table->page_directory[0]);
  write_cr0(read_cr0() | 0x80000000);
  paging_enabled  = 1;
  Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  assert(false);
  Console::puts("handled page fault\n");
}

