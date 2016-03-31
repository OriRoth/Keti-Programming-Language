# Keti Programming Language
Elegant, minimalistic virtualization of a classic Lisp Machine, written in C.
For Katy, my inspiration and love.

ATTENTION!
Line 51 in file "native.c" should be "memptr env = lookup(GET_CAR(symbol_value), ENVIRONMENT);" instead of "memptr env = lookup(symbol, ENVIRONMENT);"
As you can see, the BUG leads to malacious nested "define" operations.
I have not changed the code because the more complex examples won't work with the change.
One should update the code before try doing something like:
(define f (lambda () (define x 2)))
(define x 1)
(f) ; x should be 2, but it is 1

Line 105 in system.c should be:
if (string_compare(GET_CAR(GET_CDR(current_data)), GET_CAR(GET_CDR(data)))) {
instead of
if (string_compare(GET_CAR(GET_CDR(current_data)), GET_CAR(data))) {
causes very strange bug: by luck, the call (define define define) would delete quote definition.
