all:
	cc find.c -o find
debug:
	cc -g -Wall -fsanitize=address find.c -o find
