/*
 * CSE 351 Lab 1b (Manipulating Bits in C)
 *
 * Name(s): Joban Mand, Smayan Nirantare 
 * NetID(s): jmand1, smayan
 *
 * This is a file for managing a store of various aisles, represented by an
 * array of 64-bit integers. See aisle_manager.c for details on the aisle
 * layout and descriptions of the aisle functions that you may call here.
 *
 * Written by Porter Jones (pbjones@cs.washington.edu)
 */

#include <stddef.h>  // To be able to use NULL
#include "aisle_manager.h"
#include "store_client.h"
#include "store_util.h"

// Number of aisles in the store
#define NUM_AISLES 10

// Number of sections per aisle
#define SECTIONS_PER_AISLE 4

// Number of items in the stockroom (2^6 different id combinations)
#define NUM_ITEMS 64

// Global array of aisles in this store. Each unsigned long in the array
// represents one aisle.
unsigned long aisles[NUM_AISLES];

// Array used to stock items that can be used for later. The index of the array
// corresponds to the item id and the value at an index indicates how many of
// that particular item are in the stockroom.
int stockroom[NUM_ITEMS];


/* Starting from the first aisle, refill as many sections as possible using
 * items from the stockroom. A section can only be filled with items that match
 * the section's item id. Prioritizes and fills sections with lower addresses
 * first. Sections with lower addresses should be fully filled (if possible)
 * before moving onto the next section.
 */
void refill_from_stockroom() {
  // TODO: implement this function
  int id = 0;
  int shelf = 0;
  int items_to_add = 0;
  int backshelf = 0;
  for(int i = 0; i < NUM_AISLES; i++){//looping through aisles
	  unsigned long* aisle = &aisles[i];
  //We use a pointer to point to the address of the aisle we want because the functions relating to sections want a pointer and to avoid segmentation fault
	  for(int j = 0; j < 4; j++){
		  id = get_id(aisle, j);
		  //id of the current aisle and section
		  shelf = num_items(aisle, j);
		  //current amount of items in aisle
		  backshelf = stockroom[id];
		  //backroom count
		  items_to_add = 10 - shelf;
		  //the difference with 10 is the amount we want to add since 10 is the max
		  
	          if(backshelf < items_to_add){
			items_to_add = backshelf;
	          }//if we dont have enough, add as much as we can		
		  stockroom[id] -= items_to_add;//update stockroom
		  add_items(aisle, j, items_to_add); //actually add
          }
   }


}

/* Remove at most num items from sections with the given item id, starting with
 * sections with lower addresses, and return the total number of items removed.
 * Multiple sections can store items of the same item id. If there are not
 * enough items with the given item id in the aisles, first remove all the
 * items from the aisles possible and then use items in the stockroom of the
 * given item id to finish fulfilling an order. If the stockroom runs out of
 * items, you should remove as many items as possible.
 */
int fulfill_order(unsigned short id, int num) {
  // TODO: implement this function
  int items_removed = 0;
  for(int i = 0; i < NUM_AISLES; i++){
  	unsigned long* aisle = &aisles[i];
	//loop through each section
	for(int j = 0; j < 4; j++){
		//check if item id of section matched the one given for same item
	  	if(get_id(aisle, j) == id){
			int items = num_items(aisle, j);
			int num_to_remove = items;//by default we will remove all
			//if section has more items than needed to be removed, remove as many are needed to get
			//items_removed to num
			if(num - items_removed < items){//num - items_removed = "items_left"
				//if the nums of items left is less than there are here
				num_to_remove = num - items_removed;
				//only remove the amount we need
			}
			remove_items(aisle, j, num_to_remove);//remove either all of the items or the ones we need
			items_removed += num_to_remove; //couter
		}
	  	
	}
  }
  //remove from stockroom if needed
  if(items_removed < num){
	  //if stockroom has enough items
  	if(stockroom[id] >= num - items_removed){
		stockroom[id] -= num - items_removed;
		items_removed = num;
	}
	else{
		items_removed += stockroom[id];//add as much as we can
		stockroom[id] = 0;//it should surely be empyu now
	}
  }
  return items_removed;
}

/* Return a pointer to the first section in the aisles with the given item id
 * that has no items in it or NULL if no such section exists. Only consider
 * items stored in sections in the aisles (i.e., ignore anything in the
 * stockroom). Break ties by returning the section with the lowest address.
 */
unsigned short* empty_section_with_id(unsigned short id) {
  // TODO: implement this function
  int items = 0;
  for(int i = 0; i < NUM_AISLES; i++){
  	unsigned long* aisle = &aisles[i];
	//see above
	for(int j = 0; j < 4; j++){
		items = num_items(aisle, j);
		//just getting the items
		if(items == 0 && get_id(aisle, j) == id){//if that section had no items and matched the given item by id
			 return ((unsigned short*)aisle) + j;
			 //start at the aisle but cast it as a short for scaling and add j to get to the correct section
		}
	}
  }

  return NULL;
}

/* Return a pointer to the section with the most items in the store. Only
 * consider items stored in sections in the aisles (i.e., ignore anything in
 * the stockroom). Break ties by returning the section with the lowest address.
 */
unsigned short* section_with_most_items() {
  // TODO: implement this function
  int items = 0;
  int cur_max_items = 0; //max tracker
  int cur_aisle = 0; //tracks location
  int cur_sec = 0;  //tracks location
  for(int i = 0; i < NUM_AISLES; i++){
        unsigned long* aisle = &aisles[i];
        for(int j = 0; j < 4; j++){
                items = num_items(aisle, j);
		if(items > cur_max_items){//if the location we are looking at has more items that our current winner
			cur_max_items = items;
			cur_aisle = i;
			cur_sec = j;
		}

        }
  }
  return  ((unsigned short*)(aisles + cur_aisle)) + cur_sec;
  //Similar to one right above but we just neext to get to our correct aisle first since we return AFTER looking at everything
}
