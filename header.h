#pragma once

#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <error.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>

typedef ssize_t boolean;

struct string_t {
	char* st;
	int capacity;
};

enum instruction_code_t {
	E_NAME,
	G_SIZE,
	E_SIZE,
	L_SIZE,
	N_LINK,
	E_INUM,
	EXEC,
	UNKNOWN
};

union instruction_data_t {
	size_t integer;
	char* string;
};

struct instruction_t {
	enum instruction_code_t instruction;
	union instruction_data_t instruction_data;	
};

struct linux_dirent64 {
	ino64_t d_ino;
	off64_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

struct function_t {
	size_t number;
	struct instruction_t* instruction_sequence;
};

struct checker_t {
    boolean(*function)(char* const name, ino64_t ino, size_t size, size_t nlink, void(*launcher)(char* const file, char* const executee));
    size_t size;
};

#ifdef __cplusplus
extern "C" {
	void critical(const char* message);
	void launch(char* const executee, char* const file);
	struct checker_t compileJIT(struct function_t);
}
#else 
void critical(const char* message);
void launch(char* const executee, char* const file);
struct checker_t compileJIT(struct function_t);
#endif