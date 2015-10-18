#include "memmanage.h"
#include "system.h"
#include <stdio.h>

/*
 * static functions definition
 */
// returns the first index of a free cons leaf in the cons marks tree, -1 if not found
static int find_free_cons(int current_tree_index);
// update the cons marks tree, from cons pool index which have been changed
static void update_tree_upwards(int current_tree_index);
// recursively re-marks cons
static void rec_cons_mark(memptr address);
// marks the string connectors
static void rec_connectors_mark(memptr address);
// moves a connector block in the pool
static void connectors_swap(memptr address1, memptr parent1, bool is_car1,
		memptr address2, memptr parent2, bool is_car2);
// move the string connectors to the start of the pool
static void rec_connectors_redeploy(int *filler, memptr address);
// sorts the string connectors by string address. assumes they all are the start
// of the pool
static void connectors_sort();
// moves the strings to the start of the strings pool
static void rec_string_redeploy(int *filler, memptr parent);
// mark and sweep algorithm for the memory system. one can assume that allocation
// has failed after mark_sweep if and only if the buffer is full
static void mark_sweep();

void initialize_memory_system() {

	// all fillers point the start of the pools
	connectors_filler = 0;
	strings_filler = 0;
	// cons marks are cleared
	SETBITS(cons_marks_tree, 0, TREE_COVERAGE(MEM_SIZE));
}

memptr new_data() {

	int tree_index = find_free_cons(0);
	if (tree_index == NOT_FOUND) {
		mark_sweep();
		tree_index = find_free_cons(0);
		if (tree_index == NOT_FOUND) {
			fprintf(stderr, "Memory Failue => system out of memory\n");
			exit(0);
		}
	}

	BITSET(cons_marks_tree, tree_index);
	update_tree_upwards(tree_index);

	int mem_index = tree_index - (MEM_SIZE - 1);
	MemoryPool[mem_index].carKind = MemoryPool[mem_index].cdrKind = NIL;
	return mem_index;
}

memptr connect_string(char *str, memptr parent) {

	int len = strlen(str);
	if (connectors_filler >= CONNECTORS_POOL_SIZE
			|| strings_filler + len >= STRING_POOL_SIZE) {
		mark_sweep();
		if (connectors_filler >= CONNECTORS_POOL_SIZE
				|| strings_filler + len >= STRING_POOL_SIZE) {
			fprintf(stderr, "Memory Failue => system out of memory\n");
			exit(0);
		}
	}

	strcpy(&(StringsPool[strings_filler]), str);
	ConnectorsPool[connectors_filler].string = strings_filler;
	ConnectorsPool[connectors_filler].parent = parent;
	strings_filler += len + 1;
	return connectors_filler++;
}

char* get_string(memptr connector) {

	return &(StringsPool[ConnectorsPool[connector].string]);
}

void collect_garbage() {

	mark_sweep();
}

/*
 * static functions implementation
 */

static int find_free_cons(int current_tree_index) {

	if (BITAT(cons_marks_tree, current_tree_index)) {
		return NOT_FOUND;
	}
	if (current_tree_index >= MEM_SIZE - 1) {
		return current_tree_index;
	}
	int rec_res_left = find_free_cons(TREE_LEFT(current_tree_index));
	if (rec_res_left != NOT_FOUND) {
		return rec_res_left;
	}
	int rec_res_right = find_free_cons(TREE_RIGHT(current_tree_index));
	return rec_res_right;
}

static void update_tree_upwards(int current_tree_index) {

	int parent = TREE_PARENT(current_tree_index);
	if (parent < 0) {
		return;
	}
	if (BITAT(cons_marks_tree, TREE_LEFT(parent)) == 1
			&& BITAT(cons_marks_tree, TREE_RIGHT(parent)) == 1) {
		BITSET(cons_marks_tree, parent);
	}
	if (BITAT(cons_marks_tree, TREE_LEFT(parent)) == 0
			&& BITAT(cons_marks_tree, TREE_RIGHT(parent)) == 0) {
		BITUNSET(cons_marks_tree, parent);
	}
	update_tree_upwards(parent);
}

static void rec_cons_mark(memptr address) {

	if (BITAT(cons_marks_tree, address) == 1) {
		return;
	}
	BITSET(cons_marks_tree, address+(MEM_SIZE-1));
	update_tree_upwards(address);

	// recursive call
	if (MemoryPool[address].carKind == CONS) {
		rec_cons_mark(MemoryPool[address].car);
	}
	if (MemoryPool[address].cdrKind == CONS) {
		rec_cons_mark(MemoryPool[address].cdr);
	}
}

static void rec_connectors_mark(memptr address) {

	if (BITAT(connectors_visited_array, address)) {
		return;
	}
	BITSET(connectors_visited_array, address);

	// mark string connectors sons
	if (MemoryPool[address].carKind == STRING) {
		BITSET(connectors_marks_array, MemoryPool[address].car);
	}
	if (MemoryPool[address].cdrKind == STRING) {
		BITSET(connectors_marks_array, MemoryPool[address].cdr);
	}

	// recursive call
	if (MemoryPool[address].carKind == CONS) {
		rec_connectors_mark(address);
	}
	if (MemoryPool[address].cdrKind == CONS) {
		rec_connectors_mark(address);
	}
}

static void connectors_swap(memptr address1, memptr parent1, bool is_car1,
		memptr address2, memptr parent2, bool is_car2) {

	Connector temp;
	temp.parent = ConnectorsPool[address1].parent;
	temp.string = ConnectorsPool[address1].string;
	ConnectorsPool[address1].parent = ConnectorsPool[address2].parent;
	ConnectorsPool[address1].string = ConnectorsPool[address2].string;
	ConnectorsPool[address2].parent = temp.parent;
	ConnectorsPool[address2].string = temp.string;
	if (parent1 != NOT_FOUND) {
		if (is_car1) {
			MemoryPool[parent1].car = address2;
		} else {
			MemoryPool[parent1].cdr = address2;
		}
	}
	if (parent2 != NOT_FOUND) {
		if (is_car2) {
			MemoryPool[parent1].car = address1;
		} else {
			MemoryPool[parent1].cdr = address1;
		}
	}
}

static void rec_connectors_redeploy(int *filler, memptr address) {

	memptr sc_location;
	while (BITAT(connectors_marks_array, *filler) == 1)
		(*filler)++;
	if (MemoryPool[address].carKind == STRING) {
		sc_location = MemoryPool[address].car;
		if (*filler < sc_location) {
			// we can move the connector closer to the start of the pool
			connectors_swap(sc_location, address, true, *filler,
			NOT_FOUND, false);
			BITSET(connectors_marks_array, *filler);
			BITUNSET(connectors_marks_array, sc_location);
		}
	}
	while (BITAT(connectors_marks_array, *filler) == 1)
		(*filler)++;
	if (MemoryPool[address].cdrKind == STRING) {
		sc_location = MemoryPool[address].cdr;
		if (*filler < sc_location) {
			// we can move the connector closer to the start of the pool
			connectors_swap(sc_location, address, false, *filler,
			NOT_FOUND, false);
			BITSET(connectors_marks_array, *filler);
			BITUNSET(connectors_marks_array, sc_location);
		}
	}

	// recursive call to the replacement function
	if (MemoryPool[address].carKind == CONS) {
		rec_connectors_redeploy(filler, MemoryPool[address].car);
	}
	if (MemoryPool[address].cdrKind == CONS) {
		rec_connectors_redeploy(filler, MemoryPool[address].cdr);
	}
}

static void connectors_sort() {

	// a simple bubble sort for the second level (string connectors)
	bool done;
	memptr parent1, parent2;
	bool is_car1, is_car2;
	for (int i = 0; i < connectors_filler; i++) {
		done = true;
		for (int j = 0; j < connectors_filler - i - 1; j++) {
			if (ConnectorsPool[j].string > ConnectorsPool[j + 1].string) {
				done = false;
				// parent of connector j
				parent1 = ConnectorsPool[j].parent;
				if (MemoryPool[ConnectorsPool[j].parent].carKind == STRING
						&& MemoryPool[ConnectorsPool[j].parent].car == j) {
					is_car1 = true;
				} else if (MemoryPool[ConnectorsPool[j].parent].cdrKind
						== STRING
						&& MemoryPool[ConnectorsPool[j].parent].cdr == j) {
					is_car1 = false;
				}
				// parent of connector j+1
				parent2 = ConnectorsPool[j + 1].parent;
				if (MemoryPool[ConnectorsPool[j + 1].parent].carKind == STRING
						&& MemoryPool[ConnectorsPool[j + 1].parent].car
								== j + 1) {
					is_car2 = true;
				} else if (MemoryPool[ConnectorsPool[j + 1].parent].cdrKind
						== STRING
						&& MemoryPool[ConnectorsPool[j + 1].parent].cdr
								== j + 1) {
					is_car2 = false;
				}
				// swap and update parents field
				connectors_swap(j, parent1, is_car1, j + 1, parent2, is_car2);
			}
		}
		if (done) {
			break;
		}
	}
}

static void rec_string_redeploy(int *filler, memptr parent) {

	memptr string_address = ConnectorsPool[parent].string;
	int len = strlen(&(StringsPool[string_address])) + 1;
	if (*filler < string_address) {
		// can move the string to the start of the pool
		strcpy(&(StringsPool[*filler]), &(StringsPool[string_address]));
		// update the father
		ConnectorsPool[parent].string = *filler;
	}
	// update filler
	*filler += len;
}

// TODO: check validity of base group iteration, code transfered from previous version
static void mark_sweep() {

	/*
	 * data mark and sweep
	 */
	// zero the cons marks tree
	SETBITS(cons_marks_tree, 0, TREE_COVERAGE(MEM_SIZE));

	// re-marking, base group iteration
	rec_cons_mark(nil);
	rec_cons_mark(t);
	rec_cons_mark(environment);
	rec_cons_mark(security_head);
	rec_cons_mark(security_wait);

	/*
	 * string connectors mark and sweep
	 */
	// initialization of the marks bit array
	SETBITS(connectors_marks_array, 0, MARKS_SIZE);
	SETBITS(connectors_visited_array, 0, MARKS_SIZE);

	// marking
	rec_connectors_mark(nil);
	rec_connectors_mark(t);
	rec_connectors_mark(environment);
	rec_connectors_mark(security_head);
	rec_connectors_mark(security_wait);

	// sweeping
	int connectors_new_filler = 0;
	rec_connectors_redeploy(&connectors_new_filler, nil);
	rec_connectors_redeploy(&connectors_new_filler, t);
	rec_connectors_redeploy(&connectors_new_filler, environment);
	rec_connectors_redeploy(&connectors_new_filler, security_head);
	rec_connectors_redeploy(&connectors_new_filler, security_wait);

	// limit correction
	while (BITAT(connectors_marks_array, connectors_new_filler) == 1)
		connectors_new_filler++;
	connectors_filler = connectors_new_filler;

	// sorting, in order to commit the last part of the algorithm
	connectors_sort();

	/*
	 * strings sweep
	 */
	// sweeping
	int strings_new_filler = 0;
	for (int i = 0; i < connectors_filler; i++) {
		rec_string_redeploy(&strings_new_filler, i);
	}

	// limit correction
	strings_filler = strings_new_filler;
}
