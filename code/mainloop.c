#include "system.h"
#include "error.h"
#include <stdio.h>
#include <windows.h>

// input buffer size
#define INPUT_BUFFER_SIZE (1<<12)
#define SYNTAX_ERROR (-1)
// skips spaces
#define SKIP_SPACES(iter, buff)	do {										\
									while (is_space(buff[iter])) {++iter;}	\
								} while(0)

// input buffer, contains entire line at a time. line defined by ...(...)
char line_buffer[INPUT_BUFFER_SIZE];

// input stream
FILE* input;
int stdin_backup;
// true if done reading input file
bool done_reading = false;
// counts parentheses- increases uppon '(', decreases uppon ')'
int parentheses_counter;

/*
 * receives input stream, return next token
*/
char get_input(FILE *input) {

	char c;
	if (!done_reading) {
		c = fgetc(input);
	} else {
		c = getchar();
	}
	if (c == ';' && parentheses_counter != 0) {
		while (c != '\n') {
			if (!done_reading) {
				c = fgetc(input);
			} else {
				c = getchar();
			}
		}
	}
	while (c != EOF && c != '\n' && c != '(' && parentheses_counter == 0) {
		if (c == ';') {
			while (c != '\n') {
				if (!done_reading) {
					c = fgetc(input);
				} else {
					c = getchar();
				}
			}
		}
		if (!done_reading) {
			c = fgetc(input);
		} else {
			c = getchar();
		}
	}
	if (!done_reading) {
		if (c == '\n' && parentheses_counter == 0) {
			return get_input(input);
		}
		if (c != EOF) {
			fprintf(stdout, "%c", c);
		} else {
			done_reading = true;
			close(stdin);
			dup2(stdin_backup, stdin);
			close(stdin_backup);
			input = stdin;
			return ' ';
		}
	}
	return c;
}

/*
 * moves a string from the line buffer to the strings buffer
 * argument is starting index in the line buffer
*/
void move_string_to_string_buffer(int start) {

	int i = 0;
	do {
		string_buffer[i++] = line_buffer[start];
	} while (line_buffer[start++] != '\0' && i < STRING_BUFFER_SIZE);
}

/*
 * receives a character, returns true if it is a space
*/
bool is_space(char c) {

	return c == '\n' || c == '\t' || c == ' ' || c == '\r';
}

/*
 * receives a character, returns true if it is a digit
*/
bool is_digit(char c) {

	return (c >= '0' && c <= '9');
}

/*
 * parent - expression parent in translation tree
 * start - start index in line buffer
 * returns SYNTAX_ERROR uppon translation error, or the end index of the expression in the line buffer
*/
bool is_nil;	// true if result is nil
int translate_function(memptr parent, int start) {

	int iter = start + 1;
	memptr current_node = parent;
	memptr previous_node = NOT_FOUND;

	while (true) {
		SKIP_SPACES(iter, line_buffer);
		if (line_buffer[iter] == ')') {
			// end of function
			is_nil = IS_NOT_FOUND(previous_node);
			if (!IS_NOT_FOUND(previous_node)) {
				SET_CDR(previous_node, nil);
			}
			return ++iter;
		} else if (line_buffer[iter] == '(') {
			// start of a function
			SET_CAR(current_node, allocate_cons());
			iter = translate_function(GET_CAR(current_node), iter);
			if (iter == SYNTAX_ERROR) {
				is_nil = false;
				return SYNTAX_ERROR;
			}
			if (is_nil) {
				SET_CAR(current_node, nil);
			}
		} else if (is_digit(line_buffer[iter])) {
			// a number
			int number = 0;
			while (iter < INPUT_BUFFER_SIZE && is_digit(line_buffer[iter])) {
				number *= 10;
				number += line_buffer[iter++] - '0';
			}
			SET_CAR(current_node, MAKE_INTEGER(number));
		} else if (line_buffer[iter] == '\"') {
			// a string
			int string_start = iter + 1;
			int len = 0;
			while (++iter < INPUT_BUFFER_SIZE && line_buffer[iter] != '\"')
				len++;
			if (iter >= INPUT_BUFFER_SIZE) {
				is_nil = false;
				return SYNTAX_ERROR;
			}
			if (len == 0) {
				SET_CAR(current_node, nil);
				iter++;
			} else {
				line_buffer[iter++] = 0;
				move_string_to_string_buffer(string_start);
				SET_CAR(current_node, allocate_string_from_string_buffer());
			}
		} else {
			// a symbol
			int symbol_start = iter;
			while (iter < INPUT_BUFFER_SIZE && line_buffer[iter] != '('
					&& line_buffer[iter] != ')' && line_buffer[iter] != '\"'
					&& !is_space(line_buffer[iter])) {
				++iter;
			};
			if (iter >= INPUT_BUFFER_SIZE) {
				is_nil = false;
				return SYNTAX_ERROR;
			}
			char temp = line_buffer[iter];
			line_buffer[iter] = 0;
			if (line_buffer[symbol_start] == 't'
					&& line_buffer[symbol_start + 1] == '\0') {
				// t
				SET_CAR(current_node, t);
			} else {
				move_string_to_string_buffer(symbol_start);
				SET_CAR(current_node, allocate_cons());
				SET_CAR(GET_CAR(current_node), symbol_identifier);
				SET_CDR(GET_CAR(current_node), allocate_cons());
				SET_CAR(GET_CDR(GET_CAR(current_node)),
						allocate_string_from_string_buffer());
				SET_CDR(GET_CDR(GET_CAR(current_node)), nil);
			}
			line_buffer[iter] = temp;
		}

		SET_CDR(current_node, allocate_cons());
		previous_node = current_node;
		current_node = GET_CDR(current_node);
	}

	is_nil = false;
	return SYNTAX_ERROR;
}

/*
 * receives an addresss of an object on the pool, and prints it
 * recursive printing of cons, limited by MAX_DEPTH
*/
#define MAX_DEPTH 4
void print_result(memptr result, int depth) {

	if (error) {
		fprintf(stderr, "\n%s failure => %s",
				&error_first[error_first_identifier * ERROR_FIRST_LIMIT],
				&error_second[error_second_identifier * ERROR_SECOND_LIMIT]);
		return;
	}

	if (result == NOT_FOUND) {
		fprintf(stderr, "undefined error\n");
	} else if (depth >= MAX_DEPTH) {
		fprintf(stdout, ".");
	} else {
		if (TYPE_NIL(result)) {
			fprintf(stdout, "nil");
		} else if (TYPE_T(result)) {
			fprintf(stdout, "t");
		} else if (TYPE_INTEGER(result)) {
			fprintf(stdout, "%d",
					((signed) fix_integer(INTEGER_VALUE(result))));
		} else if (TYPE_NATIVE(result)) {
			fprintf(stdout, "<native %s>",
					&(native_names_buffer[(result - 2) * NATIVE_NAME_LIMIT]));
		} else if (TYPE_STRING(result)) {
			fprintf(stdout, "\"%s\"",
					&strings_pool[HANDLER_STRING_ADDRESS(
							GET_CDR(STRING_HANDLER_ADDRESS(result)))]);
		} else if (TYPE_CONS(result)) {
			fprintf(stdout, "( ");
			print_result(GET_CAR(result), depth + 1);
			fprintf(stdout, " , ");
			print_result(GET_CDR(result), depth + 1);
			fprintf(stdout, " )");
		} else if (result == lambda_identifier) {
			fprintf(stdout, "<lambda-identifier>");
		} else if (result == symbol_identifier) {
			fprintf(stdout, "<symbol-identifier>");
		}
	}
}

//#define LOADING_BAR_WAIT_TIME 200
void print_logo() {

	fprintf(stdout,
			"\
      .-.                                   .-.                   \n\
        /|/|    .-.                        / (_)     .-.          \n\
       /   |    `-'.-.    ).--..-._.      /          `-' .   .-.  \n\
      /    |   /  (      /    (   )      /          /   / \\  /  ) \n\
 .-' /     |_.(__. `---'/      `-'    .-/.    .-._.(__./ ._)/`-'  \n\
(__.'      `.                        (_/ `-._.        /    /      \n\
by Ori Roth\n\n");
#ifdef LOADING_BAR_WAIT_TIME
	for (int i=0 ; i<11 ; i++) {
		fprintf(stdout, "loading... [");
		for (int j=0 ; j<i ; j++) {
			fprintf(stdout, "-");
		}
		fprintf(stdout, "] %d%%", i * 10);
		fflush(stdout);
		Sleep(LOADING_BAR_WAIT_TIME);
		fprintf(stdout, "\r");
	}
	for (int i=0 ; i<28 ; i++) {
		fprintf(stdout, " ");
	}
	fprintf(stdout, "\r");
#endif
}

#define DEBUG
#ifdef DEBUG
#define PRINT_MEM_LIMIT 300
void print_mem() {

	fprintf(stdout, "environment=%d\n", environment);
	fprintf(stdout, "security_head=%d\n", security_head);
	for (int i = 0; i < PRINT_MEM_LIMIT; i++) {
		fprintf(stdout, "%4d: ( %d , %d )\n", i, GET_CAR(i), GET_CDR(i));
	}
	for (int i = 0; i < PRINT_MEM_LIMIT; i++) {
		fprintf(stdout, "%4d: %c\n", i,
				(strings_pool[i] == 0 ? '0' : strings_pool[i]));
	}
}
#endif

/*
 * the main loop: may receive an input file. if it does, we reads it first
 * fill the line buffer- when completing a line, translates it (using translate_function),
 * then evaluate it (using resolve_expression) and prints the result (or error) (using print_result)
*/
int main(int argc, char *argv[]) {

	if (argc > 1) {
		stdin_backup = dup(stdin);
		input = fopen(argv[1], "r+");
	} else {
		input = stdin;
		done_reading = true;
	}

	system_initialize();
	security_init();
	print_logo();
	fflush(stdout);

	while (true) {

		error = false;

		fprintf(stdout, "> ");
		fflush(stdout);

		int line_iter = 0;
		parentheses_counter = 0;
		line_buffer[line_iter] = get_input(input);
		while (is_space(line_buffer[line_iter]))
			line_buffer[line_iter] = get_input(input);
		if (line_buffer[line_iter] == '(') {
			// a lisp commend
			parentheses_counter = 1;
			line_iter++;
			while (parentheses_counter > 0 && line_iter < INPUT_BUFFER_SIZE) {
				line_buffer[line_iter] = get_input(input);
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

			memptr translated_expression = safe_allocate_cons();
			translate_function(translated_expression, 0);
			if (is_nil) {
				translated_expression = nil;
			}
			if (translated_expression == NOT_FOUND) {
				COMMIT_ERROR(ERROR_SYSTEM, ERROR_LE);
				print_result(0, 0);
			} else {
				fflush(stdout);
				memptr result = resolve_expression(translated_expression);
				if (!done_reading) {
					fprintf(stdout, "\n");
				}
				if (!error) {
					fprintf(stdout, "\n~ ");
				} else {
					fflush(stderr);
					fflush(stdout);
				}
				print_result(result, 0);
				fprintf(stdout, "\n\n");
			}
		} else {
			while (get_input(input) != 10)
				;
			fprintf(stdout, "\n");
		}

		fflush(stderr);
		fflush(stdout);
		security_init();
	}

	return 0;
}
