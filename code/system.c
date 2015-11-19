#include "system.h"
#include "memory.h"
#include "native.h"
#include "error.h"
#include <stdio.h>
#include <stdbool.h>

char native_names_buffer[NATIVE_NAME_LIMIT * NATIVE_COUNT] = { 'q', 'u', 'o',
		't', 'e', '\0', '\0', '\0', 'd', 'e', 'f', 'i', 'n', 'e', '\0', '\0',
		'l', 'a', 'm', 'b', 'd', 'a', '\0', '\0', 'i', 'f', '\0', '\0', '\0',
		'\0', '\0', '\0', 'a', 't', 'o', 'm', '\0', '\0', '\0', '\0', 'c', 'o',
		'n', 's', '\0', '\0', '\0', '\0', 'c', 'a', 'r', '\0', '\0', '\0', '\0',
		'\0', 'c', 'd', 'r', '\0', '\0', '\0', '\0', '\0', '+', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '-', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '*', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '/', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '=', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '<', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '>', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', 'n', 'i', 'l', '\0', '\0', '\0', '\0',
		'\0' };

memptr resolve_expression(memptr expr);
memptr lookup(memptr expr, LookUp lu);

/*
 * initialize security head, while disconnecting the previous secured addresses from the root set
*/
void security_init() {
	security_head = nil;
}

/*
 * a safe allocation - connecting the address to the security list, so it will
 * be marked if garbage collection will be called
*/
memptr safe_allocate_cons() {

	announce_allocation(2);
	if (security_head != nil) {
		SET_CDR(security_tail, allocate_cons());
		security_tail = GET_CDR(security_tail);
		SET_CAR(security_tail, allocate_cons());
	} else {
		security_head = allocate_cons();
		SET_CAR(security_head, allocate_cons());
		security_tail = security_head;
	}

	return GET_CAR(security_tail);
}

/*
 * returns the number of nodes in a list
*/
int num_nodes(memptr list) {

	if (list == nil) {
		return 0;
	}

	int counter = 1;
	while (TYPE_CONS(GET_CDR(list))) {
		counter++;
		list = GET_CDR(list);
	}
	return counter;
}

/*
 * receives two string values, and compares them
*/
bool string_compare(memptr str1, memptr str2) {

	int iter1 = HANDLER_STRING_ADDRESS(GET_CDR(STRING_HANDLER_ADDRESS(str1)));
	int iter2 = HANDLER_STRING_ADDRESS(GET_CDR(STRING_HANDLER_ADDRESS(str2)));

	while (strings_pool[iter1] != '\0' && strings_pool[iter2] != '\0'
			&& strings_pool[iter1] == strings_pool[iter2]) {
		iter1++;
		iter2++;
	}
	return strings_pool[iter1] == strings_pool[iter2];
}
/*
 * inserts a data into a list (front insert)
*/
memptr list_insert(memptr list_head, memptr data) {

	memptr new_node = allocate_cons();
	memptr old_node = GET_CAR(list_head);
	SET_CAR(list_head, new_node);
	if (TYPE_CONS(old_node)) {
		SET_CDR(new_node, old_node);
	}
	SET_CAR(new_node, data);
	return new_node;
}

/*
 * insert symbol into an environment
*/
void insert_symbol(memptr env, memptr data) {

	list_insert(env, data);
}

/*
 * removes symbol from an environment, returns true if done successfuly
*/
bool remove_symbol(memptr env, memptr data) {

	memptr next_node, current_node, previous_node, current_data;

	previous_node = env;
	next_node = GET_CAR(env);
	if (TYPE_NIL(next_node)) {
		// symbols list is empty
		return false;
	}

	do {
		current_node = next_node;
		next_node = GET_CDR(current_node);
		current_data = GET_CAR(current_node);
		if (string_compare(GET_CAR(GET_CDR(current_data)), GET_CAR(data))) {
			if (previous_node == env) {
				SET_CAR(previous_node, next_node);
			} else {
				SET_CDR(previous_node, next_node);
			}
			return true;
		}
		previous_node = current_node;
	} while (TYPE_CONS(next_node));

	return false;
}

/*
 * inserts native function into system
*/
#define INSERT_BUILTIN_SETUP(i)	do {																			\
									ALLOCATE_STRING_FROM_BUFFER(native_names_buffer, i * NATIVE_NAME_LIMIT);	\
									insert_builtin(i);															\
								} while (0)

/*
 * commits the insertion. we assume all allocations would be successful
*/
void insert_builtin(int index) {

	memptr data = allocate_cons();
	SET_CAR(data, symbol_identifier);
	SET_CDR(data, allocate_cons());
	memptr data_value = GET_CDR(data);
	SET_CAR(data_value, allocate_string_from_string_buffer());
	SET_CDR(data_value, index + 2);
	list_insert(environment, data);
}

/*
 * initialize system. allocate all the identifiers, such as nil, true, natives...
*/
void system_initialize() {

	initialize_memory_system();

	// allocation nil
	nil = allocate_cons();	// 0
	// allocating t
	t = allocate_cons();	// 1

	// allocating natives identifiers
	for (int i = 0; i < NATIVE_COUNT; i++) {
		native_identifiers[i] = allocate_cons();	// i + 2
	}

	// allocating identifiers
	lambda_identifier = allocate_cons();	// 2 + NATIVE_COUNT
	symbol_identifier = allocate_cons();	// 3 + NATIVE_COUNT
	handler_identifier = allocate_cons();	// 4 + NATIVE_COUNT

	// allocating base environment
	environment = allocate_cons();	// 5 + NATIVE_COUNT

	// define natives in base environment
	for (int i = 0; i < NATIVE_COUNT; i++) {
		INSERT_BUILTIN_SETUP(i);
	}
}

/*
 * return the value of the symbol named name. can return the symbols environment or value
*/
memptr lookup(memptr name, LookUp lu) {

	memptr next_env = environment, current_env, next_node, current_node,
			current_data;
	do {
		current_env = next_env;
		if (TYPE_NIL(GET_CAR(current_env))) {
			// symbols list is empty
			next_env = GET_CDR(current_env);
			continue;
		}

		next_node = GET_CAR(current_env);
		while (TYPE_CONS(next_node)) {
			current_node = next_node;
			current_data = GET_CAR(current_node);
			if (string_compare(GET_CAR(GET_CDR(current_data)), name)) {
				switch (lu) {
				case ENVIRONMENT:
					return current_env;
				case DATA:
					return GET_CDR(GET_CDR(current_data));
				default:
					return NOT_FOUND;
				}
			}
			next_node = GET_CDR(next_node);
		}

		next_env = GET_CDR(current_env);
	} while (TYPE_CONS(next_env));

	COMMIT_ERROR(ERROR_SYSTEM, ERROR_SU);
	return NOT_FOUND;
}

/*
 * receives a parameter list and values, and creates a new environment by them
*/
static bool create_environment(memptr values_list, memptr parameters_list) {

	int values_size = num_nodes(values_list), parameters_size = num_nodes(
			parameters_list);
	if (values_size != parameters_size) {
		COMMIT_ERROR(ERROR_SYSTEM, ERROR_IPC);
		return false;
	}

	if (values_size == 0) {
		memptr temp = environment;
		environment = allocate_cons();
		SET_CDR(environment, temp);
		return true;
	}

	memptr current_value_node, next_value_node = values_list, current_value,
			current_param_node, next_param_node = parameters_list,
			current_param, current_value_resolved, current_param_dup;

	memptr dup_list = safe_allocate_cons(), dup_list_end = NOT_FOUND;
	do {
		current_value_node = next_value_node;
		current_param_node = next_param_node;
		current_value = GET_CAR(current_value_node);
		current_param = GET_CAR(current_param_node);

		current_value_resolved = resolve_expression(current_value);
		if (IS_NOT_FOUND(current_value_resolved)) {
			return false;
		}

		announce_allocation(3);
		current_param_dup = allocate_cons();
		SET_CAR(current_param_dup, symbol_identifier);
		SET_CDR(current_param_dup, allocate_cons());
		memptr current_param_dup_value = GET_CDR(current_param_dup);
		SET_CAR(current_param_dup_value, GET_CAR(GET_CDR(current_param)));
		SET_CDR(current_param_dup_value, current_value_resolved);
		if (IS_NOT_FOUND(dup_list_end)) {
			SET_CAR(dup_list, current_param_dup);
			dup_list_end = dup_list;
		} else {
			SET_CDR(dup_list_end, allocate_cons());
			dup_list_end = GET_CDR(dup_list_end);
			SET_CAR(dup_list_end, current_param_dup);
		}

		next_value_node = GET_CDR(current_value_node);
		next_param_node = GET_CDR(current_param_node);
	} while (TYPE_CONS(GET_CDR(current_value_node)));

	memptr temp = environment;
	environment = allocate_cons();
	SET_CDR(environment, temp);
	SET_CAR(environment, dup_list);

	return true;
}

/*
 * destroy current environment
*/
static void destroy_environment() {

	// we assume the base environment is not being destroyed
	environment = GET_CDR(environment);
}

/*
 * main resolvement logic. receives an expression address and returns its evaluation result
 * if its an atomic value, return it
 * if its a call to a native function, commit it
 * if its a call to a user function (lambda expression), create its environment and evaluate its return value
*/
memptr resolve_expression(memptr expr) {

	if (IS_NOT_FOUND(expr)) {
		return NOT_FOUND;
	}

	if (TYPE_NIL(
			expr) || TYPE_T(expr) || TYPE_INTEGER(expr) || TYPE_STRING(expr) || TYPE_NATIVE(expr)) {
		return expr;
	}

	if (TYPE_CONS(expr) && (GET_CAR(expr) == lambda_identifier)) {
		return expr;
	}

	if (TYPE_CONS(expr) && (GET_CAR(expr) == symbol_identifier)) {
		return lookup(GET_CAR(GET_CDR(expr)), DATA);
	}

	memptr func_body = resolve_expression(GET_CAR(expr));
	if (IS_NOT_FOUND(func_body)) {
		return NOT_FOUND;
	}

	memptr result;
	if (TYPE_NATIVE(func_body)) {
		// builtin function
		switch (func_body) {
		case FUNC_CAR:
			result = func_car(GET_CDR(expr));
			break;
		case FUNC_CDR:
			result = func_cdr(GET_CDR(expr));
			break;
		case FUNC_CONS:
			result = func_cons(GET_CDR(expr));
			break;
		case FUNC_DEFINE:
			result = func_define(GET_CDR(expr));
			break;
		case FUNC_DIV:
			result = func_div(GET_CDR(expr));
			break;
		case FUNC_EQUALS:
			result = func_equals(GET_CDR(expr));
			break;
		case FUNC_IF:
			result = func_if(GET_CDR(expr));
			break;
		case FUNC_ATOM:
			result = func_atom(GET_CDR(expr));
			break;
		case FUNC_GREATER:
			result = func_greater(GET_CDR(expr));
			break;
		case FUNC_LAMBDA:
			result = func_lambda(GET_CDR(expr));
			break;
		case FUNC_MINUS:
			result = func_minus(GET_CDR(expr));
			break;
		case FUNC_MULT:
			result = func_mult(GET_CDR(expr));
			break;
		case FUNC_NIL:
			result = func_nil(GET_CDR(expr));
			break;
		case FUNC_PLUS:
			result = func_plus(GET_CDR(expr));
			break;
		case FUNC_QUOTE:
			result = func_quote(GET_CDR(expr));
			break;
		case FUNC_SMALLER:
			result = func_smaller(GET_CDR(expr));
			break;
		default:
			result = NOT_FOUND;
			break;
		}
	} else if (TYPE_CONS(func_body)) {
		if (GET_CAR(func_body) == lambda_identifier) {
			// interpreted as user made function
			func_body = GET_CDR(func_body);
			bool create_environment_success = create_environment(GET_CDR(expr),
					GET_CAR(func_body));
			if (!create_environment_success) {
				result = NOT_FOUND;
			} else {
				result = resolve_expression(GET_CDR(func_body));
				destroy_environment();
			}
		} else {
			COMMIT_ERROR(ERROR_SYSTEM, ERROR_EM);
			return NOT_FOUND;
		}
	} else {
		COMMIT_ERROR(ERROR_SYSTEM, ERROR_EM);
		return NOT_FOUND;
	}

	return result;
}
