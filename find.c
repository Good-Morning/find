#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>

void handle_error(const char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

struct linux_dirent64 {
	ino64_t d_ino;
	off64_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

#define BUF_SIZE 1024 * 16

int main(int argc, char** argv) {
	int fd, nread;
	char buf[BUF_SIZE];
	struct linux_dirent64 *d;
	int bpos;
	char d_type;

	fd = open(argc > 1 ?  argv[1] : ".", O_RDONLY | O_DIRECTORY);
	if (fd == -1) {
		handle_error("open");
	}

	while (-1) {
		nread = syscall(SYS_getdents64, fd, buf, BUF_SIZE);
		if (nread == -1) {
			handle_error("getdets");
		}
		if (nread == 0) {
			break;
		}

		printf("=== nread = %d ===\n", nread);
		printf("  inode# file type d_reclen   d_off   d_name\n");
		for (bpos = 0; bpos < nread;) {
			d = (struct linux_dirent64 *) (buf + bpos);
			printf("%8ld ", d->d_ino);
			d_type = d->d_type;
			printf("%10s ", (d->d_type == DT_REG ) ?   "regular" :
							(d->d_type == DT_DIR ) ? "directory" :
							(d->d_type == DT_FIFO) ?      "FIFO" :
							(d->d_type == DT_SOCK) ?    "socket" :
							(d->d_type == DT_LNK ) ?   "symlink" :
							(d->d_type == DT_BLK ) ? "block dev" :
							(d->d_type == DT_CHR ) ?  "char dev" :
							"unknown");
			printf("%4d %ld %s\n", d->d_reclen, d->d_off, d->d_name);
			bpos += d->d_reclen;
		}
	}

	exit(EXIT_SUCCESS);
}
