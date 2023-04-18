/*
 * mm.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h> // added by me
#include "mm.h"
#include "memlib.h"
#include "align.h"

/* Alignment definitions
 * In align.h, there are three definitions relating to alignment.
 * You should NOT change those.
 *
 * Work on mm.c with the following assumption.
 * HEADER_SIZE is 8
 * ALIGNMENT is 16
 * ALIGN(size) rounds up size to the nearest ALIGNMENT
 * */
extern int mm_init(void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);

/* Private global variables */
static char *mem_heap;		/* Points to first byte of heap */
static char *mem_brk;		/* Points to last byte of heap plus 1 */
static char *mem_max_addr;	/* Max legal heap addr plus 1*/
static char *heap_listp;  /* Points to the prologue block */

/*
* mem_init - Initialize the memory system model
*/
void mem_init(void)
{
    mem_heap = (char *)Malloc(MAX_HEAP);
    mem_brk = (char *)mem_heap;
    mem_max_addr = (char *)(mem_heap + MAX_HEAP);
}

/*
* mem_sbrk - Simple model of the sbrk function. Extends the heap
*	by incr bytes and returns the start address of the new area. In
*	this model, the heap cannot be shrunk.
*/
void *mem_sbrk(int incr)
{
    char *old_brk = mem_brk;

    if ( (incr < 0)|| ((mem_brk + incr) > mem_max_addr)) {
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)–1l;
    }
    mem_brk += incr;
    return (void *)old_brk;
}
/* Basic constants and macros */
#define WSIZE	4	 /* Word and header/footer size (bytes) */
#define DSIZE	8	/* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size)	|	(alloc))

/* Read and write a word at address p */
#define GET(p)	(* (unsigned int *)(p))
#define PUT(p, val)		(*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)	(GET(p) & ~0x7)
#define GET_ALL0C(p)	(GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)	((char *) (bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
static void *extend_heap(size_t words); // Add the function prototype

int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);				/* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));	/* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	/* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));	/* Epilogue header */
    heap_listp += (2*WSIZE); // Point heap_listp to the prologue block

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == –1)
    return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));		/* Free block header */
    PUT(FTRP(bp), PACK(size, 0));		/* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	/* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

void mm_free(void *bp)	{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACKCsize, 0));
    PUT(FTRPCbp), PACKCsize, 0));
    coalesce(bp);
}

static void *coalesce(void *bp)	{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {			/* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {		/* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT (FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {		/* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRPCbp), PACKCsize, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACKCsize, 0));
        bp = PREV_BLKP(bp);
    }

    else {						/* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACKCsize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACKCsize, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}
void *mm_malloc(size_t size)
{
    size_t asize;	/* Adjusted block size */
    size_t extendsize;	/* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}