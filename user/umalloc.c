#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
freelist(void) { // Show the chain of free blocks.
  int i = 0;
  Header* p = &base;

  printf("Free list:\n");
  if(!freep) { printf("--\n"); return; } // Free list hasn't been created.
  for(p = p->s.ptr; p != &base; p = p->s.ptr) {
    printf("[%d] p = %p, p->s.size = %d bytes, p->s.ptr = %p\n",
           ++i, p, sizeof(Header) * p->s.size, p->s.ptr);
  } printf("\n");
}

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint nu)
{
  char *p;
  Header *hp;

  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  Header *best;     // Pointer to the closest-size free block
  uint diff = 0;    // Difference between the closest-size free block size and nunits

  // nunits is the requested memory allocation size in unit of Header plus 1.
  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;

  // Check if a free list exists.
  // If not, create one with a dummy node, base,
  // with zero size and the next pointer pointing to itself.
  if(!freep) {
    base.s.ptr = freep = &base;
    base.s.size = 0;
  }

  // Search the entire free list to find the closest-size free block.
  // Start searching the list from base.
  for(prevp = &base, p = base.s.ptr; ; prevp = p, p = p->s.ptr) {
    if(p == &base) {
      // If malloc() fails to find a proper free block
      // (i.e. if the value of diff has not changed from the initial value)
      // until it loops back to base,
      // it calls morecore(nunits) to add a new free block to the linked list.
      if(diff == 0) {
        if((p = morecore(nunits)) == 0)
          return 0;
      }
      // Otherwise, end the search.
      else
        break;
    }

    // If the free block has the exact size,
    // take it out from the linked list
    // by making the next of the previous free block point to the next block.
    // Then, end the search and return the starting address of the data space.
    if(p->s.size == nunits) {
      prevp->s.ptr = p->s.ptr;
      return (void*)(p + 1);
    }
    // Otherwise, keep search for the closest-size free block.
    else if(p->s.size > nunits) {
      // If the difference between free block size and nunits
      // is less than the value of diff.
      if(diff == 0 || (p->s.size - nunits) < diff) {
        // Update the value of diff.
        diff = p->s.size - nunits;
        // Make best point to the current free block.
        best = p;
      }
    }
  }

  // Split the chosen free block to allocate a memory chunk
  // at the rear end of the block.
  // And return the starting address of the data space.
  best->s.size -= nunits;
  best += best->s.size;
  best->s.size = nunits;
  return (void*)(best + 1);
}
