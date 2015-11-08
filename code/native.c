#include "native.h"
#include "memory.h"
#include "system.h"
#include "error.h"

int fix_integer(int integer_value) {

	int fix = integer_value;
	if ((fix & (1 << 14)) > 1) {
		fix |= (((1 << 18) - 1) << 14);
	}
	return fix;
}

memptr func_quote(memptr expr) {

	return GET_CAR(expr);
}

memptr func_define(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_DEFINE, ERROR_IPC);
		;
		return NOT_FOUND;
	}

	memptr symbol = safe_allocate_cons(), first_arg = GET_CAR(expr);
	SET_CAR(symbol, symbol_identifier);
	SET_CDR(symbol, allocate_cons());
	memptr symbol_value = GET_CDR(symbol);
	if (TYPE_CONS(first_arg) && GET_CAR(first_arg) == symbol_identifier) {
		SET_CAR(symbol_value, GET_CAR(GET_CDR(first_arg)));
	} else {
		first_arg = resolve_expression(first_arg);
		if (!IS_NOT_FOUND(first_arg) && TYPE_CONS(first_arg)
				&& GET_CAR(first_arg) == symbol_identifier) {
			SET_CAR(symbol_value, GET_CAR(GET_CDR(first_arg)));
		} else {
			COMMIT_ERROR(ERROR_DEFINE, ERROR_IPT);
			return NOT_FOUND;
		}
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg)) {
		return NOT_FOUND;
	}
	SET_CDR(symbol_value, second_arg);

	memptr env = lookup(symbol, ENVIRONMENT);
	error = false; // TODO: not the best way to do it, probably
	if (IS_NOT_FOUND(env)) {
		insert_symbol(environment, symbol);
	} else {
		remove_symbol(env, symbol);
		insert_symbol(env, symbol);
	}

	return second_arg;
}

memptr func_lambda(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_LAMBDA, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = GET_CAR(expr), second_arg = GET_CAR(GET_CDR(expr));

	// checking first argument is a list of parameters
	if (TYPE_CONS(first_arg)) {
		memptr current_param = first_arg;
		do {
			if (GET_CAR(GET_CAR(current_param)) != symbol_identifier) {
				COMMIT_ERROR(ERROR_LAMBDA, ERROR_IPT);
				return NOT_FOUND;
			}
			current_param = GET_CDR(current_param);
		} while (TYPE_CONS(current_param));
	} else if (!TYPE_NIL(first_arg)) {
		COMMIT_ERROR(ERROR_LAMBDA, ERROR_IPT);
		return NOT_FOUND;
	}

	memptr result = safe_allocate_cons();
	SET_CAR(result, lambda_identifier);
	SET_CDR(result, allocate_cons());
	memptr current_node = GET_CDR(result);
	SET_CAR(current_node, first_arg);
	SET_CDR(current_node, second_arg);
	return result;
}

memptr func_if(memptr expr) {

	if (num_nodes(expr) != 3) {
		COMMIT_ERROR(ERROR_IF, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = GET_CAR(expr), second_arg = GET_CAR(GET_CDR(expr)),
			third_arg = GET_CAR(GET_CDR(GET_CDR(expr)));

	first_arg = resolve_expression(first_arg);
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}

	if (TYPE_T(first_arg)) {
		return resolve_expression(second_arg);
	} else if (TYPE_NIL(first_arg)) {
		return resolve_expression(third_arg);
	}

	return NOT_FOUND;
}

memptr func_atom(memptr expr) {

	if (num_nodes(expr) != 1) {
		COMMIT_ERROR(ERROR_ATOM, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(arg)) {
		return NOT_FOUND;
	}

	// TODO: should native function and symbols be included at the atoms group?
	return ((TYPE_INTEGER(arg) || TYPE_NIL(arg) || TYPE_T(arg)
			|| TYPE_STRING(arg) || TYPE_NATIVE(arg)
			|| (arg == symbol_identifier) || (arg == lambda_identifier)) ?
			t : nil);
}

memptr func_cons(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_CONS, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr result = safe_allocate_cons();

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}
	SET_CAR(result, first_arg);

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg)) {
		return NOT_FOUND;
	}
	SET_CDR(result, second_arg);

	return result;
}

memptr func_car(memptr expr) {

	if (num_nodes(expr) != 1) {
		COMMIT_ERROR(ERROR_CAR, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(arg) || !TYPE_CONS(arg)) {
		COMMIT_ERROR(ERROR_CAR, ERROR_IPT);
		return NOT_FOUND;
	}

	return GET_CAR(arg);
}

memptr func_cdr(memptr expr) {

	if (num_nodes(expr) != 1) {
		COMMIT_ERROR(ERROR_CDR, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(arg) || !TYPE_CONS(arg)) {
		COMMIT_ERROR(ERROR_CAR, ERROR_IPT);
		return NOT_FOUND;
	}

	return GET_CDR(arg);
}

memptr func_plus(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_PLUS, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg) || !TYPE_INTEGER(first_arg)) {
		COMMIT_ERROR(ERROR_PLUS, ERROR_IPT);
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg) || !TYPE_INTEGER(second_arg)) {
		COMMIT_ERROR(ERROR_PLUS, ERROR_IPT);
		return NOT_FOUND;
	}

	return MAKE_INTEGER(INTEGER_VALUE(first_arg) + INTEGER_VALUE(second_arg));
}

memptr func_minus(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_MINUS, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg) || !TYPE_INTEGER(first_arg)) {
		COMMIT_ERROR(ERROR_MINUS, ERROR_IPT);
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg) || !TYPE_INTEGER(second_arg)) {
		COMMIT_ERROR(ERROR_MINUS, ERROR_IPT);
		return NOT_FOUND;
	}

	return MAKE_INTEGER(INTEGER_VALUE(first_arg) - INTEGER_VALUE(second_arg));
}

memptr func_mult(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_MULT, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg) || !TYPE_INTEGER(first_arg)) {
		COMMIT_ERROR(ERROR_MULT, ERROR_IPT);
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg) || !TYPE_INTEGER(second_arg)) {
		COMMIT_ERROR(ERROR_MULT, ERROR_IPT);
		return NOT_FOUND;
	}

	return MAKE_INTEGER(INTEGER_VALUE(first_arg) * INTEGER_VALUE(second_arg));
}

memptr func_div(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_DIV, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg) || !TYPE_INTEGER(first_arg)) {
		COMMIT_ERROR(ERROR_DIV, ERROR_IPT);
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg) || !TYPE_INTEGER(second_arg)
			|| INTEGER_VALUE(second_arg) == 0) {
		COMMIT_ERROR(ERROR_DIV, ERROR_IPT);
		return NOT_FOUND;
	}

	return MAKE_INTEGER(INTEGER_VALUE(first_arg) / INTEGER_VALUE(second_arg));
}

memptr func_equals(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_EQUALS, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg)) {
		return NOT_FOUND;
	}

	// TODO: add cases (strings, etc.)
	if (TYPE_CONS(first_arg) && TYPE_CONS(second_arg)
			&& GET_CAR(first_arg) == symbol_identifier
			&& GET_CAR(second_arg) == symbol_identifier) {
		// compares symbols by name
		first_arg = GET_CAR(GET_CDR(first_arg));
		second_arg = GET_CAR(GET_CDR(second_arg));
	}
	if (TYPE_INTEGER(first_arg) && TYPE_INTEGER(second_arg)) {
		return ((INTEGER_VALUE(first_arg) == INTEGER_VALUE(second_arg)) ?
				t : nil);
	} else if (TYPE_STRING(first_arg) && TYPE_STRING(second_arg)) {
		return string_compare(first_arg, second_arg);
	} else if ((first_arg == symbol_identifier
			&& second_arg == symbol_identifier)
			|| (first_arg == lambda_identifier
					&& second_arg == lambda_identifier)) {
		return true;
	} else {
		// by address
		return ((first_arg == second_arg) ? t : nil);
	}

	return NOT_FOUND;
}

memptr func_smaller(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_SMALLER, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg)) {
		return NOT_FOUND;
	}

	// TODO: add cases (strings, etc.)
	if (TYPE_INTEGER(first_arg) && TYPE_INTEGER(second_arg)) {
		return ((fix_integer(INTEGER_VALUE(first_arg))
				< fix_integer(INTEGER_VALUE(second_arg))) ? t : nil);
	} else {
		// by address
		return ((first_arg < second_arg) ? t : nil);
	}

	return NOT_FOUND;
}

memptr func_greater(memptr expr) {

	if (num_nodes(expr) != 2) {
		COMMIT_ERROR(ERROR_GREATER, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}

	memptr second_arg = resolve_expression(GET_CAR(GET_CDR(expr)));
	if (IS_NOT_FOUND(second_arg)) {
		return NOT_FOUND;
	}

	// TODO: add cases (strings, etc.)
	if (TYPE_INTEGER(first_arg) && TYPE_INTEGER(second_arg)) {
		return ((fix_integer(INTEGER_VALUE(first_arg))
				> fix_integer(INTEGER_VALUE(second_arg))) ? t : nil);
	} else {
		// by address
		return ((first_arg > second_arg) ? t : nil);
	}

	return NOT_FOUND;
}

memptr func_nil(memptr expr) {

	if (num_nodes(expr) != 1) {
		COMMIT_ERROR(ERROR_NIL, ERROR_IPC);
		return NOT_FOUND;
	}

	memptr first_arg = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(first_arg)) {
		return NOT_FOUND;
	}

	return (first_arg == nil ? t : nil);
}
