#include "header.h"

const boolean true = -1;
const boolean false = 0;
char* const* envp;

void critical(const char* message) {
	if (message) {
		write(2, message, strlen(message));
	}
	exit(EXIT_FAILURE);
}

#define EXIT_CODE_PRINTING(IF, THEN, INFO)  \
if (                                        \
	IF(exit_code) &&                        \
	!(                                      \
		WIFEXITED(exit_code) &&             \
		WEXITSTATUS(exit_code) == 0         \
	)                                       \
) {                                         \
	printf(INFO "%d\n", THEN(exit_code));   \
	fflush(stdout);                         \
} 

void launch(char* const file, char* const executee) {
	
	const pid_t pid = fork();
	if (pid == -1) {
		perror("fork"); 
		return;
	}
	
	if (!pid) { 
		// child

		char* const args[3] = {executee, file, 0};
		execve(executee, args, envp);

		perror("execute");
		critical(0);
	} else {    
		// host
		
		int exit_code = 0;
		do {
			if (waitpid(pid, &exit_code, WUNTRACED | WCONTINUED) == -1) {
				perror("waitpid");
				break;
			}

			EXIT_CODE_PRINTING(WIFEXITED, WEXITSTATUS, "exited, exit code: ");
			EXIT_CODE_PRINTING(WIFSIGNALED, WTERMSIG, "killed with signal: ");
			EXIT_CODE_PRINTING(WIFSTOPPED, WSTOPSIG, "stopped with singal: ");
			if (WIFCONTINUED(exit_code)) {
				printf("continued\n");
				fflush(stdout);
			}
		} while (!(WIFEXITED(exit_code) || WIFSIGNALED(exit_code)));
	}
}

int try_open(const char *file, int oflag) {
	int fd = open(file, oflag);
	if (fd == -1) {
		perror("open");
	}
	return fd;
}

#define RETURN_FALSE_IF(condition) if (condition) { return false; }

boolean evaluate(struct function_t functor, char* const name, ino64_t ino, size_t size, size_t nlink){
	for (int i = 0; i < functor.number; i++) {
		size_t data = functor.instruction_sequence[i].instruction_data.integer;
		char* string = functor.instruction_sequence[i].instruction_data.string;
		enum instruction_code_t instruction = functor.instruction_sequence[i].instruction;
		switch (instruction) {
			case E_INUM: RETURN_FALSE_IF(ino != data)          break;
			case E_NAME: RETURN_FALSE_IF(strcmp(name, string)) break;
			case G_SIZE: RETURN_FALSE_IF(size < data)          break;
			case E_SIZE: RETURN_FALSE_IF(size != data)         break;
			case L_SIZE: RETURN_FALSE_IF(size > data)          break;
			case N_LINK: RETURN_FALSE_IF(nlink != data)        break;
			case EXEC: launch(string, name);                   break;
			case UNKNOWN: critical("unknown instruction");
		}
	}
	return true;
}

const char* get_type(char d_type) {
	switch(d_type) {
		case DT_REG : return   "regular";
		case DT_DIR : return "directory";
		case DT_FIFO: return      "FIFO";
		case DT_SOCK: return    "socket";
		case DT_LNK : return   "symlink";
		case DT_BLK : return "block dev";
		case DT_CHR : return  "char dev";
		default: return "unknown";
	}
}

#define BUF_SIZE 1024

void directories_walk(struct string_t* cwd, struct checker_t functor) {
	int nread;
	char buf[BUF_SIZE];
	struct linux_dirent64* d;
	int bpos, fd;
	struct stat a_stat;

	fd = try_open(".", O_RDONLY | O_DIRECTORY);
	
	while(true) {
		nread = syscall(SYS_getdents64, fd, buf, BUF_SIZE);
		if (-1 == nread) {
			perror(cwd->st);
			return;
		} else if (0 == nread) {
			break;
		}
		for (bpos = 0; bpos < nread;) {
			d = (struct linux_dirent64 *) (buf + bpos);
			if (d->d_type != DT_DIR) {
				if (-1 == stat(d->d_name, &a_stat)) {
					perror(d->d_name);
					bpos += d->d_reclen;
					continue;
				}
				if (functor.function(d->d_name, d->d_ino, a_stat.st_size, a_stat.st_nlink, launch)) {
					printf("%8ld ", d->d_ino);
					printf("%10s ", get_type(d->d_type));
					printf("%s/%s\n", cwd->st, d->d_name);
				}
			}
			if (d->d_type == DT_DIR && strcmp(d->d_name, "..") && strcmp(d->d_name, ".")) {
				if (chdir(d->d_name) == 0) {
					int return_index = strlen(cwd->st);

					int requested_size = strlen(cwd->st) + strlen(d->d_name) + 2;
					if (requested_size > cwd->capacity) {
						char* temp = malloc(requested_size * sizeof(char));
						if (temp == 0) {
							critical("Out of memory");
						}
						strcpy(temp, cwd->st);
						free(cwd->st);
						cwd->st = temp;
						cwd->capacity = requested_size;
					}
					strcat(cwd->st, "/");
					strcat(cwd->st, d->d_name);
					
					directories_walk(cwd, functor);
					
					cwd->st[return_index] = 0;

					if (-1 == chdir("..")) {
						perror(d->d_name);
						critical("Internal or file system's fault");
					}
				}
			}
			bpos += d->d_reclen;
		}
	}
	close(fd);
}

const char* BAD_USAGE = "bad usage\n Try 'find -help' to see the manual";

enum instruction_code_t get_key(const char* key) {
	if (0 == strcmp(key,  "-name")) return E_NAME;
	if (0 == strcmp(key,  "-size")) return L_SIZE;
	if (0 == strcmp(key,  "=size")) return E_SIZE;
	if (0 == strcmp(key,  "+size")) return G_SIZE;
	if (0 == strcmp(key, "-nlink")) return N_LINK;
	if (0 == strcmp(key,  "-inum")) return E_INUM;
	if (0 == strcmp(key,  "-exec")) return EXEC;
	return UNKNOWN;
}

void temp(int a, int b, int c, int d, int e, int f) {

}

int main(int argc, char** argv, char* const* env) {
	if (argc < 2) {
		critical(BAD_USAGE);
	}
	
	if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "/?") == 0) {
		printf("Usage: find [-help] [-name name] [-inum inum]\n"
		 "[-size -size] [-size =size] [-size +size] [-nlinks nlinks] [-exec path]\n\n"
		 "-help, -h, /?    print this page and ignore other keys\n"
		 "-name   specify the name util will look for\n"
		 "-inum   specify the number of the INode util will look for\n"
		 "-size   specify the upper bound for the size of the files util will look for\n"
		 "=size   specify the exact size of the files util will look for\n"
		 "+size   specify the lower bound for the size of the files util will look for\n"
		 "-nlinks     specify the number of hardlinks of the file util will look for\n"
		 "-exec       specify the programme to recieve searched files.\n\n"
		 "'find' handles with instructions (keys) interpretationally, e.g.:\n"
		 "find . -name stript.sh -exec /bin/bash +size 100 -exec /bin/bash\n"
		 "will execute every script with name 'script.sh' in the current directory\n"
		 "(and its subdir's) then reexecute those of these scripts which a bigger\n"
		 "then 100 bytes.");
		return EXIT_SUCCESS;
	}

	envp = env;
	struct function_t functor;
	functor.instruction_sequence = malloc(argc * sizeof(struct instruction_t));
	if (functor.instruction_sequence == 0) {
		critical("Out of memory");
	}
	functor.number = 0;

	for (size_t index = 2; argv[index]; index++) {
		enum instruction_code_t key = get_key(argv[index]);
		boolean is_numeral = (key != E_NAME) && (key != EXEC);
		if (key == UNKNOWN) { critical(BAD_USAGE); }

		index++;
		if (argv[index]) {
			functor.instruction_sequence[functor.number].instruction = key;
			if (is_numeral) {
				functor.instruction_sequence[functor.number].instruction_data.integer = atoi(argv[index]);
			} else {
				functor.instruction_sequence[functor.number].instruction_data.string = argv[index];
			}
			functor.number++;
		} else { critical(BAD_USAGE); }
	}

	struct string_t cwd;
	cwd.st = strdup(argv[1]);
	if (cwd.st == 0) {
		critical("Out of memory");
	}
	cwd.capacity = strlen(argv[1]);

	if (-1 == chdir(argv[1])) {
		perror(argv[1]);
		critical("Bad root directory");
	}
	struct checker_t checker;
	checker = compileJIT(functor);
	directories_walk(&cwd, checker);

	free(cwd.st);
	free(functor.instruction_sequence);
	munmap(checker.function, checker.size);
	return EXIT_SUCCESS;
}
