#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdbool.h>
#include "bitarray.h"
#include "error.h"

#define MEM_SIZE (1 << 14)
#define STRINGS_POOL_SIZE (1 << 14)

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

#define MEMORY_ERROR	do {										\
							error = false;							\
							COMMIT_ERROR(ERROR_MEMORY, ERROR_OOM);	\
							print_result(0, 0);						\
							fprintf(stderr, "\n");					\
							fflush(stderr);							\
						} while (0)

typedef unsigned int cons;
typedef unsigned short memptr;

cons cons_pool[MEM_SIZE];
memptr free_cons_stack[MEM_SIZE];
int free_cons_stack_head;
bit cons_marks[BITS(MEM_SIZE)];

memptr handlers_sort_buffer[MEM_SIZE];

char strings_pool[STRINGS_POOL_SIZE];
int strings_filler;

#define STRING_BUFFER_SIZE 16
char string_buffer[STRING_BUFFER_SIZE];

// TODO: insert specific NATIVE_COUNT
#define ROOT_SET_SIZE (7 + 16)
memptr root_set[ROOT_SET_SIZE];
memptr marks_stack[MEM_SIZE];
int marks_stack_head;

#define ALLOCATE_STRING_FROM_BUFFER(buff, i)	do {												\
													int filler = 0;									\
													while (buff[i + filler] != '\0') {				\
														string_buffer[filler] = buff[i + filler];	\
														filler++;									\
													}												\
													string_buffer[filler] = '\0';					\
												} while (0)

#endif /* MEMORY_H_ */
