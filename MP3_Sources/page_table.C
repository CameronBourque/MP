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
  unsigned long info_frame_number = (unsigned long) kernel_mem_pool->get_frames(ENTRIES_PER_PAGE * 4 / PAGE_SIZE);
  page_directory = (unsigned long*)(info_frame_number * PAGE_SIZE);

  Console::puts("info frame: ");Console::putui(info_frame_number);Console::puts(" page_dir addr: ");Console::putui((unsigned long)page_directory);Console::puts("\n");

  //Allocate the page table on the stack
  info_frame_number = (unsigned long) kernel_mem_pool->get_frames(ENTRIES_PER_PAGE * 4 / PAGE_SIZE);
  unsigned long* page_table = (unsigned long*)(info_frame_number * PAGE_SIZE);

  Console::puts("page_table addr: ");Console::putui((unsigned long)page_table);Console::puts("\n");

  //Fill the page table
  unsigned long address = 0;
  for(int i = 1; i < 1024; i++)
  {
    page_table[i] = address | 0x3; //set to supervisor lvl, r/w, present
    address += 4096;
  }

  //Fill the page directory
  page_directory[0] = ((unsigned long)page_table) | 0x3;
  Console::puts("page dir addr = ");Console::putui(page_directory[0]);Console::puts("\n");
  Console::puts("page table[1] addr = ");Console::putui((unsigned long)(&page_table[1]));Console::puts("\n");
  for(int i = 1; i < 1024; i++)
  {
    page_directory[i] = 0x2;
  }

  Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
  current_page_table = this;
  Console::puts("Current CR3: ");Console::putui((unsigned long)read_cr3);Console::puts("\n");
  Console::puts("Page directory addr: ");Console::putui((unsigned long)page_directory);Console::puts("\n");
  write_cr3((unsigned long)page_directory);
  Console::puts("CR3 after writing: ");Console::putui((unsigned long)read_cr3);Console::puts("\n");
  Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
  Console::puts("writing cr0\n");
  Console::putui((unsigned long)read_cr0);Console::puts("\n");
  write_cr0(read_cr0() | 0x80000000);
  Console::putui((unsigned long)read_cr0);Console::puts("\n");
  Console::puts("updating bool val\n");
  paging_enabled  = 1;
  Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  assert(false);
  Console::puts("handled page fault\n");
}

