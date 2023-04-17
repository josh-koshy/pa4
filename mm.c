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

/* Global variables */
static int NUM_FREE_LISTS = 16; // Change this value according to your desired free list size classes
static char **segregated_free_lists; // TODO You may use global variables.
// Global variables
// Array of pointers to segregated free lists


/* Helper functions
 * You may change or redefine these to your liking
 * */
static void insert_into_free_list(char *bp); // Change this value according to your desired free list size classes
static void remove_from_free_list(char *bp);
static char *find_fit(size_t asize);
static void *coalesce(char *bp);

#define HEADER(size, alloc) ((size) | (alloc))
#define GET(p)          (*(uint64_t *)(p))
#define PUT(p, val)     (*(uint64_t *)(p) = (val), \
                         *(uint64_t *)((char *)(p) + GET_SIZE(p) - HEADER_SIZE) = (val))
#define PUT_PREV(p, val) (*(uint64_t *)((char *)(p) + HEADER_SIZE) = (val))
#define GET_PREV(p) (*(uint64_t *)((char *)(p) + HEADER_SIZE))
#define PUT_NEXT(p, val) (*(uint64_t *)((char *)(p) + HEADER_SIZE + sizeof(uint64_t)) = (val))
#define GET_NEXT(p) (*(uint64_t *)((char *)(p) + HEADER_SIZE + sizeof(uint64_t)))


#define GET_SIZE(p)     (GET(p) & ~(ALIGNMENT-1))
#define GET_ALLOC(p)    (GET(p) & 0x1)
#define MAX(x, y)       ((x) > (y)? (x) : (y))
#define PTR2BLK(ptr)    ((char *) ptr - HEADER_SIZE);
#define NEXT_BLK(bp)    ((char *) bp + GET_SIZE(bp))
#define PREV_BLK(bp)    ((char *) bp - GET_SIZE(((char *) bp - HEADER_SIZE)))

/*
 * mm_init - initialize the malloc package.
 */
static int find_list_index(size_t size) {
    // This is a simple example, you can use more efficient algorithms
    int index = 0;
    size_t current_size = ALIGNMENT;

    while (current_size < size && index < NUM_FREE_LISTS - 1) {
        current_size *= 2;
        index++;
    }

    return index;
}

static void insert_into_bst(char **root, char *bp) {
    if (*root == NULL) {
        *root = bp;
        PUT_PREV(bp, 0);
        PUT_NEXT(bp, 0);
        return;
    }

    size_t bp_size = GET_SIZE(bp);
    size_t root_size = GET_SIZE(*root);

    if (bp_size < root_size) {
        insert_into_bst((char **)(&(GET_PREV(*root))), bp);
    } else {
        insert_into_bst((char **)(&(GET_NEXT(*root))), bp);
    }
}






int mm_init(void)
{
    /*  Offsetting mem_sbrk's pointer by 8 from 16 alignment
    *   mem_sbrk returns 8-byte aligned address on class server
    *   If p is aligned to 16-bytes,
    *   Then allocate another 8-byte
    *   So that p is now 8-byte offset from 16-byte alignment.
    *   This allows space for the 8-byte header.
    */
    segregated_free_lists = (char **) malloc(NUM_FREE_LISTS * sizeof(char *));
    if (segregated_free_lists == NULL) {
        return -1;
    }
    int i;
    for (i = 0; i < NUM_FREE_LISTS; i++) {
        segregated_free_lists[i] = NULL;
    }

    void *p = mem_sbrk(0);
    if (((size_t)p % ALIGNMENT)==0){
        p = mem_sbrk(HEADER_SIZE);
    }

    /*  TODO: Initialize global variables, if any are used.
    *   (Hint: You may use global variables.)
    */

    /*  TODO: If you use dummy headers and footers,
    *   they should also be initialized.
    */
    return 0;
}

/*
 * mm_malloc - Allocate a block
 * This version calls sbrk to ask for more memory from the OS,
 * and will eventually run out of memory.
 *
 * Note:
 * You are encouraged to implement a lazy coalesce, that coalesces during malloc, not free.
 * If you wish to receive extra credit, mm_free and any of its subroutines must not coalesce.
 */
void *mm_malloc(size_t size)
{
    size_t asize = ALIGN(size + HEADER_SIZE);
    char *bp = find_fit(asize);

    if (bp != NULL) {
        remove_from_free_list(bp);
        PUT(bp, HEADER(asize, 1));
        return bp + HEADER_SIZE;
    }

    // If no suitable block found, resort to expanding the heap
    size_t extend_size = MAX(asize, mem_pagesize());
    char *new_block = mem_sbrk(extend_size);
    if (new_block == (void *)-1) {
        return NULL; // sbrk failed
    }
    PUT(new_block, HEADER(extend_size, 1));
    mm_free(new_block + HEADER_SIZE); // Insert the new block into the free list
    return mm_malloc(size); // Retry the allocation
}


/*
 * mm_free - Freeing a block.
 * This version does nothing.
 *
 * Note:
 * You are encouraged to implement a lazy coalesce, that coalesces during malloc, not free.
 * If you wish to receive extra credit, mm_free and any of its subroutines must not coalesce.
 */


void mm_free(void *ptr)
{
    char *bp = (char *)ptr - HEADER_SIZE;
    size_t size = GET_SIZE(bp);

    PUT(bp, HEADER(size, 0)); // Mark the block as unallocated
    int list_index = find_list_index(GET_SIZE(bp));
    insert_into_bst(&(segregated_free_lists[list_index]), bp);
}




/*
 * mm_coalesce - coalesce blocks
 *
 * Note:
 * If you wish to receive extra credit, this must be called only during malloc and/or its subroutines.
 */
static void insert_into_free_list(char *bp) {
    int list_index = find_list_index(GET_SIZE(bp));
    insert_into_bst(segregated_free_lists[list_index], bp);
}

static char *find_fit_in_bst(char *root, size_t asize) {
    if (root == NULL) {
        return NULL;
    }

    size_t root_size = GET_SIZE(root);
    if (root_size >= asize) {
        char *left_child = (char *)GET_PREV(root);
        char *fit = find_fit_in_bst(left_child, asize);
        return fit ? fit : root;
    } else {
        char *right_child = (char *)GET_NEXT(root);
        return find_fit_in_bst(right_child, asize);
    }
}


static void remove_from_free_list(char *bp) {
    int list_index = find_list_index(GET_SIZE(bp));

    if (segregated_free_lists[list_index] == bp) {
        segregated_free_lists[list_index] = (char *)GET(bp + sizeof(char *));
    } else {
        char *prev = segregated_free_lists[list_index];
        while (prev != NULL && (char *)GET(prev + sizeof(char *)) != bp) {
            prev = (char *)GET(prev + sizeof(char *));
        }
        if (prev != NULL) {
            PUT_NEXT(prev, GET(bp + sizeof(char *)));
        }
    }
}


static char *find_fit(size_t asize) {
    int list_index = find_list_index(asize);
    int j;
    for (j = list_index; j < NUM_FREE_LISTS; j++) {
        char *bp = find_fit_in_bst(segregated_free_lists[j], asize);
        if (bp) {
            return bp;
        }
    }
    return NULL;
}

static void *coalesce(char *bp) {
    char *prev_blk = PREV_BLK(bp);
    char *next_blk = NEXT_BLK(bp);
    size_t prev_alloc = (prev_blk >= (char *)mem_heap_lo()) ? GET_ALLOC(prev_blk) : 1;
    size_t next_alloc = (next_blk < (char *)mem_heap_hi()) ? GET_ALLOC(next_blk) : 1;

    if (prev_alloc && next_alloc) {
        return bp;
    }

    if (!prev_alloc && !next_alloc) {
        remove_from_free_list(prev_blk);
        remove_from_free_list(next_blk);
    } else if (!prev_alloc) {
        remove_from_free_list(prev_blk);
    } else if (!next_alloc) {
        remove_from_free_list(next_blk);
    }

    if (!prev_alloc) {
        size_t new_size = GET_SIZE(bp) + GET_SIZE(prev_blk);
        PUT(prev_blk, HEADER(new_size, 0));
        bp = prev_blk;
    }

    if (!next_alloc) {
        size_t new_size = GET_SIZE(bp) + GET_SIZE(next_blk);
        PUT(bp, HEADER(new_size, 0));
    }

    int list_index = find_list_index(GET_SIZE(bp));
    insert_into_bst(&(segregated_free_lists[list_index]), bp);

    return bp;
}
