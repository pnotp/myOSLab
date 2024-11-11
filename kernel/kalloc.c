// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++)
  {
    char buf[10];
    snprintf(buf, 10, "kmem-%d", i);
    initlock(&kmems[i].lock, buf);
    kmems[i].freelist = 0;
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  push_off(); // close interrupt
  int cid = cpuid();
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[cid].lock);
  r->next = kmems[cid].freelist;
  kmems[cid].freelist = r;
  release(&kmems[cid].lock);

  pop_off(); // open interrupt
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  
  int cid;
  push_off();
  cid = cpuid();
  acquire(&kmems[cid].lock);
  r = kmems[cid].freelist;
  if(r)
    kmems[cid].freelist = r->next;
  release(&kmems[cid].lock);

  if(!r){
    for(int i = 1; i < NCPU; i++){
      int next_cid = (cid + i) % NCPU;

      acquire(&kmems[next_cid].lock);
      if(kmems[next_cid].freelist){
        r = kmems[next_cid].freelist;
        kmems[next_cid].freelist = r->next;
        release(&kmems[next_cid].lock);
        break;
      }
      else{
        release(&kmems[next_cid].lock);
      }
    }
  }

  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
