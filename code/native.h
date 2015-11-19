#ifndef NATIVE_H_
#define NATIVE_H_

#include "memory.h"
#include <stdio.h>

// natives enum. 0 and 1 are nil and true
enum {
	FUNC_QUOTE = 2,
	FUNC_DEFINE,
	FUNC_LAMBDA,
	FUNC_IF,
	FUNC_ATOM,
	FUNC_CONS,
	FUNC_CAR,
	FUNC_CDR,
	FUNC_PLUS,
	FUNC_MINUS,
	FUNC_MULT,
	FUNC_DIV,
	FUNC_EQUALS,
	FUNC_SMALLER,
	FUNC_GREATER,
	FUNC_NIL
} Native;

// sign an integer value
int fix_integer(int integer_value);

/* native functions */
memptr func_quote(memptr expr);
memptr func_define(memptr expr);
memptr func_lambda(memptr expr);
memptr func_if(memptr expr);
memptr func_atom(memptr expr);
memptr func_cons(memptr expr);
memptr func_car(memptr expr);
memptr func_cdr(memptr expr);
memptr func_plus(memptr expr);
memptr func_minus(memptr expr);
memptr func_mult(memptr expr);
memptr func_div(memptr expr);
memptr func_equals(memptr expr);
memptr func_smaller(memptr expr);
memptr func_greater(memptr expr);
memptr func_nil(memptr expr);

#endif /* NATIVE_H_ */
