#include "system.h"
#include <stdio.h>

typedef enum {
	ENVIRONMENT, DATA
} LookUp;
memptr resolve_func_expr(memptr func_expr);
static void hash(memptr env, memptr data);
static bool unhash(memptr env, memptr data);
static memptr lookup(memptr expr, LookUp lu);

memptr seek_field(memptr object, memptr field_name) {

	memptr current_node = MemoryPool[object].car;
	memptr current_param = MemoryPool[current_node].car;
	if (strcmp(get_string(MemoryPool[current_param].car),
			get_string(MemoryPool[field_name].car)) == 0) {
		return MemoryPool[current_param].cdr;
	}

	while (MemoryPool[current_node].cdrKind == CONS) {
		current_node = MemoryPool[current_node].cdr;
		current_param = MemoryPool[current_node].car;
		if (strcmp(get_string(MemoryPool[current_param].car),
				get_string(MemoryPool[field_name].car)) == 0) {
			return MemoryPool[current_param].cdr;
		}
	}

	return NOT_FOUND;
}

#define FORBIDDEN_SIZE 4
memptr forbidden_array[FORBIDDEN_SIZE];
bool initial_analyze_forbidden(memptr inspect) {

	for (int i = 0; i < FORBIDDEN_SIZE; i++) {
		int x = forbidden_array[i];
		if (inspect == forbidden_array[i]) {
			return true;
		}
	}

	return false;
}

/*
 * builtin functions
 */

memptr safe_new_data() {

	memptr alloc = new_data();
	SECURE(alloc);
	return alloc;
}

// returns the number of parameters in the expression, including the function name
static int num_nodes(memptr func_expr) {

	if (func_expr == nil) {
		return 0;
	}

	int counter = 1;
	while (MemoryPool[func_expr].cdrKind == CONS) {
		counter++;
		func_expr = MemoryPool[func_expr].cdr;
	}
	return counter;
}

typedef enum {
	PLUS, MINUS, MULT, DIV
} Arithmetic;

static memptr do_arithmetics(memptr func_expr, memptr first_arg,
		Arithmetic type) {

	if (num_nodes(func_expr) < 3) {
		switch (type) {
		case PLUS:
			ERROR("'+' : bad parameters count\n");
			break;
		case MINUS:
			ERROR("'-' : bad parameters count\n");
			break;
		case MULT:
			ERROR("'*' : bad parameters count\n");
			break;
		case DIV:
			ERROR("'/' : bad parameters count\n");
			break;
		}
		return NOT_FOUND;
	}

	memptr current_node, current_param;
	int res, tail = (type == PLUS || type == MINUS) ? 0 : 1;

	current_node = MemoryPool[func_expr].cdr;
	current_param = first_arg;
	if (current_param == NOT_FOUND) {
		return NOT_FOUND;
	}
	if (MemoryPool[current_param].carKind != INTEGER
			|| MemoryPool[current_param].cdrKind != NIL) {
		switch (type) {
		case PLUS:
			ERROR("'+' : bad parameter\n");
			break;
		case MINUS:
			ERROR("'-' : bad parameter\n");
			break;
		case MULT:
			ERROR("'*' : bad parameter\n");
			break;
		case DIV:
			ERROR("'/' : bad parameter\n");
			break;
		}
		return NOT_FOUND;
	}
	res = MemoryPool[current_param].car;

	while (MemoryPool[current_node].cdrKind == CONS) {
		current_node = MemoryPool[current_node].cdr;
		current_param = resolve_expr(MemoryPool[current_node].car);
		if (current_param == NOT_FOUND) {
			return NOT_FOUND;
		}
		if (MemoryPool[current_param].carKind != INTEGER
				|| MemoryPool[current_param].cdrKind != NIL) {
			switch (type) {
			case PLUS:
				ERROR("'+' : bad parameter\n");
				break;
			case MINUS:
				ERROR("'-' : bad parameter\n");
				break;
			case MULT:
				ERROR("'*' : bad parameter\n");
				break;
			case DIV:
				ERROR("'/' : bad parameter\n");
				break;
			}
			return NOT_FOUND;
		}
		if (type == PLUS || type == MINUS) {
			tail += MemoryPool[current_param].car;
		} else if (type == MULT || type == DIV) {
			tail *= MemoryPool[current_param].car;
		}
	}
	if (type == PLUS) {
		res += tail;
	} else if (type == MINUS) {
		res -= tail;
	} else if (type == MULT) {
		res *= tail;
	} else if (type == DIV) {
		if (tail == 0) {
			ERROR("'/' : zero division\n");
			return NOT_FOUND;
		}
		res /= tail;
	}

	memptr res_cons = safe_new_data();
	MemoryPool[res_cons].carKind = INTEGER;
	MemoryPool[res_cons].car = res;
	MemoryPool[res_cons].cdrKind = NIL;
	return res_cons;
}

// + function for 2+ integer parameters
static memptr func_plus(memptr func_expr, memptr first_arg) {

	return do_arithmetics(func_expr, first_arg, PLUS);
}

// - function for 2+ integer parameters
static memptr func_minus(memptr func_expr, memptr first_arg) {

	return do_arithmetics(func_expr, first_arg, MINUS);
}

// * function for 2+ integer parameters
static memptr func_mult(memptr func_expr, memptr first_arg) {

	return do_arithmetics(func_expr, first_arg, MULT);
}

// / function for 2+ integer parameters
static memptr func_div(memptr func_expr, memptr first_arg) {

	return do_arithmetics(func_expr, first_arg, DIV);
}

typedef enum {
	EQUAL, SMALLER, GREATER
} Comparision;

static memptr do_comparision(memptr func_expr, memptr first_arg,
		Comparision type) {
	// TODO: function comparison?
	// TODO: cons equalization (to do it? should it be recursive?)

	if (num_nodes(func_expr) < 3) {
		switch (type) {
		case EQUAL:
			ERROR("'=' : bad parameters count\n");
			break;
		case SMALLER:
			ERROR("'<' : bad parameters count\n");
			break;
		case GREATER:
			ERROR("'>' : bad parameters count\n");
			break;
		}
		return NOT_FOUND;
	}

	memptr current_node, current_param;
	int value;
	char *str_address;
	bool eq = true;

	current_node = MemoryPool[func_expr].cdr;
	current_param = first_arg;
	if (current_param == NOT_FOUND) {
		return NOT_FOUND;
	}
	if (current_param == nil || current_param == t) {
		value = current_param;
		while (MemoryPool[current_node].cdrKind == CONS) {
			current_node = MemoryPool[current_node].cdr;
			current_param = resolve_expr(MemoryPool[current_node].car);
			if (current_param == NOT_FOUND) {
				return NOT_FOUND;
			}
			if (type == EQUAL || type == SMALLER || type == GREATER) {
				if (current_param != nil && current_param != t) {
					switch (type) {
					case EQUAL:
						ERROR("'=' : bad parameter\n");
						break;
					case SMALLER:
						ERROR("'<' : bad parameter\n");
						break;
					case GREATER:
						ERROR("'>' : bad parameter\n");
						break;
					}
					return NOT_FOUND;
				}
			}
		}
		if (type == SMALLER || type == GREATER) {
			eq = false;
		}
	} else if (MemoryPool[current_param].carKind == INTEGER
			&& MemoryPool[current_param].cdrKind == NIL) {
		value = MemoryPool[current_param].car;
		while (MemoryPool[current_node].cdrKind == CONS) {
			current_node = MemoryPool[current_node].cdr;
			current_param = resolve_expr(MemoryPool[current_node].car);
			if (current_param == NOT_FOUND) {
				return NOT_FOUND;
			}
			if (MemoryPool[current_param].carKind != INTEGER
					|| MemoryPool[current_param].cdrKind != NIL) {
				switch (type) {
				case EQUAL:
					ERROR("'=' : bad parameter\n");
					break;
				case SMALLER:
					ERROR("'<' : bad parameter\n");
					break;
				case GREATER:
					ERROR("'>' : bad parameter\n");
					break;
				}
				return NOT_FOUND;
			}
			if (type == EQUAL) {
				if (MemoryPool[current_param].car != value) {
					eq = false;
				}
			} else if (type == SMALLER) {
				if (MemoryPool[current_param].car <= value) {
					eq = false;
				}
			} else if (type == GREATER) {
				if (MemoryPool[current_param].car >= value) {
					eq = false;
				}
			}
			value = MemoryPool[current_param].car;
		}
	} else if (MemoryPool[current_param].carKind == STRING
			&& MemoryPool[current_param].cdrKind == NIL) {
		str_address = get_string(MemoryPool[current_param].car);
		while (MemoryPool[current_node].cdrKind == CONS) {
			current_node = MemoryPool[current_node].cdr;
			current_param = resolve_expr(MemoryPool[current_node].car);
			if (current_param == NOT_FOUND) {
				return NOT_FOUND;
			}
			if (MemoryPool[current_param].carKind != STRING
					|| MemoryPool[current_param].cdrKind != NIL) {
				switch (type) {
				case EQUAL:
					ERROR("'=' : bad parameter\n");
					break;
				case SMALLER:
					ERROR("'<' : bad parameter\n");
					break;
				case GREATER:
					ERROR("'>' : bad parameter\n");
					break;
				}
				return NOT_FOUND;
			}
			if (type == EQUAL) {
				if (strcmp(str_address,
						get_string(MemoryPool[current_param].car)) != 0) {
					eq = false;
				}
			} else if (type == SMALLER) {
				if (strcmp(str_address,
						get_string(MemoryPool[current_param].car)) >= 0) {
					eq = false;
				}
			} else if (type == GREATER) {
				if (strcmp(str_address,
						get_string(MemoryPool[current_param].car)) <= 0) {
					eq = false;
				}
			}
			str_address = get_string(MemoryPool[current_param].car);
		}
	} else {
		// equalization by address
		if (type != EQUAL) {
			return NOT_FOUND;
		}
		value = current_param;
		while (MemoryPool[current_node].cdrKind == CONS) {
			current_node = MemoryPool[current_node].cdr;
			current_param = resolve_expr(MemoryPool[current_node].car);
			if (current_param == NOT_FOUND) {
				return NOT_FOUND;
			}
			if (current_param != value) {
				eq = false;
				break;
			}
		}
	}

	return eq ? t : nil;
}

// == function for 2+ parameters
static memptr func_equals(memptr func_expr, memptr first_arg) {

	return do_comparision(func_expr, first_arg, EQUAL);
}

// < function for 2+ parameters
static memptr func_smaller(memptr func_expr, memptr first_arg) {

	return do_comparision(func_expr, first_arg, SMALLER);
}

// > function for 2+ parameters
static memptr func_greater(memptr func_expr, memptr first_arg) {

	return do_comparision(func_expr, first_arg, GREATER);
}

static memptr func_cons(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 3) {
		ERROR("cons : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr res = safe_new_data();
	MemoryPool[res].carKind = CONS;
	MemoryPool[res].cdrKind = CONS;

	memptr current_node, current_data;
// first parameter
	current_node = MemoryPool[func_expr].cdr;
	current_data = first_arg;
	if (current_data == NOT_FOUND) {
		return NOT_FOUND;
	}
	MemoryPool[res].car = current_data;

// second parameter
	current_node = MemoryPool[current_node].cdr;
	current_data = resolve_expr(MemoryPool[current_node].car);
	if (current_data == NOT_FOUND) {
		return NOT_FOUND;
	}
	MemoryPool[res].cdr = current_data;

	return res;
}

static memptr func_car(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 2) {
		ERROR("car : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr param = first_arg;
	if (param == NOT_FOUND) {
		return NOT_FOUND;
	}

	if (MemoryPool[param].carKind != CONS
			|| MemoryPool[param].cdrKind != CONS) {
		ERROR("car : bad parameter\n");
		return NOT_FOUND;
	}

	return MemoryPool[param].car;
}

static memptr func_cdr(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 2) {
		ERROR("cdr : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr param = first_arg;
	if (param == NOT_FOUND) {
		return NOT_FOUND;
	}

	if (MemoryPool[param].carKind != CONS
			|| MemoryPool[param].cdrKind != CONS) {
		ERROR("cdr : bad parameter\n");
		return NOT_FOUND;
	}

	return MemoryPool[param].cdr;
}

static memptr func_nil(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 2) {
		ERROR("nil : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr param = first_arg;
	if (param == NOT_FOUND) {
		return NOT_FOUND;
	}

	return param == nil;
}

static memptr func_true(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 2) {
		ERROR("true : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr param = first_arg;
	if (param == NOT_FOUND) {
		return NOT_FOUND;
	}

	return param == t;
}

static memptr func_cond(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) == 1) {
		// no parameters
		ERROR("cond : bad parameters count\n");
		return NOT_FOUND; // TODO: should it be nil?
	}

	memptr current_node, current_data, condition, result;
	current_node = func_expr;
	while (MemoryPool[current_node].cdrKind == CONS) {

		current_node = MemoryPool[current_node].cdr;
		current_data = MemoryPool[current_node].car;

		if (MemoryPool[current_data].carKind == CONS
				&& MemoryPool[current_data].cdrKind == CONS) {
			// cons support
			condition = MemoryPool[current_data].car;
			result = MemoryPool[current_data].cdr;
		} else if (MemoryPool[current_data].carKind == CONS
				&& MemoryPool[current_data].cdrKind == INTEGER) {
			// "function" support (interpreted as a function by the translator)
			current_data = MemoryPool[current_data].car;
			if (num_nodes(current_data) != 2) {
				ERROR("cond : malformed\n");
				return NOT_FOUND;
			}
			condition = MemoryPool[current_data].car;
			result = MemoryPool[MemoryPool[current_data].cdr].car;
		} else {
			// invalid
			ERROR("cond : malformed\n");
			return NOT_FOUND;
		}

		memptr condition_result = resolve_expr(condition);
		if (condition_result == NOT_FOUND) {
			return NOT_FOUND;
		} else if (condition_result != nil && condition_result != t) {
			ERROR("cond : malformed\n");
			return NOT_FOUND;
		}
		if (condition_result == t) {
			return resolve_expr(result);
		}
	}
	ERROR("cond : unresolved\n");
	return NOT_FOUND; // TODO: as above
}

static memptr func_setq(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 3) {
		// invalid call
		ERROR("setq : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr current_node = MemoryPool[func_expr].cdr;
	if (MemoryPool[MemoryPool[current_node].car].carKind != STRING
			|| MemoryPool[MemoryPool[current_node].car].cdrKind != CONS) {
		// first argument not a name
		ERROR("setq : first parameter should be a name\n");
		return NOT_FOUND;
	}
	memptr symbol = safe_new_data();
	MemoryPool[symbol].car = connect_string(
			get_string(MemoryPool[MemoryPool[current_node].car].car), symbol);
	MemoryPool[symbol].carKind = STRING;
	MemoryPool[symbol].cdrKind = NIL; // temporary
	current_node = MemoryPool[current_node].cdr;
	memptr value = MemoryPool[current_node].car;
	value = resolve_expr(value);

	if (value == NOT_FOUND) {
		// invalid value
		return NOT_FOUND;
	}

// defining the new symbol
	bool is_in_current_env = unhash(environment, symbol);
	if (is_in_current_env) {
		// in current environment
		hash(environment, symbol);
	} else {
		memptr env = lookup(symbol, ENVIRONMENT);
		if (env == NOT_FOUND) {
			// not in system
			hash(environment, symbol);
		} else {
			// in env
			hash(env, symbol);
		}
	}

	MemoryPool[symbol].cdrKind = CONS;
	MemoryPool[symbol].cdr = value;
	return value;
}

static memptr func_defun(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 4) {
		ERROR("defun : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr name = MemoryPool[MemoryPool[func_expr].cdr].car;
	if (MemoryPool[name].carKind != STRING
			|| MemoryPool[name].cdrKind != CONS) {
		// first argument not a name
		ERROR("defun : first argument should be a name\n");
		return NOT_FOUND;
	}

	memptr parameters =
			MemoryPool[MemoryPool[MemoryPool[func_expr].cdr].cdr].car;
	if (parameters != nil) {
		if (MemoryPool[parameters].carKind != CONS
				|| MemoryPool[parameters].cdrKind != INTEGER) {
			// second argument not a parameters list
			ERROR("defun : second argument should be a parameters list\n");
			return NOT_FOUND;
		}

		memptr current_param_node, next_param_node = MemoryPool[parameters].car,
				current_param;
		// TODO: is nil the right representation of "()"?
		if (next_param_node != nil) {
			// function has parameter(s)
			do {
				current_param_node = next_param_node;
				current_param = MemoryPool[current_param_node].car;
				if (MemoryPool[current_param].carKind != STRING
						|| MemoryPool[current_param].cdrKind != CONS) {
					// a parameter is not a name
					ERROR("defun : parameter should be a name\n");
					return NOT_FOUND;
				}
				next_param_node = MemoryPool[next_param_node].cdr;
			} while (MemoryPool[current_param_node].cdrKind == CONS);
		}
		// TODO: add more checks?
	}

// now we can build the function representation in the memory pool,
// and connect the functions name to it
	memptr function = safe_new_data(), current;
	MemoryPool[function].car = connect_string(get_string(MemoryPool[name].car),
			function);
	MemoryPool[function].carKind = STRING;
	MemoryPool[function].cdr = new_data();
	MemoryPool[function].cdrKind = CONS;

// the name value - a user function
	current = MemoryPool[function].cdr;
	MemoryPool[current].car = safe_new_data();
	MemoryPool[current].carKind = CONS;
	MemoryPool[current].cdrKind = INTEGER;

// first argument - parameters list
	current = MemoryPool[current].car;
	MemoryPool[current].car = (
			parameters == nil ? nil : MemoryPool[parameters].car);
	MemoryPool[current].cdr = safe_new_data();
	MemoryPool[current].carKind = CONS;
	MemoryPool[current].cdrKind = CONS;

// second argument - function body
	current = MemoryPool[current].cdr;
	MemoryPool[current].carKind = CONS;
	MemoryPool[current].cdrKind = NIL;
	MemoryPool[current].car =
			MemoryPool[MemoryPool[MemoryPool[MemoryPool[func_expr].cdr].cdr].cdr].car;

// hashing the function into the system
	bool is_in_current_env = unhash(environment, function);
	if (is_in_current_env) {
		// in current environment
		hash(environment, function);
	} else {
		memptr env = lookup(function, ENVIRONMENT);
		if (env == NOT_FOUND) {
			// not in system
			hash(environment, function);
		} else {
			// in env
			hash(env, function);
		}
	}

	return MemoryPool[function].cdr;
}

typedef enum {
	OR, AND, XOR
} BooleanOperation;
static memptr do_boolean(memptr func_expr, memptr first_arg,
		BooleanOperation type) {

	if (num_nodes(func_expr) < 3) {
		switch (type) {
		case OR:
			ERROR("'or' : bad parameters count\n");
			break;
		case AND:
			ERROR("'and' : bad parameters count\n");
			break;
		case XOR:
			ERROR("'xor' : bad parameters count\n");
			break;
		}
		return NOT_FOUND;
	}

	memptr current_node, current_param;
	bool res;

	current_node = MemoryPool[func_expr].cdr;
	current_param = first_arg;
	if (current_param == NOT_FOUND) {
		return NOT_FOUND;
	}
	if (current_param != nil && current_param != t) {
		switch (type) {
		case OR:
			ERROR("'or' : bad parameter\n");
			break;
		case AND:
			ERROR("'and' : bad parameter\n");
			break;
		case XOR:
			ERROR("'xor' : bad parameter\n");
			break;
		}
		return NOT_FOUND;
	}
	res = (current_param == t);

	while (MemoryPool[current_node].cdrKind == CONS) {

		// short circuit
		if ((type == OR && res == true) || (type == AND && res == false)) {
			break;
		}

		current_node = MemoryPool[current_node].cdr;
		current_param = resolve_expr(MemoryPool[current_node].car);
		if (current_param == NOT_FOUND) {
			return NOT_FOUND;
		}
		if (current_param != nil && current_param != t) {
			switch (type) {
			case OR:
				ERROR("'or' : bad parameter\n");
				break;
			case AND:
				ERROR("'and' : bad parameter\n");
				break;
			case XOR:
				ERROR("'xor' : bad parameter\n");
				break;
			}
			return NOT_FOUND;
		}
		switch (type) {
		case OR:
			res |= (current_param == t);
			break;
		case AND:
			res &= (current_param == t);
			break;
		case XOR:
			res ^= (current_param == t);
			break;
		}
	}

	return res;
}

memptr func_or(memptr func_expr, memptr first_arg) {
	return do_boolean(func_expr, first_arg, OR);
}

memptr func_and(memptr func_expr, memptr first_arg) {
	return do_boolean(func_expr, first_arg, AND);
}

memptr func_xor(memptr func_expr, memptr first_arg) {
	return do_boolean(func_expr, first_arg, XOR);
}

static memptr func_list(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) == 1) {
		return nil;
	}

	memptr current_node, current_param, current_dup_node = safe_new_data(),
			base = current_dup_node;

	current_node = MemoryPool[func_expr].cdr;
	current_param = first_arg;
	if (current_param == NOT_FOUND) {
		return NOT_FOUND;
	}
	MemoryPool[current_dup_node].car = current_param;
	MemoryPool[current_dup_node].carKind = CONS;

	while (MemoryPool[current_node].cdrKind == CONS) {

		current_node = MemoryPool[current_node].cdr;
		MemoryPool[current_dup_node].cdr = new_data();
		MemoryPool[current_dup_node].cdrKind = CONS;
		current_dup_node = MemoryPool[current_dup_node].cdr;

		current_param = resolve_expr(MemoryPool[current_node].car);
		if (current_param == NOT_FOUND) {
			return NOT_FOUND;
		}
		MemoryPool[current_dup_node].car = current_param;
		MemoryPool[current_dup_node].carKind = CONS;
	}

	MemoryPool[current_dup_node].cdrKind = CONS;
	MemoryPool[current_dup_node].cdr = nil;

	return base;
}

static memptr func_value(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) < 2) {
		ERROR("'value' : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr current_node = MemoryPool[func_expr].cdr, current_value = first_arg;
	while (MemoryPool[current_node].cdrKind == CONS) {
		current_node = MemoryPool[current_node].cdr;
		current_value = resolve_expr(MemoryPool[current_node].car);
		if (current_value == NOT_FOUND) {
			return NOT_FOUND;
		}
	}

	return current_value;
}

static memptr func_assign(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 4) {
		ERROR("'assign' : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr current_node = MemoryPool[func_expr].cdr;
	memptr field_name = MemoryPool[current_node].car;
	if (MemoryPool[field_name].carKind != STRING
			|| MemoryPool[field_name].cdrKind != CONS) {
		ERROR("'assign' : first argument should be a field name\n");
		return NOT_FOUND;
	}

	current_node = MemoryPool[current_node].cdr;
	memptr field_value = resolve_expr(MemoryPool[current_node].car);
	if (field_value == NOT_FOUND) {
		return NOT_FOUND;
	}
	SECURE(field_value);

	current_node = MemoryPool[current_node].cdr;
	memptr core = resolve_expr(MemoryPool[current_node].car);
	if (core == NOT_FOUND) {
		return NOT_FOUND;
	}
	SECURE(core);

	if (MemoryPool[core].carKind == CONS && MemoryPool[core].cdrKind == NIL) {
		// core is already an object
		memptr new_node = new_data();
		MemoryPool[new_node].cdr = MemoryPool[core].car;
		MemoryPool[new_node].cdrKind = CONS;
		MemoryPool[core].car = new_node;
		MemoryPool[new_node].car = new_data();
		MemoryPool[new_node].carKind = CONS;

		memptr field = MemoryPool[new_node].car;
		MemoryPool[field].cdr = field_value;
		MemoryPool[field].cdrKind = CONS;
		MemoryPool[field].car = connect_string(
				get_string(MemoryPool[field_name].car), field);
		MemoryPool[field].carKind = STRING;

		return core;
	}

// base
	memptr base = safe_new_data();
	MemoryPool[base].cdrKind = NIL;
	MemoryPool[base].car = new_data();
	MemoryPool[base].carKind = CONS;

// core
	current_node = MemoryPool[base].car;
	MemoryPool[current_node].car = new_data();
	MemoryPool[current_node].carKind = CONS;
	MemoryPool[current_node].cdr = new_data();
	MemoryPool[current_node].cdrKind = CONS;
	memptr current_field = MemoryPool[current_node].car;
	MemoryPool[current_field].car = connect_string("core", current_field);
	MemoryPool[current_field].carKind = STRING;
	MemoryPool[current_field].cdr = core;
	MemoryPool[current_field].cdrKind = CONS;

// self
	current_node = MemoryPool[current_node].cdr;
	MemoryPool[current_node].car = new_data();
	MemoryPool[current_node].carKind = CONS;
	MemoryPool[current_node].cdr = new_data();
	MemoryPool[current_node].cdrKind = CONS;
	current_field = MemoryPool[current_node].car;
	MemoryPool[current_field].car = connect_string("self", current_field);
	MemoryPool[current_field].carKind = STRING;
	MemoryPool[current_field].cdr = base;
	MemoryPool[current_field].cdrKind = CONS;

// field
	current_node = MemoryPool[current_node].cdr;
	MemoryPool[current_node].car = new_data();
	MemoryPool[current_node].carKind = CONS;
	MemoryPool[current_node].cdrKind = NIL;
	current_field = MemoryPool[current_node].car;
	MemoryPool[current_field].car = connect_string(
			get_string(MemoryPool[field_name].car), current_field);
	MemoryPool[current_field].carKind = STRING;
	MemoryPool[current_field].cdr = field_value;
	MemoryPool[current_field].cdrKind = CONS;

	return base;
}

static memptr func_invoke(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 3) {
		ERROR("'invoke' : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr current_node = MemoryPool[func_expr].cdr;
	memptr object = first_arg;
	if (MemoryPool[object].carKind != CONS
			|| MemoryPool[object].cdrKind != NIL) {
		// TODO: for flexibility (?)
		return object;
	}

	current_node = MemoryPool[current_node].cdr;
	memptr field_name = MemoryPool[current_node].car;
	if (MemoryPool[field_name].carKind != STRING
			|| MemoryPool[field_name].cdrKind != CONS) {
		ERROR("'invoke' : second argument should be a field name\n");
		return NOT_FOUND;
	}

	memptr seek = seek_field(object, field_name);
	if (seek == NOT_FOUND) {
		ERROR("'invoke' : field not found\n");
		return NOT_FOUND;
	}

	return seek;
}

static memptr func_supps(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) != 3) {
		ERROR("'supps' : bad parameters count\n");
		return NOT_FOUND;
	}

	memptr current_node = MemoryPool[func_expr].cdr;
	memptr object = first_arg;
	if (MemoryPool[object].carKind != CONS
			|| MemoryPool[object].cdrKind != NIL) {
		// first argument not an object
		return nil;
	}

	current_node = MemoryPool[current_node].cdr;
	memptr field_name = MemoryPool[current_node].car;
	if (MemoryPool[field_name].carKind != STRING
			|| MemoryPool[field_name].cdrKind != CONS) {
		ERROR("'supps' : second argument should be a field name\n");
		return NOT_FOUND;
	}

	current_node = MemoryPool[object].car;
	memptr current_param = MemoryPool[current_node].car;
	if (strcmp(get_string(MemoryPool[current_param].car),
			get_string(MemoryPool[field_name].car)) == 0) {
		return t;
	}

	while (MemoryPool[current_node].cdrKind == CONS) {
		current_node = MemoryPool[current_node].cdr;
		current_param = MemoryPool[current_node].car;
		if (strcmp(get_string(MemoryPool[current_param].car),
				get_string(MemoryPool[field_name].car)) == 0) {
			return t;
		}
	}

	return nil;
}

static memptr func_error(memptr func_expr, memptr first_arg) {

	if (num_nodes(func_expr) > 2) {
		ERROR("'error' : bad parameters count\n");
		return NOT_FOUND;
	}

	if (num_nodes(func_expr) == 1) {
		// no message
		ERROR("user interrupt\n");
		return NOT_FOUND;
	}

// first parameter should be a message
	memptr param = first_arg;
	if (param == NOT_FOUND) {
		return NOT_FOUND;
	}
	if (MemoryPool[param].carKind != STRING
			|| MemoryPool[param].cdrKind != NIL) {
		// message not a string
		ERROR("'error' : bad parameter\n");
		return NOT_FOUND;
	}

	char error_helper[32 + 20 + 3];
	char name_buffer[32];
	char *name = get_string(MemoryPool[param].car);
	char temp;
	bool longer = (strlen(name) > 31), put_dots = false;
	if (longer) {
		temp = name[32];
		name[32] = 0;
		put_dots = true;
	}

// modulo escaping
	int i = 0, j = 0;
	while (i < 31 && j < strlen(name)) {
		name_buffer[i] = name[j];
		if (name_buffer[i] == '%') {
			if (i == 30) {
				name_buffer[i] = 0;
				put_dots = true;
			} else {
				name_buffer[++i] = '%';
			}
		}
		++i;
		++j;
	}
	name_buffer[i] = 0;

	sprintf(error_helper, "%s%s\n", name_buffer, put_dots ? "..." : "");
	if (longer) {
		name[32] = temp;
	}
	ERROR(error_helper);

	return NOT_FOUND;
}

/*
 * system management
 */

static memptr build_node(memptr data) {

	memptr node = safe_new_data();
	MemoryPool[node].carKind = CONS;
	MemoryPool[node].car = data;
	MemoryPool[node].cdrKind = NIL;
	return node;
}

static memptr list_insert(memptr list_head, memptr data) {

	memptr new_node = data, old_node;
	if (MemoryPool[list_head].carKind == NIL) {
		// list is empty
		MemoryPool[list_head].carKind = CONS;
		MemoryPool[list_head].car = new_node;
		MemoryPool[new_node].cdrKind = NIL;
	} else {
		// list is not empty
		old_node = MemoryPool[list_head].car;
		MemoryPool[list_head].car = new_node;
		MemoryPool[new_node].cdrKind = CONS;
		MemoryPool[new_node].cdr = old_node;
	}
	return new_node;
}

static void build_hash_nodes(memptr env) {

	MemoryPool[env].carKind = NIL;

	memptr node;
	for (int i = 0; i < HASH_FACTOR; i++) {
		node = safe_new_data();
		MemoryPool[node].carKind = NIL;
		list_insert(env, node);
	}
}

static int hash_index(memptr data) {

// hash by name (first letter)
	return ((get_string(MemoryPool[data].car))[0]) % HASH_FACTOR;
}

static memptr hash_node(memptr env, memptr data) {

	int hash_node_index = hash_index(data);
	memptr current = MemoryPool[env].car;
	for (int i = 0; i < hash_node_index; i++) {
		current = MemoryPool[current].cdr;
	}
	return current;
}

static void hash(memptr env, memptr data) {

	memptr current_hash_node = hash_node(env, data);
	list_insert(current_hash_node, build_node(data));
}

// TODO: check if it is doing what it supposed to do
static bool unhash(memptr env, memptr data) {

	memptr previous_node, current_node, current_data, hash_base;

	hash_base = previous_node = hash_node(env, data);
	if (MemoryPool[previous_node].carKind == NIL) {
		// hash list is empty
		return false;
	}

	do {
		current_node = (
				previous_node == hash_base ?
						MemoryPool[previous_node].car :
						MemoryPool[previous_node].cdr);
		current_data = MemoryPool[current_node].car;
		if (strcmp(get_string(MemoryPool[current_data].car),
				get_string(MemoryPool[data].car)) == 0) {
			if (previous_node == hash_base) {
				MemoryPool[previous_node].carKind =
						MemoryPool[current_node].cdrKind;
				MemoryPool[previous_node].car = MemoryPool[current_node].cdr;
			} else {
				MemoryPool[previous_node].cdrKind =
						MemoryPool[current_node].cdrKind;
				MemoryPool[previous_node].cdr = MemoryPool[current_node].cdr;
			}
			return true;
		}
		previous_node = current_node;
	} while (MemoryPool[current_node].cdrKind == CONS);

	return false;
}

static void insert_builtin(builtin func, char *name, int forbidden_index) {

	static int index = 0;
	builtins[index] = func;
	memptr data = new_data();
	MemoryPool[data].carKind = STRING;
	MemoryPool[data].car = connect_string(name, data);
	MemoryPool[data].cdrKind = CONS;
	MemoryPool[data].cdr = new_data();
	if (forbidden_index >= 0) {
		forbidden_array[forbidden_index] = MemoryPool[data].cdr;
	}
	MemoryPool[MemoryPool[data].cdr].carKind = INTEGER;
	MemoryPool[MemoryPool[data].cdr].car = index;
	MemoryPool[MemoryPool[data].cdr].cdrKind = INTEGER;

	hash(environment, data);
	index++;
}

void system_initialize() {

	initialize_memory_system();
	error = false;
	error_message[0] = 0;

	nil = new_data();	// 0
	t = new_data();		// 1
	MemoryPool[nil].carKind = NIL;
	MemoryPool[nil].cdrKind = NIL;
	MemoryPool[t].carKind = NIL;
	MemoryPool[t].cdrKind = NIL;

	environment = new_data();
	MemoryPool[environment].cdrKind = NIL;
	build_hash_nodes(environment);

	insert_builtin(func_plus, "+", -1);
	insert_builtin(func_minus, "-", -1);
	insert_builtin(func_mult, "*", -1);
	insert_builtin(func_div, "/", -1);
	insert_builtin(func_equals, "=", -1);
	insert_builtin(func_smaller, "<", -1);
	insert_builtin(func_greater, ">", -1);
	insert_builtin(func_cons, "cons", -1);
	insert_builtin(func_car, "car", -1);
	insert_builtin(func_cdr, "cdr", -1);
	insert_builtin(func_nil, "nil", -1);
	insert_builtin(func_true, "true", -1);
	insert_builtin(func_cond, "cond", 0);
	insert_builtin(func_setq, "setq", 1);
	insert_builtin(func_defun, "defun", 2);
	insert_builtin(func_or, "or", -1);
	insert_builtin(func_and, "and", -1);
	insert_builtin(func_xor, "xor", -1);
	insert_builtin(func_list, "list", -1);
	insert_builtin(func_value, "value", -1);
	insert_builtin(func_assign, "assign", 3);
	insert_builtin(func_invoke, "invoke", -1);
	insert_builtin(func_supps, "supps", -1);
	insert_builtin(func_error, "error", -1);
}

static memptr lookup(memptr expr, LookUp lu) {

	memptr next_env = environment, current_env, next_node, current_node,
			current_data;
	do {
		current_env = next_env;
		next_node = hash_node(current_env, expr);
		if (MemoryPool[next_node].carKind == NIL) {
			// hash list is empty
			next_env = MemoryPool[current_env].cdr;
			continue;
		}

		next_node = MemoryPool[next_node].car;
		do {
			current_node = next_node;
			current_data = MemoryPool[current_node].car;
			if (strcmp(get_string(MemoryPool[current_data].car),
					get_string(MemoryPool[expr].car)) == 0) {
				switch (lu) {
				case ENVIRONMENT:
					return current_env;
				case DATA:
					return MemoryPool[current_data].cdr;
				default:
					return NOT_FOUND;
				}
				return MemoryPool[current_data].cdr;
			}
			next_node = MemoryPool[next_node].cdr;
		} while (MemoryPool[current_node].cdrKind != NIL);

		next_env = MemoryPool[current_env].cdr;
	} while (MemoryPool[current_env].cdrKind != NIL);

	char error_helper[32 + 20 + 3];
	char name_buffer[32];
	char *name = get_string(MemoryPool[expr].car);
	char temp;
	bool longer = (strlen(name) > 31), put_dots = false;
	if (longer) {
		temp = name[32];
		name[32] = 0;
		put_dots = true;
	}

// modulo escaping
	int i = 0, j = 0;
	while (i < 31 && j < strlen(name)) {
		name_buffer[i] = name[j];
		if (name_buffer[i] == '%') {
			if (i == 30) {
				name_buffer[i] = 0;
				put_dots = true;
			} else {
				name_buffer[++i] = '%';
			}
		}
		++i;
		++j;
	}
	name_buffer[i] = 0;

	sprintf(error_helper, "symbol %s%s not defined\n", name_buffer,
			put_dots ? "..." : "");
	if (longer) {
		name[32] = temp;
	}
	ERROR(error_helper);
	return NOT_FOUND;
}

static bool create_environment(memptr values_list, memptr parameters_list,
		memptr first_arg) {

	int values_size = num_nodes(values_list), parameters_size = num_nodes(
			parameters_list);
	if (values_size != parameters_size) {
		return false;
	}

	if (values_size == 0) {
		memptr temp = environment;
		environment = new_data();
		MemoryPool[environment].cdrKind = CONS;
		MemoryPool[environment].cdr = temp;
		build_hash_nodes(environment);
		return true;
	}

	memptr current_value_node, next_value_node = values_list, current_value,
			current_param_node, next_param_node = parameters_list,
			current_param, current_value_resolved, current_param_dup;

	bool first_iteration = true;
	memptr dup_list = safe_new_data(), dup_list_end = NOT_FOUND;
	MemoryPool[dup_list].carKind = MemoryPool[dup_list].cdrKind = NIL;
	do {
		current_value_node = next_value_node;
		current_param_node = next_param_node;
		current_value = MemoryPool[current_value_node].car;
		current_param = MemoryPool[current_param_node].car;
		current_param_dup = safe_new_data();
		MemoryPool[current_param_dup].carKind = STRING;
		MemoryPool[current_param_dup].car = MemoryPool[current_param].car;
		MemoryPool[current_param_dup].cdrKind = CONS;

		current_value_resolved =
				first_iteration ? first_arg : resolve_expr(current_value);
		if (current_value_resolved == NOT_FOUND) {
			// TODO: should all arguments be resolved?
			return false;
		}
		MemoryPool[current_param_dup].cdr = current_value_resolved;
		if (dup_list_end == NOT_FOUND) {
			MemoryPool[dup_list].car = current_param_dup;
			MemoryPool[dup_list].carKind = CONS;
			MemoryPool[dup_list].cdrKind = NIL;
			dup_list_end = dup_list;
		} else {
			MemoryPool[dup_list_end].cdr = new_data();
			MemoryPool[dup_list_end].cdrKind = CONS;
			dup_list_end = MemoryPool[dup_list_end].cdr;
			MemoryPool[dup_list_end].car = current_param_dup;
			MemoryPool[dup_list_end].carKind = CONS;
			MemoryPool[dup_list_end].cdrKind = NIL;
		}
//		hash(environment, current_param_dup);

		next_value_node = MemoryPool[current_value_node].cdr;
		next_param_node = MemoryPool[current_param_node].cdr;
		first_iteration = false;
	} while (MemoryPool[current_value_node].cdrKind == CONS);

	memptr temp = environment;
	environment = new_data();
	MemoryPool[environment].cdrKind = CONS;
	MemoryPool[environment].cdr = temp;
	build_hash_nodes(environment);

	// hashing
	memptr current_node, next_node = dup_list;
	do {
		current_node = next_node;
		hash(environment, MemoryPool[current_node].car);
		next_node = MemoryPool[current_node].cdr;
	} while (MemoryPool[current_node].cdrKind == CONS);

	return true;
}

static void destroy_environment() {

	// we assume the base environment is not being destroyed
	environment = MemoryPool[environment].cdr;
}

/*
 * TODO: rethink this?
 * (nil nil)		=> nil
 * (integer nil)	=> integer
 * (string nil)		=> string
 * (cons cons)		=> cons
 * (string cons)	=> function/variable name
 * (cons integer)	=> function expression
 */
memptr resolve_expr(memptr expr) {

	if (expr == 0 || expr == 1) {
		// nil or true
		return expr;
	}

	if (MemoryPool[expr].cdrKind == NIL) {
		// final expression
		return expr;
	}

	if (MemoryPool[expr].carKind == CONS && MemoryPool[expr].cdrKind == CONS) {
		// a cons. not being copied
		return expr;
	}

	if (MemoryPool[expr].carKind == CONS
			&& MemoryPool[expr].cdrKind == INTEGER) {
		// a function expression
		return resolve_func_expr(MemoryPool[expr].car);
	}

// else a name of variable or a function
	return lookup(expr, DATA);
}

memptr resolve_func_expr(memptr func_expr) {

	memptr func_body = resolve_expr(MemoryPool[func_expr].car);
	memptr first_arg = NOT_FOUND;
	bool forbidden = initial_analyze_forbidden(func_body);

	if (!forbidden) {
		first_arg = resolve_expr(MemoryPool[MemoryPool[func_expr].cdr].car);
		if (first_arg == NOT_FOUND) {
			return NOT_FOUND;
		}
	}

	bool internal_object_method = false;
	if (MemoryPool[MemoryPool[func_expr].car].carKind == STRING
			&& MemoryPool[MemoryPool[func_expr].car].cdrKind == CONS
			&& !forbidden) {
		// first argument is a *legal* *name*
		if (first_arg != NOT_FOUND && MemoryPool[first_arg].carKind == CONS
				&& MemoryPool[first_arg].cdrKind == NIL) {
			// second argument is an object
			memptr seek = seek_field(first_arg, MemoryPool[func_expr].car);
			if (seek != NOT_FOUND) {
				// hidden call to the objects method!
				// we can assume func_body evaluation did not affect the system
				func_body = seek;
				internal_object_method = true;
			}
		}
	}

	memptr result;
	if (MemoryPool[func_body].carKind == INTEGER
			&& MemoryPool[func_body].cdrKind == INTEGER) {
		// builtin function
		result = builtins[MemoryPool[func_body].car](func_expr, first_arg);
	} else if (MemoryPool[func_body].carKind == CONS
			&& MemoryPool[func_body].cdrKind == INTEGER) {
		// user function
		if (!create_environment(
				MemoryPool[func_expr].cdrKind == NIL ?
						nil : MemoryPool[func_expr].cdr,
				MemoryPool[MemoryPool[func_body].car].car, first_arg)) {
			ERROR("<?>: parameter mismatch\n");
			result = NOT_FOUND;
		} else {
			result = resolve_expr(
					MemoryPool[MemoryPool[MemoryPool[func_body].car].cdr].car);
			destroy_environment();
		}
	} else {
		if (internal_object_method) {
			// a simple field
			return func_body;
		}
		// cannot be resolved as a function
		ERROR("expression could not be resolved as a function\n");
		result = NOT_FOUND;
	}

	return result;
}

void print_mem_at(int address) {

	printf("%3d [", address);
	if (MemoryPool[address].carKind == NIL || address == nil) {
		printf("  ----   ");
	} else if (address == 1) {
		printf("    t    ");
	} else if (MemoryPool[address].carKind == INTEGER) {
		printf("%8d ", MemoryPool[address].car);
	} else if (MemoryPool[address].carKind == STRING) {
		printf("%8s ", get_string(MemoryPool[address].car));
	} else {
		printf("->%6d ", MemoryPool[address].car);
	}
	if (MemoryPool[address].cdrKind == NIL) {
		printf("  ----  ");
	} else if (MemoryPool[address].cdrKind == INTEGER) {
		printf("%8d", MemoryPool[address].cdr);
	} else if (MemoryPool[address].cdrKind == STRING) {
		printf("%8s", get_string(MemoryPool[address].cdr));
	} else {
		printf("->%6d", MemoryPool[address].cdr);
	}
	printf("]");
}

void print_mem(int m) {

	for (int i = 0; i < m; i++) {
		print_mem_at(i);
		printf("\n");
	}
}

/*
 * debug functions
 */

memptr create_array(int size) {

	if (size == 0) {
		return nil;
	}
	memptr current_node = new_data();
	memptr start = current_node;
	MemoryPool[current_node].carKind = NIL;
	MemoryPool[current_node].cdrKind = NIL;
	for (int i = 1; i < size; i++) {
		MemoryPool[current_node].cdrKind = CONS;
		MemoryPool[current_node].cdr = new_data();
		current_node = MemoryPool[current_node].cdr;
		MemoryPool[current_node].carKind = NIL;
		MemoryPool[current_node].cdrKind = NIL;
	}
	return start;
}

void insert_array(memptr array, memptr data, int index) {

	memptr current_node = array;
	for (int i = 0; i < index; i++) {
		current_node = MemoryPool[current_node].cdr;
	}
	MemoryPool[current_node].carKind = CONS;
	MemoryPool[current_node].car = data;
}

memptr at_array(memptr array, int index) {

	memptr current_node = array;
	for (int i = 0; i < index; i++) {
		current_node = MemoryPool[current_node].cdr;
	}
	return MemoryPool[current_node].car;
}

void set_cons(memptr data, int typeCar, memptr car, int typeCdr, memptr cdr) {

	MemoryPool[data].carKind = typeCar;
	MemoryPool[data].cdrKind = typeCdr;
	MemoryPool[data].car = car;
	MemoryPool[data].cdr = cdr;
}
