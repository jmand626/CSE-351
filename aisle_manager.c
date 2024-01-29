/*
 * CSE 351 Lab 1b (Manipulating Bits in C)
 *
 * Name(s): Joban Mand, Smayan Nirantare 
 * NetID(s): jmand1, smayan
 *
 * ----------------------------------------------------------------------------
 * Overview
 * ----------------------------------------------------------------------------
 *  This is a program to keep track of the items in a small aisle of a store.
 *
 *  A store's aisle is represented by a 64-bit long integer, which is broken
 *  into 4 16-bit sections representing one type of item each. Note that since
 *  a section is 16-bits, it fits nicely into C's short datatype.
 *
 *  Aisle Layout:
 *
 *    Within an aisle, sections are indexed starting with the least-significant
 *    section being at index 0 and continuing up until one less than the number
 *    of sections.
 *
 *    Example aisle:
 *
 *                MSB                                                       LSB
 *                  +-------------+-------------+-------------+-------------+
 *                  |  Section 3  |  Section 2  |  Section 1  |  Section 0  |
 *                  +-------------+-------------+-------------+-------------+
 *                  |             |             |             |             |
 *      bit offset: 64            48            32            16            0
 *
 *  Section Layout:
 *
 *    A section in an aisle is broken into 2 parts. The 6 most significant bits
 *    represent a unique identifier for the type of item stored in that
 *    section. The rest of the bits in a section (10 least significant)
 *    indicate individual spaces for items in that section. For each of the 10
 *    bits/spaces, a 1 indicates that an item of the section's type is stored
 *    there and a 0 indicates that the space is empty.
 *
 *    Example aisle section: 0x651A
 *
 *                MSB                               LSB
 *                  +-----------+-------------------+
 *                  |0 1 1 0 0 1|0 1 0 0 0 1 1 0 1 0|
 *                  +-----------+-------------------+
 *                  | item id   | section spaces    |
 *      bit offset: 16          10                  0
 *
 *      In this example, the item id is 0b011001, and there are currently 4
 *      items stored in the section (at bit offsets 8, 4, 3, and 1) and 6
 *      vacant spaces.
 *
 *  Written by Porter Jones (pbjones@cs.washington.edu)
 */

#include "aisle_manager.h"
#include "store_util.h"

// the number of total bits in a section
#define SECTION_SIZE 16

// The number of bits in a section used for the item spaces
#define NUM_SPACES 10

// The number of bits in a section used for the item id
#define ID_SIZE 6

// The number of sections in an aisle
#define NUM_SECTIONS 4

// TODO: Fill in these with the correct hex values

// Mask for extracting a section from the least significant bits of an aisle.
// (aisle & SECTION_MASK) should preserve a section's worth of bits at the
// lower end of the aisle and set all other bits to 0. This is essentially
// extracting section 0 from the example aisle shown above.
#define SECTION_MASK 0xFFFF


// Mask for extracting the spaces bits from a section.
// (section & SPACES_MASK) should preserve all the spaces bits in a section and
// set all non-spaces bits to 0.
#define SPACES_MASK 0x3FF


// Mask for extracting the ID bits from a section.
// (section & ID_MASK) should preserve all the id bits in a section and set all
// non-id bits to 0.
#define ID_MASK 0xFC00

#define MSB1 0x8000


/* Given a pointer to an aisle and a section index, return the section at the
 * given index of the given aisle.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
unsigned short get_section(unsigned long* aisle, int index) {
  // TODO: implement this method
  // We need to cast aisle as a short because aisles are longs while sections are supposed to be shorts
  // "1 * index" figures out how many "short pointers" to add. We deference this line to get the value to put it into a short to return that
  unsigned short s =  *((short*) aisle + (1 * index));
  return s;
}

/* Given a pointer to an aisle and a section index, return the spaces of the
 * section at the given index of the given aisle. The returned short should
 * have the least 10 significant bits set to the spaces and the 6 most
 * significant bits set to 0.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
unsigned short get_spaces(unsigned long* aisle, int index) {
  // TODO: implement this method
  // First we need the section of the index we have. Thankfully, we have a function for that
  unsigned short s = get_section(aisle, index);
  //With the macro bitmask we defined earlier that we can extract the space bits so we do just that
  return s & SPACES_MASK;
}

/* Given a pointer to an aisle and a section index, return the id of the
 * section at the given index of the given aisle. The returned short should
 * have the least 6 significant bits set to the id and the 10 most significant
 * bits set to 0.
 *
 * Example: if the section is 0b0110010100011010, return 0b0000000000011001.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
unsigned short get_id(unsigned long* aisle, int index) {
  // TODO: implement this method
  // First we need the section of the index we have. Thankfully, we have a function for that
  unsigned short s = get_section(aisle, index);
  //With the macro bitmask we defined earlier we can extract the ID bits. However, we must shift them to the right and then return,
  //which is different from how the section is set up. We want the left 10 bits to be 0, so we can do a logical right shift to put 10 zeroes on the left.
  return ((s & ID_MASK) >> 10);
}

/* Given a pointer to an aisle, a section index, and a short representing a new
 * bit pattern to be used for section, set the section at the given index of
 * the given aisle to the new bit pattern.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
void set_section(unsigned long* aisle, int index, unsigned short new_section) {
  // TODO: implement this method
  // clear section at index
  *aisle = *aisle & (~( ( (unsigned long)SECTION_MASK) << (16 * index)));
  //cast new section to unsigned long -> padd with extra zeroes on left till there are 8 bytes and then or it with aisle to change bits
  *aisle = *aisle | ((unsigned long) new_section) << (16 * index) ;
   
}

/* Given a pointer to an aisle, a section index, and a short representing a new
 * bit pattern to be used for the spaces of the section, set the spaces for the
 * section at the given index of the given aisle to the new bit pattern. The
 * new spaces pattern should be in the 10 least significant bits of the given
 * bit pattern. If the new pattern uses bits outside the 10 least significant
 * bits, then the method should leave the spaces unchanged.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
void set_spaces(unsigned long* aisle, int index, unsigned short new_spaces) {
  // TODO: implement this method
  if(!( (new_spaces >> 10) > 0)){//If it fits in the 10 bit limit we put
	  //And aisle with the opposite of spaces_mask to put 10 LSBS to 0
	 *aisle = *aisle & (~( ( (unsigned long)SPACES_MASK) << (16 * index)));
	 //Put 10 LSBS to new_spaces by shitfing by each section
	 *aisle = *aisle | ((unsigned long) new_spaces) << (16 * index) ;	 
  }
}

/* Given a pointer to an aisle, a section index, and a short representing a new
 * bit pattern to be used for the id of the section, set the id for the section
 * at the given index of the given aisle to the new bit pattern. The new id
 * pattern should be in the 6 least significant bits of the given bit pattern.
 * If the new pattern uses bits outside the 6 least significant bits, then the
 * method should leave the id unchanged.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
void set_id(unsigned long* aisle, int index, unsigned short new_id) {
  // TODO: implement this method
  if(!( (new_id >> 6) > 0)){//shifting to right to check for value
	 //Anding aisle with opposite of ID_MASK to set 6 MSBS to 0 and preserving the rest 
         *aisle = *aisle & (~( ( (unsigned long)ID_MASK) << (16 * index)));
	 //setting 6 MSBS to new id
	 *aisle = *aisle | ((unsigned long) new_id) << (16 * index + 10) ; 
  }
}

/* Given a pointer to an aisle, a section index, and a space index, toggle the
 * item in the given space index of the section at the given section index in
 * the given aisle. Toggling means that if the space was previously empty, it
 * should now be filled with an item, vice-versa.
 *
 * Can assume the section index is a valid index (0-3 inclusive).
 * Can assume the spaces index is a valid index (0-9 inclusive).
 */
void toggle_space(unsigned long* aisle, int index, int space_index) {
  // TODO: implement this method
  // Flipping a bit by casting 1 to a long for 63 bits of a 0 on the right, shifting that to the space we want and exoring only that section to flip
   *aisle = *aisle ^ ( ( (unsigned long) 0x1) << (16 * index + space_index));
  
}

/* Given a pointer to an aisle and a section index, return the number of items
 * in the section at the given index of the given aisle.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
unsigned short num_items(unsigned long* aisle, int index) {
  // TODO: implement this method
  unsigned short spaces = get_spaces(aisle, index);
  int counter = 0;//going through one section
  for(int i = 0; i < 10; i++){
       if( (spaces & 0x1) == 1){//if the bit on the right side &'s to 1 its 1 and there is an item there
	       counter++;
       }
       spaces = spaces >> 1;//move to next
  }
  return (unsigned short) counter;
}

/* Given a pointer to an aisle, a section index, and the desired number of
 * items to add, add at most the given number of items to the section at the
 * given index in the given aisle. Items should be added to the least
 * significant spaces possible. If n is larger than or equal to the number of
 * empty spaces in the section, then the section should appear full after the
 * method finishes.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
void add_items(unsigned long* aisle, int index, int n) {
  // TODO: implement this method
      if( n >= 10 - num_items(aisle, index)){
            set_spaces(aisle, index, SPACES_MASK);
      }//If it just goes over everything should automatically be full
      unsigned short spaces = get_spaces(aisle, index);
      int counter = 0;
      for(int i = 0; i < 10; i++){
	 if(counter == n){//if we have added everything we needed
                 break;
         }
         if( (spaces & (unsigned short) 0x1) == 0){//if there is an empty space where we want to add
               toggle_space(aisle, index, i);
	       counter++;
         } 
         spaces = spaces >> 1;//similar to above, move to next
      }
}

/* Given a pointer to an aisle, a section index, and the desired number of
 * items to remove, remove at most the given number of items from the section
 * at the given index in the given aisle. Items should be removed from the
 * least significant spaces possible. If n is larger than or equal to the
 * number of items in the section, then the section should appear empty after
 * the method finishes.
 *
 * Can assume the index is a valid index (0-3 inclusive).
 */
void remove_items(unsigned long* aisle, int index, int n) {
  // TODO: implement this method
    if( n >= 10 - num_items(aisle, index)){
            set_spaces(aisle, index, ~SPACES_MASK);//remove everything
      }
      unsigned short spaces = get_spaces(aisle, index);
      int counter = 0;
      for(int i = 0; i < 10; i++){
         if(counter == n){
                 break;
         }
         if( (spaces & (unsigned short) 0x1) == 1){//just opposite of above
               toggle_space(aisle, index, i);
               counter++;
         }
         spaces = spaces >> 1;
      }
}

/* Given a pointer to an aisle, a section index, and a number of slots to
 * rotate by, rotate the items in the section at the given index of the given
 * aisle to the left by the given number of slots.
 *
 * Example: if the spaces are 0b0111100001, then rotating left by 2 results
 *          in the spaces     0b1110000101
 *
 * Can assume the index is a valid index (0-3 inclusive).
 * Can NOT assume n < NUM_SPACES (hint: find an equivalent rotation).
 */
void rotate_items_left(unsigned long* aisle, int index, int n) {
  // TODO: implement this method
  // keep num_shifts/n below 10
  int num_shifts = n - ((n/10)*10);
  unsigned short spaces = get_spaces(aisle, index);
  //create a mask with n 1s in the Most Signifcant Bits using arithmetic shifting
  unsigned short mask = (unsigned short)(((short)MSB1) >> (num_shifts - 1));
  //shift the 1s  to the n left-most spaces bits
  //then & that with spaces so you can extract the n left-most space bits into left_most_space_bits
  unsigned short left_most_space_bits = spaces & (mask >> 6);
  //shift these extrcted bits to the least signifcant bits
  unsigned short extracted_mask = (left_most_space_bits >> (10-num_shifts));
  //shift spaces by n and or with extracted_mask to replace extra 0s that were created - effectively looping the spaces
  unsigned new_spaces = ((spaces << num_shifts) & SPACES_MASK) | extracted_mask;
  //set new_spaces to aisle
  set_spaces(aisle, index, new_spaces);
}

/* Given a pointer to an aisle, a section index, and a number of slots to
 * rotate by, rotate the items in the section at the given index of the given
 * aisle to the right by the given number of slots.
 *
 * Example: if the spaces are 0b1000011110, then rotating right by 2 results
 *          in the spaces     0b1010000111
 *
 * Can assume the index is a valid index (0-3 inclusive).
 * Can NOT assume n < NUM_SPACES (hint: find an equivalent rotation).
 */
void rotate_items_right(unsigned long* aisle, int index, int n) {
  // TODO: implement this method
  int num_shifts = n - ((n/10)*10);
  unsigned short spaces = get_spaces(aisle, index);
  //create a mask with n 1s in the Most Signifcant Bits using arithmetic shifting
  unsigned short mask = (unsigned short)(((short)MSB1) >> (num_shifts - 1));
  //shift the 1s  to the n left-most spaces bits
  //then & that with spaces so you can extract the n right-most space bits into right_most_space_bits
  unsigned short right_most_space_bits = spaces & (mask >> (6 + (10 - num_shifts)) );
  //shift these extracted bits to the least signifcant bits
  unsigned short extracted_mask = (right_most_space_bits << (10 - num_shifts));
  //shift spaces by n and or with extracted_mask to replace extra 0s that were created - effectively looping the spaces
  unsigned new_spaces = ((spaces >> num_shifts) & SPACES_MASK) | extracted_mask;
  //set new_spaces to aisle
  set_spaces(aisle, index, new_spaces);
}
