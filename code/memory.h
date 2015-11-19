#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdbool.h>
#include "bitarray.h"
#include "error.h"

// cons pool size
#define MEM_SIZE (1 << 14)
// chars pool size
#define STRINGS_POOL_SIZE (1 << 14)

// not found - error address
#define NOT_FOUND ((1<<14)-1)
#define IS_NOT_FOUND(value16) ((value16) == NOT_FOUND)

// string value interpretation
#define STRING_HANDLER_ADDRESS(string16) ((string16)&((1<<14)-1))
#define HANDLER_STRING_ADDRESS(handler16) ((handler16)&((1<<14)-1))

// type interpretation
#define TYPE_NIL(value16) ((value16) == 0)
#define TYPE_T(value16) ((value16) == 1)
#define TYPE_NATIVE(value16) ((value16) < (NATIVE_COUNT + 2) && (value16) > 1)
#define TYPE_CONS(value16) (((value16) > NATIVE_COUNT + 4) && (((value16) & (3<<14)) == 0))
#define TYPE_INTEGER(value16) (((value16) & (1<<15)) == (1<<15))
#define TYPE_STRING(value16) ((((value16) & (3<<14))>>14) == 1)

#define MAKE_INTEGER(value) (((value) & ((1<<15)-1)) | (1<<15))
#define INTEGER_VALUE(value16) ((value16) & ((1<<15)-1))

// car/cdr interpretation
#define GET_CAR(value16) ((cons_pool[value16] & (((1<<16)-1)<<16))>>16)
#define GET_CDR(value16) (cons_pool[value16] & ((1<<16)-1))
#define SET_CAR(at16, value16) do {cons_pool[at16] = ((cons_pool[at16] & ((1<<16)-1)) | ((value16)<<16));} while (0)
#define SET_CDR(at16, value16) do {cons_pool[at16] = ((cons_pool[at16] & (((1<<16)-1)<<16)) | (value16));} while (0)

// print error messege
#define MEMORY_ERROR	do {										\
							error = false;							\
							COMMIT_ERROR(ERROR_MEMORY, ERROR_OOM);	\
							print_result(0, 0);						\
							fprintf(stderr, "\n");					\
							fflush(stderr);							\
						} while (0)

// cons is 32 bits sized
typedef unsigned int cons;
// address is 16 bits sized
typedef unsigned short memptr;

cons cons_pool[MEM_SIZE];		// cons pool
memptr free_cons_stack[MEM_SIZE];	// cons pool free addresses stack
int free_cons_stack_head;		// stacks head
bit cons_marks[BITS(MEM_SIZE)];		// cons marks bit array, used in garbage collection

memptr handlers_sort_buffer[MEM_SIZE];	// sort buffer, used in garbage collection

char strings_pool[STRINGS_POOL_SIZE];	// chars pool
int strings_filler;			// chars pool filler. points first free record on the pool

// string buffer, from which strings allocation is made
#define STRING_BUFFER_SIZE 16
char string_buffer[STRING_BUFFER_SIZE];

// root set of mark n sweep
// TODO: insert specific NATIVE_COUNT
#define ROOT_SET_SIZE (7 + 16)
memptr root_set[ROOT_SET_SIZE];
// used in order to reduce recursion
memptr marks_stack[MEM_SIZE];
int marks_stack_head;

// moves string from buffer to pool
#define ALLOCATE_STRING_FROM_BUFFER(buff, i)	do {												\
													int filler = 0;									\
													while (buff[i + filler] != '\0') {				\
														string_buffer[filler] = buff[i + filler];	\
														filler++;									\
													}												\
													string_buffer[filler] = '\0';					\
												} while (0)

#endif /* MEMORY_H_ */
