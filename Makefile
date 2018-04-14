all: 
	gcc *.c *.cpp *.S -o find
check:
	gcc -Wall -fsanitize=address *.c *.cpp *.S -o find
debug:
	gcc -g *.c *.cpp *.S -o find
list:
	gcc -S *.c *.cpp *.S
test: check
	./find ./tests -name HEAD
	./find ./tests -nlink 2
	./find ./tests -size 50
	./find ./tests +size 50
	./find ./tests =size 101
	./find ./tests -name HEAD -exec /bin/cat
	
