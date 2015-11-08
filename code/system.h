#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "memory.h"

#define NATIVE_COUNT 16
#define NATIVE_NAME_LIMIT 8
memptr native_identifiers[NATIVE_COUNT];
char native_names_buffer[NATIVE_NAME_LIMIT * NATIVE_COUNT];

typedef enum {
	ENVIRONMENT, DATA
} LookUp;

memptr environment;
memptr nil;
memptr t;
memptr security_head;
memptr security_tail;
memptr lambda_identifier;
memptr symbol_identifier;
memptr handler_identifier;

// builtin functions addresses container
memptr builtins[NATIVE_COUNT];

// initialize the system
void system_initialize();

// resolve an expression, returns evaluation result
memptr resolve_expression(memptr expr);

#endif /* SYSTEM_H_ */
