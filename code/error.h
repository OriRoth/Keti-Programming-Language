#ifndef ERROR_H_
#define ERROR_H_

#include <stdbool.h>

/* error is a combination of two messages. */
// maximal size of first messege
#define ERROR_FIRST_SIZE 18
// number of first messeges
#define ERROR_FIRST_LIMIT 8
// maximal size of second messege
#define ERROR_SECOND_SIZE 6
// number of second messeges
#define ERROR_SECOND_LIMIT 24

// first messeges
enum {
	ERROR_MEMORY = 0,
	ERROR_SYSTEM,
	ERROR_QUOTE,
	ERROR_DEFINE,
	ERROR_LAMBDA,
	ERROR_IF,
	ERROR_ATOM,
	ERROR_CONS,
	ERROR_CAR,
	ERROR_CDR,
	ERROR_PLUS,
	ERROR_MINUS,
	ERROR_MULT,
	ERROR_DIV,
	ERROR_EQUALS,
	ERROR_SMALLER,
	ERROR_GREATER,
	ERROR_NIL
};

// second messeges
enum {
	ERROR_OOM = 0, ERROR_IPC, ERROR_IPT, ERROR_EM, ERROR_LE, ERROR_SU
};

// true if error occured
bool error;
// contain the error first and second types
int error_first_identifier;
int error_second_identifier;
// errors messeges source
char error_first[ERROR_FIRST_SIZE * ERROR_FIRST_LIMIT];
char error_second[ERROR_SECOND_SIZE * ERROR_SECOND_LIMIT];

// declares an error. only initial error is regarded
#define COMMIT_ERROR(first_messege, second_messege)	do {												\
														if (!error) {									\
															error = true;								\
															error_first_identifier = first_messege;		\
															error_second_identifier = second_messege;	\
														}												\
													} while (0)

#endif /* ERROR_H_ */
