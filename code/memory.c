#include "memory.h"
#include "system.h"
#include <stdio.h>

void mark_sweep();

void initialize_memory_system() {

	for (int i = 1; i < MEM_SIZE; i++) {
		free_cons_stack[i - 1] = MEM_SIZE - i - 1;
	}
	free_cons_stack_head = MEM_SIZE - 2;
	strings_filler = 0;
}

memptr allocate_cons() {

	if (free_cons_stack_head >= 0) {
		cons_pool[free_cons_stack[free_cons_stack_head]] = 0;
		return free_cons_stack[free_cons_stack_head--];
	}

	mark_sweep();
	if (free_cons_stack_head >= 0) {
		cons_pool[free_cons_stack[free_cons_stack_head]] = 0;
		return free_cons_stack[free_cons_stack_head--];
	}

	MEMORY_ERROR
	;
	exit(0);
}

void announce_allocation(int num_of_allocations) {

	if (free_cons_stack_head >= num_of_allocations - 1) {
		return;
	}

	mark_sweep();
	if (free_cons_stack_head >= num_of_allocations - 1) {
		return;
	}

	MEMORY_ERROR
	;
	exit(0);
}

// returns string handler address (16 bits)
cons allocate_string_from_string_buffer() {

	string_buffer[STRING_BUFFER_SIZE - 1] = '\0';
	int len = 0;
	while (string_buffer[len++] != '\0')
		;
	memptr handler;
	if (strings_filler + len < STRINGS_POOL_SIZE - 1) {
		handler = allocate_cons();
		SET_CAR(handler, handler_identifier);
		SET_CDR(handler, strings_filler);
		int string_buffer_iter = 0, string_filler_iter = strings_filler;
		do {
			strings_pool[string_filler_iter++] =
					string_buffer[string_buffer_iter];
		} while (string_buffer[string_buffer_iter++] != '\0');
		strings_filler += len;
		return (handler | (1 << 14));
	}

	mark_sweep();
	if (strings_filler + len < STRINGS_POOL_SIZE - 1) {
		handler = allocate_cons();
		SET_CAR(handler, handler_identifier);
		SET_CDR(handler, strings_filler);
		int string_buffer_iter = 0, string_filler_iter = strings_filler;
		do {
			strings_pool[string_filler_iter++] =
					string_buffer[string_buffer_iter];
		} while (string_buffer[string_buffer_iter++] != '\0');
		strings_filler += len;
		return (handler | (1 << 14));
	}

	MEMORY_ERROR
	;
	exit(0);
}

/*
 * Garbage Collection
 */

static void rec_mark(memptr root) {

	marks_stack[++marks_stack_head] = root;
	BITSET(cons_marks, root);
	while (marks_stack_head >= 0) {
		memptr current = marks_stack[marks_stack_head--];
		if (TYPE_STRING(GET_CAR(current))) {
			BITSET(cons_marks, STRING_HANDLER_ADDRESS(GET_CAR(current)));
		}
		if (TYPE_STRING(GET_CDR(current))) {
			BITSET(cons_marks, STRING_HANDLER_ADDRESS(GET_CDR(current)));
		}
		if (TYPE_CONS(GET_CAR(current))) {
			BITSET(cons_marks, GET_CAR(current));
			marks_stack[++marks_stack_head] = GET_CAR(current);
		}
		if (TYPE_CONS(GET_CDR(current))) {
			BITSET(cons_marks, GET_CDR(current));
			marks_stack[++marks_stack_head] = GET_CDR(current);
		}
	}
}

void fill_root_set() {

	root_set[0] = nil;
	root_set[1] = t;
	root_set[2] = symbol_identifier;
	root_set[3] = lambda_identifier;
	root_set[4] = handler_identifier;
	root_set[5] = environment;
	root_set[6] = security_head;
	for (int i = 0; i < NATIVE_COUNT; i++) {
		root_set[i + 7] = native_identifiers[i];
	}
}

void mark_sweep() {

	fprintf(stderr, "\nGARBAGE COLLECTION INITIATED\n");
	fflush(stderr);

	SETBITS(cons_marks, MEM_SIZE);
	free_cons_stack_head = -1;
	fill_root_set();

	// mark
	for (int i = 0; i < ROOT_SET_SIZE; i++) {
		marks_stack_head = -1;
		rec_mark(root_set[i]);
	}

	// sweep
	memptr handlers_sort_buffer_filler = 0;
	for (int i = MEM_SIZE - 2; i >= 0; i--) {
		if (BITAT(cons_marks, i) != 1) {
			free_cons_stack[++free_cons_stack_head] = i;
		} else if (GET_CAR(i) == handler_identifier) {
			handlers_sort_buffer[handlers_sort_buffer_filler++] = i;
		}
	}

	// handlers sort
	memptr temp;
	for (int i = 0; i < handlers_sort_buffer_filler; i++) {
		for (int j = 0; j < handlers_sort_buffer_filler - i - 1; j++) {
			if (HANDLER_STRING_ADDRESS(
					GET_CDR(handlers_sort_buffer[j])) > HANDLER_STRING_ADDRESS(GET_CDR(handlers_sort_buffer[j + 1]))) {
				temp = handlers_sort_buffer[j];
				handlers_sort_buffer[j] = handlers_sort_buffer[j + 1];
				handlers_sort_buffer[j + 1] = temp;
			}
		}
	}

	// strings bottom append
	strings_filler = 0;
	for (int i = 0; i < handlers_sort_buffer_filler; i++) {

		memptr dst = strings_filler;
		memptr src = HANDLER_STRING_ADDRESS(GET_CDR(handlers_sort_buffer[i]));
		int len = 0;
		do {
			strings_pool[dst] = strings_pool[src];
			len++;
			dst++;
		} while (strings_pool[src++] != '\0');
		SET_CDR(handlers_sort_buffer[i], strings_filler);
		strings_filler += len;
	}
}
