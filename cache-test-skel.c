/*
 * CSE 351 Lab 4 (Caches and Cache-Friendly Code)
 * Part 1 - Inferring Mystery Cache Geometries
 *
 * Name(s): Joban Mand, Smayan Nirante 
 * NetID(s): jmand1, smayan
 *
 * NOTES:
 * 1. When using access_cache() you do not need to provide a "real" memory
 * addresses. You can use any convenient integer value as a memory address,
 * you should not be able to cause a segmentation fault by providing a memory
 * address out of your programs address space as the argument to access_cache.
 *
 * 2. Do NOT change the provided main function, especially the print statement.
 * If you do so, the autograder may fail to grade your code even if it produces
 * the correct result.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "support/mystery-cache.h"

//Assume we start with an empty cache for all of these

/* Returns the size (in B) of each block in the cache. */
int get_block_size(void) {
  /* YOUR CODE GOES HERE */
  int i = 0;
  //Counting!
  access_cache(0);
  //Throws itself as a sacrifice to miss so the next accesses hit!
  while(access_cache(i)){//If we miss and bring in a block from its begining, the rest of the block will be in the cahce as well. 
	//Therefore, if we access until we miss, we will go through a block
	i++;
	//Counting!
  }
  return i;
  

}


/* Returns the size (in B) of the cache. */
int get_cache_size(int block_size) {//We have block_size here and we use it!
  /* YOUR CODE GOES HERE */
  int looper = 0;
  //Counter!
  int j = 0;
  //gcc does not like it when you declare loop variables in the loop
  flush_cache();
  //Reset to get our baseline
  access_cache(0);
  //Start off with this special case

  while(access_cache(0)){//Our first miss and therefore false return here would have already happend with the previous line. We want the miss for the first address because it     would be the least-recently used when we have gone 1 past the size, overrunning the cache and therefore kicking out the block at the begining of the cache	       

	looper++;
	//How many blocks have we added
	flush_cache();
	//Reset our cache so we can add the previous amount of blocks plus 1 everytime / Also safety net for least used replacement theorem
        for(j = 0; j < looper; j++){//Looper is how many times we have added a block
	       access_cache(0 + (j * block_size) );
	//this code will, every single time we run it again, put back the same blocks in the same order as last time, plus another one, keeping the one at 0 the first one in
        }	       
  }/*The only way we would have exited the loop was if on a case of putting in the blocks again, we went pass the limit and therefore had to reset the first block. When we put a  condition like this in, access_cache at the point would try to go into the cache address beyond its size, and then it would have to revert to the first block (But that only shows when we call access on the first block again, which is the loop actually ends here). We have passed the size and now it would make it false by changing the first value for the first time in the loop, which would automatically end the  loop and get to this point. Here, we have gone one block_size too far, and so we subtract 1 block-size. */
  return looper * block_size - block_size;
}


/* Returns the associativity of the cache. */
int get_cache_assoc(int cache_size) {/*It makes sense! For any cache, if we go up by cache size, the index field will be the same, but with more the one associativity, there are multiple places to put those blocks. We can count how many times we take to fill them up before we override the first one (access_cache(0), the least-recently used) to determine associativity because the amount of spaces in a set corresponds to the associativity. If we access in multiples of size, we will only go to the first set. 2 way associative - > 2 slots -> 0 will take up the first one, size will take up the second, and size*2 will have to override one of them, and will take the slot 0 had. It took us size*2 to override, which means we found our value*/
  /* YOUR CODE GOES HERE */
  int as = 0;
  //associatvity counter
  int i = 0;
  //looper
  flush_cache();
  //reset baseline
  access_cache(0);
  //baseline
  while(access_cache(0)){//looking for second miss of this -> first override
	  as++;
	  //everytime we loop we are looking to further how far with cache size we multiply to fill out the first set. We do this in the begining since we start with as = 0
	  
	  for(i = 0; i <= as; i++){//Filling up the first set with hits for what we already have, size * 0, size * 1, size * 2, etc
		  access_cache(cache_size *i);
          }
  }
  return as;

}


/* Run the functions above on a given cache and print the results. */
int main(int argc, char* argv[]) {
  int size;
  int assoc;
  int block_size;
  char do_block_size, do_size, do_assoc;
  do_block_size = do_size = do_assoc = 0;
  if (argc == 1) {
    do_block_size = do_size = do_assoc = 1;
  } else {
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "block_size") == 0) {
        do_block_size = 1;
        continue;
      }
      if (strcmp(argv[i], "size") == 0) {
        do_size = 1;
        continue;
      }
      if (strcmp(argv[i], "assoc") == 0) {
        do_assoc = 1;
      }
    }
  }

  if (!do_block_size && !do_size && !do_assoc) {
    printf("No function requested!\n");
    printf("Usage: ./cache-test\n");
    printf("Usage: ./cache-test {block_size/size/assoc}\n");
    printf("\tyou may specify multiple functions\n");
    return EXIT_FAILURE;
  }

  cache_init(0, 0);

  block_size = size = assoc = -1;
  if (do_block_size) {
    block_size = get_block_size();
    printf("Cache block size: %d bytes\n", block_size);
  }
  if (do_size) {
    if (block_size == -1) block_size = get_block_size();
    size = get_cache_size(block_size);
    printf("Cache size: %d bytes\n", size);
  }
  if (do_assoc) {
    if (block_size == -1) block_size = get_block_size();
    if (size == -1) size = get_cache_size(block_size);
    assoc = get_cache_assoc(size);
    printf("Cache associativity: %d\n", assoc);
  }
}
