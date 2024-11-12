// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NUM_BUCKET 13
#define HASH(x) ((x) % NUM_BUCKET)

struct {
  struct spinlock locks[NUM_BUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[NUM_BUCKET];
  char lockname[NUM_BUCKET][20];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NUM_BUCKET; i++){
    // char buf[10];
    snprintf(bcache.lockname[i], 10, "bcache-%d", i);
    initlock(&bcache.locks[i], bcache.lockname[i]);

    // Create linked list of buffers
    bcache.heads[i].prev = bcache.heads + i;
    bcache.heads[i].next = bcache.heads + i;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int idx = HASH(b->blockno);
    b->next = bcache.heads[idx].next;
    b->prev = &bcache.heads[idx];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[idx].next->prev = b;
    bcache.heads[idx].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = HASH(blockno);
  acquire(&bcache.locks[idx]);

  // Is the block already cached?
  for(b = bcache.heads[idx].next; b != &bcache.heads[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.heads[idx].prev; b != &bcache.heads[idx]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // no empty block
  release(&bcache.locks[idx]);
  for(int j = HASH(idx + 1); j != idx; j = HASH(j+1)){
    acquire(&bcache.locks[j]);
    for(b = bcache.heads[j].prev; b != &bcache.heads[j]; b = b->prev){
      if(b->refcnt == 0) {
        // 替换
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        // 在j桶中删去b
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.locks[j]);

        // 加到自己的桶里
        acquire(&bcache.locks[idx]);
        b->next = bcache.heads[idx].next;
        b->prev = &bcache.heads[idx];
        bcache.heads[idx].next->prev = b;
        bcache.heads[idx].next = b;

        release(&bcache.locks[idx]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.locks[j]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int idx = HASH(b->blockno);

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.locks[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[idx].next;
    b->prev = &bcache.heads[idx];
    bcache.heads[idx].next->prev = b;
    bcache.heads[idx].next = b;
  }
  
  release(&bcache.locks[idx]);
}

void
bpin(struct buf *b) {
  int idx = HASH(b->blockno);
  acquire(&bcache.locks[idx]);
  b->refcnt++;
  release(&bcache.locks[idx]);
}

void
bunpin(struct buf *b) {
  int idx = HASH(b->blockno);
  acquire(&bcache.locks[idx]);
  b->refcnt--;
  release(&bcache.locks[idx]);
}


