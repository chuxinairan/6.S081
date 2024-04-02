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

struct bucket
{
  struct spinlock lock;
  struct buf head;
};

struct
{
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct bucket hashing[HASHNUM];
} bcache;

void binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < HASHNUM; i++)
  {
    bcache.hashing[i].head.next = &bcache.hashing[i].head;
    bcache.hashing[i].head.prev = &bcache.hashing[i].head;
    initlock(&bcache.hashing[i].lock, "bcache.bucket");
  }

  // Create linked list of buffers
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    bcache.hashing[0].head.next->prev = b;
    b->next = bcache.hashing[0].head.next;
    b->prev = &bcache.hashing[0].head;
    bcache.hashing[0].head.next = b;
    b->refcnt = 0;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno % HASHNUM;
  acquire(&bcache.hashing[index].lock);

  // Is the block already cached?
  for (b = bcache.hashing[index].head.next; b!=&bcache.hashing[index].head; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      b->lastused = ticks;
      release(&bcache.hashing[index].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  int i = 0;
  int lru_bucket=0;
  struct buf* evict_buf=0;
  int finded = 0;
  uint minticks = 0xffffffff;
  acquire(&bcache.lock);
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.

  //find in the belonging bucket
  for(struct buf *j=bcache.hashing[index].head.next; j!=&bcache.hashing[index].head; j=j->next){
    if(j->refcnt == 0 && j->lastused < minticks){
      lru_bucket = index;
      evict_buf = j;
      minticks = j->lastused;
      finded = 1;
    }
  }

  //
  for(; i<HASHNUM; i++){
    if (finded)
      break;
    if(i == index)
      continue;
    acquire(&bcache.hashing[i].lock);
    for(struct buf *j=bcache.hashing[i].head.next; j!=&bcache.hashing[i].head; j=j->next){
      if(j->refcnt == 0 && j->lastused < minticks){
        lru_bucket = i;
        evict_buf = j;
        minticks = j->lastused;
        finded = 1;
      }
    }
    if(finded){
      break;
    }
    release(&bcache.hashing[i].lock);
  }

  if(!finded)
    panic("bget: no buffers");

  for(i=i+1; i<HASHNUM; i++){
    if(i == index)
      continue;
    acquire(&bcache.hashing[i].lock);
    for(struct buf *j=bcache.hashing[i].head.next; j!=&bcache.hashing[i].head; j=j->next){
      if(j->refcnt == 0 && j->lastused < minticks){
        if(lru_bucket != i){
          release(&bcache.hashing[lru_bucket].lock);
        }
        lru_bucket = i;
        evict_buf = j;
        minticks = j->lastused;
      }
    }
    if(i != lru_bucket){
      release(&bcache.hashing[i].lock);
    } 
  }
  
  b = evict_buf;
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->lastused = ticks;
  b->prev->next = b->next;
  b->next->prev = b->prev;
  
  bcache.hashing[index].head.next->prev = b;
  b->next = bcache.hashing[index].head.next;
  b->prev = &bcache.hashing[index].head;
  bcache.hashing[index].head.next = b;
  
  release(&bcache.hashing[index].lock);
  if(lru_bucket != index)
    release(&bcache.hashing[lru_bucket].lock);
  release(&bcache.lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = b->blockno % HASHNUM;
  acquire(&bcache.hashing[index].lock);
  b->refcnt--;
  release(&bcache.hashing[index].lock);
}

void bpin(struct buf *b)
{
  int index = b->blockno % HASHNUM;
  acquire(&bcache.hashing[index].lock);
  b->refcnt++;
  release(&bcache.hashing[index].lock);
}

void bunpin(struct buf *b)
{
  int index = b->blockno % HASHNUM;
  acquire(&bcache.hashing[index].lock);
  b->refcnt--;
  release(&bcache.hashing[index].lock);
}
