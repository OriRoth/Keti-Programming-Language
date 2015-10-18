#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "memmanage.h"
#include <string.h>

#define ERROR_MESSAGE_BUFFER_SIZE 64
#define HASH_FACTOR 8
#define BUILTIN_BUFFER_SIZE 100 // TODO: give exact size
#define SECURITY_INIT()	security_head = security_wait = nil
#define SECURE(address)	do {												\
							security_wait = address;						\
							memptr temp;									\
							if (security_head == nil) {						\
								security_head = new_data();					\
								MemoryPool[security_head].carKind = CONS;	\
								MemoryPool[security_head].car = address;	\
								MemoryPool[security_head].cdrKind = NIL;	\
							} else {										\
								temp = security_head;						\
								security_head = new_data();					\
								MemoryPool[security_head].carKind = CONS;	\
								MemoryPool[security_head].car = address;	\
								MemoryPool[security_head].cdrKind = CONS;	\
								MemoryPool[security_head].cdr = temp;		\
							}												\
							security_wait = nil;							\
						} while(0)
#define ERROR(msg)	do {													\
						error = true;										\
						strcpy(&error_message, msg);						\
						error_message[ERROR_MESSAGE_BUFFER_SIZE - 1] = 0;	\
						} while(0)

memptr environment;
memptr nil;
memptr t;
typedef memptr (*builtin)(memptr func_expr, memptr first_arg);
bool error;
char error_message[ERROR_MESSAGE_BUFFER_SIZE];
memptr security_head;
memptr security_wait;

// builtin functions container
builtin builtins[BUILTIN_BUFFER_SIZE];

// initialize the system
void system_initialize();

// prints the first m memory cells of the cons pool
void print_mem(int m);

memptr translate_func_expr(memptr func_expr);
memptr resolve_expr(memptr expr);

#endif /* SYSTEM_H_ */
