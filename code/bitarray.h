#ifndef BITARRAY_H_
#define BITARRAY_H_

typedef unsigned char bit;		// there is no 1 bit size value in C
typedef unsigned long tree_node;

const static unsigned char mask_on[8] = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1
		<< 5, 1 << 6, 1 << 7 };
const static unsigned char mask_off[8] = { 1 ^ -1, (1 << 1) ^ -1, (1 << 2) ^ -1,
		(1 << 3) ^ -1, (1 << 4) ^ -1, (1 << 5) ^ -1, (1 << 6) ^ -1, (1 << 7)
				^ -1 };

/* bit array */
#define BITS(size) ((size>>3) + ((size&7) > 0))							// for size bits we need size/8 chars
#define BITAT(bitarray, index) ((bitarray[index>>3] & mask_on[index&7]) > 0)			// bitarray[index]
#define BITSET(bitarray, index) (bitarray[index>>3] |= mask_on[index&7])			// bitarray[index] = 1
#define BITUNSET(bitarray, index) (bitarray[index>>3] &= mask_off[index&7])			// bitarray[index] = 0
#define SETBITS(bitarray, size) do {int i=0; while(i<BITS(size)) bitarray[i++] = 0;} while (0)	// memcpy(bitarray, 0, size)

/* bit tree */
#define TREE_COVERAGE(size) (BITS(size*2))
#define TREE_LEFT(index) ((index<<1)+1)
#define TREE_RIGHT(index) ((index<<1)+2)
#define TREE_PARENT(index) ((index-1)>>1)

#endif /* BITARRAY_H_ */
