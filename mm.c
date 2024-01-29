/*
 * CSE 351 Lab 5 (Dynamic Storage Allocator)
 *
 * Name(s): Joban Mand, Smayan Nirantare 
 * NetID(s): jmand1, smayan
 *
 * NOTES:
 *  - Explicit allocator with an explicit free-list
 *  - Free-list uses a single, doubly-linked list with LIFO insertion policy,
 *    first-fit search strategy, and immediate coalescing.
 *  - We use "next" and "previous" to refer to blocks as ordered in the free-list.
 *  - We use "following" and "preceding" to refer to adjacent blocks in memory.
 *  - Pointers in the free-list will point to the beginning of a heap block
 *    (i.e., to the header).
 *  - Pointers returned by mm_malloc point to the beginning of the payload
 *    (i.e., to the word after the header).
 *
 * ALLOCATOR BLOCKS:
 *  - See definition of block_info struct fields further down
 *  - USED: +---------------+   FREE: +---------------+
 *          |    header     |         |    header     |
 *          |(size_and_tags)|         |(size_and_tags)|
 *          +---------------+         +---------------+
 *          |  payload and  |         |   next ptr    |
 *          |    padding    |         +---------------+
 *          |       .       |         |   prev ptr    |
 *          |       .       |         +---------------+
 *          |       .       |         |  free space   |
 *          |               |         |  and padding  |
 *          |               |         |      ...      |
 *          |               |         +---------------+
 *          |               |         |    footer     |
 *          |               |         |(size_and_tags)|
 *          +---------------+         +---------------+
 *
 * BOUNDARY TAGS:
 *  - Headers and footers for a heap block store identical information.
 *  - The block size is stored as a word, but because of alignment, we can use
 *    some number of the least significant bits as tags/flags.
 *  - TAG_USED is bit 0 (the 1's digit) and indicates if this heap block is
 *    used/allocated.
 *  - TAG_PRECEDING_USED is bit 1 (the 2's digit) and indicates if the
 *    preceding heap block is used/allocated. Used for coalescing and avoids
 *    the need for a footer in used/allocated blocks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"


// Static functions for unscaled pointer arithmetic to keep other code cleaner.
//  - The first argument is void* to enable you to pass in any type of pointer
//  - Casting to char* changes the pointer arithmetic scaling to 1 byte
//    (e.g., UNSCALED_POINTER_ADD(0x1, 1) returns 0x2)
//  - We cast the result to void* to force you to cast back to the appropriate
//    type and ensure you don't accidentally use the resulting pointer as a
//    char* implicitly.
static inline void* UNSCALED_POINTER_ADD(void* p, int x) { return ((void*)((char*)(p) + (x))); }
static inline void* UNSCALED_POINTER_SUB(void* p, int x) { return ((void*)((char*)(p) - (x))); }


// A block_info can be used to access information about a heap block,
// including boundary tag info (size and usage tags in header and footer)
// and pointers to the next and previous blocks in the free-list.
struct block_info {
    // Size of the block and tags (preceding-used? and used? flags) combined
	// together. See the SIZE() function and TAG macros below for more details
	// and how to extract these pieces of info.
    size_t size_and_tags;
    // Pointer to the next block in the free list.
    struct block_info* next;
    // Pointer to the previous block in the free list.
    struct block_info* prev;
};
typedef struct block_info block_info;


// Pointer to the first block_info in the free list, the list's head.
// In this implementation, this is stored in the first word in the heap and
// accessed via mem_heap_lo().
#define FREE_LIST_HEAD *((block_info **)mem_heap_lo())

// Size of a word on this architecture.
#define WORD_SIZE sizeof(void*)

// Minimum block size (accounts for header, next ptr, prev ptr, and footer).
#define MIN_BLOCK_SIZE (sizeof(block_info) + WORD_SIZE)

// Alignment requirement for allocator.
#define ALIGNMENT 8

// SIZE(block_info->size_and_tags) extracts the size of a 'size_and_tags' field.
// SIZE(size) returns a properly-aligned value of 'size' (by rounding down).
static inline size_t SIZE(size_t x) { return ((x) & ~(ALIGNMENT - 1)); }

// Bit mask to use to extract or set TAG_USED in a boundary tag.
#define TAG_USED 1

// Bit mask to use to extract or set TAG_PRECEDING_USED in a boundary tag.
#define TAG_PRECEDING_USED 2


/*
 * Print the heap by iterating through it as an implicit free list.
 *  - For debugging; make sure to remove calls before submission as will affect
 *    throughput.
 *  - Can ignore compiler warning about this function being unused.
 */
static void examine_heap() {
  block_info* block;

  // print to stderr so output isn't buffered and not output if we crash
  fprintf(stderr, "FREE_LIST_HEAD: %p\n", (void*) FREE_LIST_HEAD);

  for (block = (block_info*) UNSCALED_POINTER_ADD(mem_heap_lo(), WORD_SIZE);  // first block on heap
       SIZE(block->size_and_tags) != 0 && block < (block_info*) mem_heap_hi();
       block = (block_info*) UNSCALED_POINTER_ADD(block, SIZE(block->size_and_tags))) {

    // print out common block attributes
    fprintf(stderr, "%p: %ld %ld %ld\t",
            (void*) block,
            SIZE(block->size_and_tags),
            block->size_and_tags & TAG_PRECEDING_USED,
            block->size_and_tags & TAG_USED);

    // and allocated/free specific data
    if (block->size_and_tags & TAG_USED) {
      fprintf(stderr, "ALLOCATED\n");
    } else {
      fprintf(stderr, "FREE\tnext: %p, prev: %p\n",
              (void*) block->next,
              (void*) block->prev);
    }
  }
  fprintf(stderr, "END OF HEAP\n\n");
}


/*
 * Find a free block of the requested size in the free list.
 * Returns NULL if no free block is large enough.
 */
static block_info* search_free_list(size_t req_size) {
  block_info* free_block;

  free_block = FREE_LIST_HEAD;
  while (free_block != NULL) {
    if (SIZE(free_block->size_and_tags) >= req_size) {
      return free_block;
    } else {
      free_block = free_block->next;
    }
  }
  return NULL;
}


/* Insert free_block at the head of the list (LIFO). */
static void insert_free_block(block_info* free_block) {
  block_info* old_head = FREE_LIST_HEAD;
  free_block->next = old_head;
  if (old_head != NULL) {
    old_head->prev = free_block;
  }
  free_block->prev = NULL;
  FREE_LIST_HEAD = free_block;
}


/* Remove a free block from the free list. */
static void remove_free_block(block_info* free_block) {
  block_info* next_free;
  block_info* prev_free;

  next_free = free_block->next;
  prev_free = free_block->prev;

  // If the next block is not null, patch its prev pointer.
  if (next_free != NULL) {
    next_free->prev = prev_free;
  }

  // If we're removing the head of the free list, set the head to be
  // the next block, otherwise patch the previous block's next pointer.
  if (free_block == FREE_LIST_HEAD) {
    FREE_LIST_HEAD = next_free;
  } else {
    prev_free->next = next_free;
  }
}


/* Coalesce 'old_block' with any preceding or following free blocks. */
static void coalesce_free_block(block_info* old_block) {
  block_info* block_cursor;
  block_info* new_block;
  block_info* free_block;
  // size of old block
  size_t old_size = SIZE(old_block->size_and_tags);
  // running sum to be size of final coalesced block
  size_t new_size = old_size;

  // Coalesce with any preceding free block
  block_cursor = old_block;
  while ((block_cursor->size_and_tags & TAG_PRECEDING_USED) == 0) {
    // While the block preceding this one in memory (not the
    // prev. block in the free list) is free:

    // Get the size of the previous block from its boundary tag.
    size_t size = SIZE(*((size_t*) UNSCALED_POINTER_SUB(block_cursor, WORD_SIZE)));
    // Use this size to find the block info for that block.
    free_block = (block_info*) UNSCALED_POINTER_SUB(block_cursor, size);
    // Remove that block from free list.
    remove_free_block(free_block);

    // Count that block's size and update the current block pointer.
    new_size += size;
    block_cursor = free_block;
  }
  new_block = block_cursor;

  // Coalesce with any following free block.
  // Start with the block following this one in memory
  block_cursor = (block_info*) UNSCALED_POINTER_ADD(old_block, old_size);
  while ((block_cursor->size_and_tags & TAG_USED) == 0) {
    // While following block is free:

    size_t size = SIZE(block_cursor->size_and_tags);
    // Remove it from the free list.
    remove_free_block(block_cursor);
    // Count its size and step to the following block.
    new_size += size;
    block_cursor = (block_info*) UNSCALED_POINTER_ADD(block_cursor, size);
  }

  // If the block actually grew, remove the old entry from the free-list
  // and add the new entry.
  if (new_size != old_size) {
    // Remove the original block from the free list
    remove_free_block(old_block);

    // Save the new size in the block info and in the boundary tag
    // and tag it to show the preceding block is used (otherwise, it
    // would have become part of this one!).
    new_block->size_and_tags = new_size | TAG_PRECEDING_USED;
    // The boundary tag of the preceding block is the word immediately
    // preceding block in memory where we left off advancing block_cursor.
    *(size_t*) UNSCALED_POINTER_SUB(block_cursor, WORD_SIZE) = new_size | TAG_PRECEDING_USED;

    // Put the new block in the free list.
    insert_free_block(new_block);
  }
  return;
}


/* Get more heap space of size at least req_size. */
static void request_more_space(size_t req_size) {
  size_t pagesize = mem_pagesize();
  size_t num_pages = (req_size + pagesize - 1) / pagesize;
  block_info* new_block;
  size_t total_size = num_pages * pagesize;
  size_t prev_last_word_mask;

  void* mem_sbrk_result = mem_sbrk(total_size);
  if ((size_t) mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in request_more_space\n");
    exit(0);
  }
  new_block = (block_info*) UNSCALED_POINTER_SUB(mem_sbrk_result, WORD_SIZE);

  // Initialize header by inheriting TAG_PRECEDING_USED status from the
  // end-of-heap word and resetting the TAG_USED bit.
  prev_last_word_mask = new_block->size_and_tags & TAG_PRECEDING_USED;
  new_block->size_and_tags = total_size | prev_last_word_mask;
  // Initialize new footer
  ((block_info*) UNSCALED_POINTER_ADD(new_block, total_size - WORD_SIZE))->size_and_tags =
          total_size | prev_last_word_mask;

  // Initialize new end-of-heap word: SIZE is 0, TAG_PRECEDING_USED is 0,
  // TAG_USED is 1. This trick lets us do the "normal" check even at the end
  // of the heap.
  *((size_t*) UNSCALED_POINTER_ADD(new_block, total_size)) = TAG_USED;

  // Add the new block to the free list and immediately coalesce newly
  // allocated memory space.
  insert_free_block(new_block);
  coalesce_free_block(new_block);
}


/* Initialize the allocator. */
int mm_init() {
  // Head of the free list.
  block_info* first_free_block;

  // Initial heap size: WORD_SIZE byte heap-header (stores pointer to head
  // of free list), MIN_BLOCK_SIZE bytes of space, WORD_SIZE byte heap-footer.
  size_t init_size = WORD_SIZE + MIN_BLOCK_SIZE + WORD_SIZE;
  size_t total_size;

  void* mem_sbrk_result = mem_sbrk(init_size);
  //  printf("mem_sbrk returned %p\n", mem_sbrk_result);
  if ((ssize_t) mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in mm_init, returning %p\n",
           mem_sbrk_result);
    exit(1);
  }

  first_free_block = (block_info*) UNSCALED_POINTER_ADD(mem_heap_lo(), WORD_SIZE);

  // Total usable size is full size minus heap-header and heap-footer words.
  // NOTE: These are different than the "header" and "footer" of a block!
  //  - The heap-header is a pointer to the first free block in the free list.
  //  - The heap-footer is the end-of-heap indicator (used block with size 0).
  total_size = init_size - WORD_SIZE - WORD_SIZE;

  // The heap starts with one free block, which we initialize now.
  first_free_block->size_and_tags = total_size | TAG_PRECEDING_USED;
  first_free_block->next = NULL;
  first_free_block->prev = NULL;
  // Set the free block's footer.
  *((size_t*) UNSCALED_POINTER_ADD(first_free_block, total_size - WORD_SIZE)) =
	  total_size | TAG_PRECEDING_USED;

  // Tag the end-of-heap word at the end of heap as used.
  *((size_t*) UNSCALED_POINTER_SUB(mem_heap_hi(), WORD_SIZE - 1)) = TAG_USED;

  // Set the head of the free list to this new free block.
  FREE_LIST_HEAD = first_free_block;
  return 0;
}


// TOP-LEVEL ALLOCATOR INTERFACE ------------------------------------

/*
 * Allocate a block of size size and return a pointer to it. If size is zero,
 * returns NULL.
 */
void* mm_malloc(size_t size) {
  size_t req_size;
  //The size the block needs to be based on the size we put in and the alignment setup (the latter was already coded for us)
  block_info* ptr_free_block = NULL;
  //The block we want to use
  block_info* fwd_block_info = NULL;
  //the block after that one we "malloced". We need to consider this because we must track a block's preceding_block_use_tag for the next block in line to further maintain proper   setup
  int split_free_size = 0;
  //Temp variable I declared for the specfic edge case of having extra free space in the block that was found therefore splitting into an allocated and free space
  size_t block_size;
  //variable to be used with the SIZE static inline function
  size_t preceding_block_use_tag;
  //tag used for the following block
  block_info* free_remainder_1 = NULL;
  block_info* free_remainder_2 = NULL;
  //Preceding two variables kinda like temp variables used in split case

  // Zero-size requests get NULL.
  if (size == 0) {
    return NULL;
  }

  // Add one word for the initial size header.
  // Note that we don't need a footer when the block is used/allocated!
  size += WORD_SIZE;
  if (size <= MIN_BLOCK_SIZE) {
    // Make sure we allocate enough space for the minimum block size.
    req_size = MIN_BLOCK_SIZE;
  } else {
    // Round up for proper alignment.
    req_size = ALIGNMENT * ((size + ALIGNMENT - 1) / ALIGNMENT);
  }

  // TODO: Implement mm_malloc.  You can change or remove any of the
  // above code.  It is included as a suggestion of where to start.
  // You will want to replace this return statement...
  
  //payload will be WORD_SIZE ahead this
  //request_more_space(req_size);
  
  ptr_free_block = search_free_list(req_size);
  //Thankfully we do not have to code search_free_list jesus christ - This method will search the doubly-linked list for the first block that fits that size we need according what we requested AND what our alignment setup dictates

  if(!ptr_free_block){//If we cannot find a block that fits, the previous call would have returned null - we need to request more space
	  request_more_space(req_size);
	  //add more space to the HEAP AS A WHOLE
	  ptr_free_block = search_free_list(req_size);
	  //Search again now that we have gurantereed we have enough space
  }

 
   
  	  
  //remove free block from list
  remove_free_block(ptr_free_block);

  block_size = SIZE(ptr_free_block->size_and_tags);
  //Now we put the correct size into block size

  preceding_block_use_tag = ptr_free_block->size_and_tags & TAG_PRECEDING_USED;
  //By "anding" our current block's size_and_tags with TAG_PRECEDING_USED we combined our current block's tag with the tag marking the previous block as used to be used for the    next block -> where now, from the next block's perspective, the block we malloced is the preceding block


  //We use this variable as this is a common value used repeatadly that we need to determine if we split or not
  split_free_size = block_size - req_size;

  if(split_free_size < MIN_BLOCK_SIZE){//NOT THE SPLITTING CASE - where we have no extra space or the extra space is too small to reenter the free list and instead made into padding for the malloced block
	
  	//figure out if splitting the block is needed
     	

  	//set size and tag bits to show that this block has been used
  	ptr_free_block->size_and_tags = ptr_free_block->size_and_tags | TAG_USED;

  	//following block (the next block after the one we malloced)
  	fwd_block_info = (block_info*)UNSCALED_POINTER_ADD(ptr_free_block, block_size);

	//The next block's preceding block is the current block. We do a similar things with tags but with the tag that shows the prior (preceding block has been used)
  	fwd_block_info->size_and_tags = fwd_block_info->size_and_tags | TAG_PRECEDING_USED;
  }else{
	//The split case - If we have extra free space - reinserting the rest of the block back into the free list.
  	//insert_free_block(UNSCALED_POINTER_ADD(ptr_free_block, req_size) );
	
        //ptr_free_block = (block_info*)UNSCALED_POINTER_ADD(ptr_free_block, block_size + MIN_BLOCK_SIZE);
	
	//We use this thing
        //split_free_size = block_size - req_size;

        ptr_free_block->size_and_tags = req_size | preceding_block_use_tag;
	//Here this is not an obvious line of code but it essentially setting the preceding_block_size tag to on on the next block because from the perspective of the next block,         which is now the piece of the block split to be free and put back into that list, which has a size of AT LEAST req_size, 
	ptr_free_block->size_and_tags = ptr_free_block->size_and_tags | TAG_USED;
	//This, unlike the above line, is obvious. It sets the block to be marked as used 
	//ptr_free_block->size_and_tags = req_size | preceding_block_use_tag;
	free_remainder_1 = (block_info*)UNSCALED_POINTER_ADD(ptr_free_block, req_size);
        //setting up our first temp block struct to be pointing at our mainblock plus req_size, which points to start of the free block
        free_remainder_2 = (block_info*)UNSCALED_POINTER_ADD(ptr_free_block, block_size - WORD_SIZE);
	//setting up our second temp block struct to be pointing at our mainblock plus the size of our block - WORD_SIZE, where WORD_SIZE has always been the size of the header or        footer, which means that this is the size of our block - footer.size since we went up two block_sizes to put us at the begining of the footer for the free block.
	
        free_remainder_1->size_and_tags = split_free_size | TAG_PRECEDING_USED;
	free_remainder_2->size_and_tags = split_free_size | TAG_PRECEDING_USED;
        //Set the preceding_used_tag in both the header and the footer of the free piece
         
        insert_free_block(UNSCALED_POINTER_ADD(ptr_free_block, req_size));
	//putting the free piece back into its list.
  }
  
  

  return ((void*)UNSCALED_POINTER_ADD(ptr_free_block, WORD_SIZE));
  //Remember, we need to return to the payload, not the front of the block/header
}


/* Free the block referenced by ptr. */
void mm_free(void* ptr) {//Do not forget this ptr lmao
  size_t payload_size;
  //The amount of usable space in a block
  block_info* block_to_free;
  //the full block
  block_info* following_block;
  //the next block -> remember in mm_malloc, we had to go the next block to modify its preceding used tag
  block_info* footer_tag;
  //used to store data that reaches the footer that should be the same as the header

  // TODO: Implement mm_free.  You can change or remove the declaraions
  // above.  They are included as minor hints.
  
  block_to_free = UNSCALED_POINTER_SUB(ptr, WORD_SIZE);
  //So many god damn seg faults until I realized that I had block_to_pre in unscaledpointersub as opposed to ptr. GOD WHYYYYYYYYYYYYYYYYYYYYYYYYYYY
  //What this actully does is bring the ptr back a word_size AKA black a header to the start of the block

  payload_size = SIZE(block_to_free->size_and_tags);
  //using the static inline function to get the size -> the size in the tags of a malloc block have only ever pointed to the payload size
  following_block =  UNSCALED_POINTER_ADD(block_to_free, payload_size);
  //We plug this into following_block because otherwise gcc gets really mad. The sum moves us to the next block
  following_block->size_and_tags = following_block->size_and_tags & (~TAG_PRECEDING_USED);
  //since from the perspective of the next block the previous block is the current block, we change its tag by masking it with the flipped version of TAG_PRECEDING_USED which has   1s in a bit places except for the 2nd one since the tag originally had the value of 1, turning it spefically in 0. With an & 1 for the rest of the bits, they are peserved, but w  e want to reset this tag because it is no longer true -> the previous block is now free

  footer_tag = (block_info*)UNSCALED_POINTER_ADD(block_to_free, payload_size - WORD_SIZE);
  //Holding this in temp so C will not have a heart attack -> Adding payload_size -WORD_SIZE will go to the footer of the block. We want this since free blocks have footers that a  re copies of the header while allocated blocks do not, forcing us to make sure we make it. This is the 2nd command down from here
  block_to_free->size_and_tags = block_to_free->size_and_tags & (~TAG_USED);
  //similar to what we did with the following block but for the header (we have not used temp yet and therefore have not touched the footer) -> resetting the used bit
  footer_tag->size_and_tags = block_to_free->size_and_tags;
  //Setting the footer to have the same tags as the header

  insert_free_block(block_to_free);
  coalesce_free_block(block_to_free);
  //easiest piece -> reinserting block into free list and coalescing the now free piece
}


/*
 * A heap consistency checker. Optional, but recommended to help you debug
 * potential issues with your allocator.
 */
int mm_check() {
  // TODO: Implement a heap consistency checker as needed/desired.
  return 0;
}
