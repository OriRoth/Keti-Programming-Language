#include "system.h"
#include <stdio.h>

#define INPUT_BUFFER_SIZE (1<<12)
#define SYNTAX_ERROR -1
#define SKIP_SPACES(iter, buff)	do {										\
									while (isspace(buff[iter])) {++iter;}	\
									} while(0)

char line_buffer[INPUT_BUFFER_SIZE];

void print_error() {
	if (error) {
		fprintf(stderr, "Line Failure => ");
		fprintf(stderr, error_message);
	}
}

int translate_function(memptr *parent, int start) {

	int iter = start + 1;
	SKIP_SPACES(iter, line_buffer);
	if (line_buffer[iter] == ')') {
		// not a function but a nil
		*parent = nil;
		return ++iter;
	}

	MemoryPool[*parent].car = new_data();
	MemoryPool[*parent].carKind = CONS;
	MemoryPool[*parent].cdrKind = INTEGER;
	memptr previous_node = NOT_FOUND;
	memptr current_node = MemoryPool[*parent].car;
	MemoryPool[current_node].carKind = MemoryPool[current_node].cdrKind = NIL;
	memptr current_value;

	while (true) {
		MemoryPool[current_node].car = new_data();
		MemoryPool[current_node].carKind = CONS;
		current_value = MemoryPool[current_node].car;
		MemoryPool[current_value].carKind = NIL;
		MemoryPool[current_value].cdrKind = NIL;
		SKIP_SPACES(iter, line_buffer);
		if (line_buffer[iter] == ')') {
			// end of function
			if (previous_node != NOT_FOUND) {
				MemoryPool[previous_node].cdrKind = NIL;
			}
			return ++iter;
		} else if (line_buffer[iter] == '(') {
			// start of a function
			iter = translate_function(&current_value, iter);
			if (iter == SYNTAX_ERROR) {
				return SYNTAX_ERROR;
			}
			MemoryPool[current_node].car = current_value;
		} else if (isdigit(line_buffer[iter])) {
			// a number
			int number = 0;
			while (iter < INPUT_BUFFER_SIZE && isdigit(line_buffer[iter])) {
				number *= 10;
				number += line_buffer[iter++] - '0';
			}
			MemoryPool[current_value].carKind = INTEGER;
			MemoryPool[current_value].cdrKind = NIL;
			MemoryPool[current_value].car = number;
		} else if (line_buffer[iter] == '\"') {
			// a string
			int string_start = iter + 1;
			while (++iter < INPUT_BUFFER_SIZE && line_buffer[iter] != '\"')
				;
			if (iter >= INPUT_BUFFER_SIZE) {
				return SYNTAX_ERROR;
			}
			line_buffer[iter++] = 0;
			MemoryPool[current_value].car = connect_string(
					&(line_buffer[string_start]), current_value);
			MemoryPool[current_value].carKind = STRING;
			MemoryPool[current_value].cdrKind = NIL;
		} else {
			// a symbol
			int symbol_start = iter;
			while (iter < INPUT_BUFFER_SIZE && line_buffer[iter] != '('
					&& line_buffer[iter] != ')' && line_buffer[iter] != '\"'
					&& !isspace(line_buffer[iter])) {
				++iter;
			};
			if (iter >= INPUT_BUFFER_SIZE) {
				return SYNTAX_ERROR;
			}
			char temp = line_buffer[iter];
			line_buffer[iter] = 0;
			if (strcmp(&(line_buffer[symbol_start]), "t") == 0) {
				// t
				MemoryPool[current_node].car = true;
			} else {
				MemoryPool[current_value].car = connect_string(
						&(line_buffer[symbol_start]), current_value);
				MemoryPool[current_value].carKind = STRING;
				MemoryPool[current_value].cdr = nil;
				MemoryPool[current_value].cdrKind = CONS;
			}
			line_buffer[iter] = temp;
		}

		MemoryPool[current_node].cdr = new_data();
		MemoryPool[current_node].cdrKind = CONS;
		previous_node = current_node;
		current_node = MemoryPool[current_node].cdr;
	}

	return SYNTAX_ERROR;
}

void print_result(memptr result) {

	if (result == NOT_FOUND) {
		if (error) {
			print_error();
		} else {
			fprintf(stderr, "undefined error\n");
		}
	} else {
		if (result == nil) {
			// nil
			printf("nil : nil\n");
		} else if (result == true) {
			// true
			printf("true : true\n");
		} else if (MemoryPool[result].cdrKind == NIL) {
			if (MemoryPool[result].carKind == INTEGER) {
				// integer
				printf("%d : integer\n", MemoryPool[result].car);
			} else if (MemoryPool[result].carKind == STRING) {
				// string
				printf("%s : string\n", get_string(MemoryPool[result].car));
			} else if (MemoryPool[result].carKind == CONS) {
				// object
				printf("<?> : object\n");
			}
		} else if (MemoryPool[result].cdrKind == CONS) {
			if (MemoryPool[result].carKind == CONS) {
				// cons
				// TODO: add cons type
				printf("(<?> , <?>) : cons\n");
			} else if (MemoryPool[result].carKind == STRING) {
				// symbol
				printf("%s : symbol\n", get_string(MemoryPool[result].car));
			}
		} else if (MemoryPool[result].cdrKind == INTEGER) {
			// function
			// TODO: add functions name
			printf("<?> : function\n");
		}
	}
}

void print_logo() {

	printf("\
 __   __  ___   _______  ______    _______    ___      ___   _______  _______ \n\
|  |_|  ||   | |       ||    _ |  |       |  |   |    |   | |       ||       |\n\
|       ||   | |       ||   | ||  |   _   |  |   |    |   | |  _____||    _  |\n\
|       ||   | |       ||   |_||_ |  | |  |  |   |    |   | | |_____ |   |_| |\n\
|       ||   | |      _||    __  ||  |_|  |  |   |___ |   | |_____  ||    ___|\n\
| ||_|| ||   | |     |_ |   |  | ||       |  |       ||   |  _____| ||   |    \n\
|_|   |_||___| |_______||___|  |_||_______|  |_______||___| |_______||___|    \n\
by Ori Roth\n\n");
}

int main() {

	system_initialize();
	SECURITY_INIT();
	print_logo();

	bool correction = 1;
	while (true) {

		if (correction) {
			printf("> ");
			fflush(stdout);
		}
		correction = 1 - correction;

		int line_iter = 0;
		int parentheses_counter = 0;
		line_buffer[line_iter] = getchar();
		if (line_buffer[line_iter] == '(') {
			// a lisp commend
			parentheses_counter = 1;
			line_iter++;
			while (parentheses_counter > 0 && line_iter < INPUT_BUFFER_SIZE) {
				line_buffer[line_iter] = getchar();
				if (line_buffer[line_iter] == 10) {
					continue;
				} else if (line_buffer[line_iter] == '(') {
					++parentheses_counter;
				} else if (line_buffer[line_iter] == ')') {
					--parentheses_counter;
				}
				++line_iter;
			}
			if (line_iter >= INPUT_BUFFER_SIZE) {
				fprintf(stderr, "line too big\n");
				continue;
			}

			memptr translated_expression = safe_new_data();
			if (translate_function(&translated_expression, 0) == SYNTAX_ERROR) {
				ERROR("syntax error\n");
				print_error();
			} else {
				memptr result = resolve_expr(translated_expression);
				print_result(result);
			}
		} else {
			// TODO: add system commends
		}

		fflush(stderr);
		fflush(stdout);
		error = false;
		error_message[0] = 0;
		SECURITY_INIT();

	}

	return 0;
}
