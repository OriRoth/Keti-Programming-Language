#ifndef MEMMANAGE_H_
#define MEMMANAGE_H_

#include <stdbool.h>
#include "bitarray.h"

/*
 * ******************************************************************
 *                                                                  *
 * implements a doubled-leveled memory system, supporting cons      *
 * pool and a string pool                                           *
 *                                                                  *
 ********************************************************************
 */

enum {
	// How many bits for index into pool:
	LG2_POOLSIZE = 14,
	// How many bits for storing car/cdr kind:
	KIND_SIZE = 2
};

#define NOT_FOUND (-1)
#define MEM_SIZE (1 << 14)
#define STRING_POOL_SIZE (1<<16) // TODO: update
#define MARKS_SIZE CONNECTORS_POOL_SIZE

// a pointer to a static array would be an integer index
typedef unsigned int memptr;

enum kind {
	NIL = 0, STRING, INTEGER, CONS
};
typedef struct Cons_t {
	unsigned int carKind :KIND_SIZE;
	unsigned int car :LG2_POOLSIZE;
	unsigned int cdrKind :KIND_SIZE;
	unsigned int cdr :LG2_POOLSIZE;
} Cons;
typedef struct {
	memptr parent;
	memptr start;
} StringHandle;

typedef struct AtomElem_t {
	const char *name;
	int value;
} AtomElem;

/*
 * Globals
 */
// Memory Pool of struct Cons nodes - Virtualization of Heap
Cons MemoryPool[MEM_SIZE];
// connectors pool
Connector ConnectorsPool[MEM_SIZE];
// The strings pool
char StringsPool[STRING_POOL_SIZE];
// marks tree array for cons pool
bit cons_marks_tree[TREE_COVERAGE(MEM_SIZE)];
// marks array for connectors mark and sweep
bit connectors_marks_array[BITS(MEM_SIZE)];
bit connectors_visited_array[BITS(MEM_SIZE)];
// fillers for connectors pool and strings pool
int connectors_filler;
int strings_filler;

/*
 * initialize the memory system, represented by info structure
 */
void initialize_memory_system();

/*
 * return an index of a free MemoryPool block, and marks it as occupied
 */
memptr new_data();

/*
 * copies the string into StringsPool, and returns its strings tree node
 */
memptr connect_string(char *str, memptr parent);

/*
 * returns the address of the string pointed by the connector
 */
char* get_string(memptr conector);

/*
 * a call to the mark and sweep algorithm
 */
void collect_garbage();

#endif /* MEMMANAGE_H_ */
