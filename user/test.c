#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHILD_COUNT 20
static const int WAIT_TIME = 5;

int main(int argc, char **argv)
{
	static const char arg1[] = "/bin/ls";
	static const char arg2[] = "-R";
	static const char * const argls[] = {arg1, arg2};

	chdir("/ptreefs");
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if (pid == 0) {
		printf("======================================\n");
		printf("initial process tree\n");
		printf("======================================\n");
		execv(argls[0], argls);
		exit(0);
	}

	while (wait(NULL) > 0)
		;


	for (int i = 0; i < CHILD_COUNT; ++i) {
		pid = fork();

		if (pid < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		if (pid == 0) {
			sleep(WAIT_TIME);
			return 0;
		}
	}

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if (pid == 0) {
		printf("======================================\n");
		printf("After creating processes, name: test\n");
		printf("======================================\n");
		execv(argls[0], argls);
		exit(0);
	}


	while (wait(NULL) > 0)
		;
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if (pid == 0) {
		printf("======================================\n");
		printf("After processes terminated\n");
		printf("======================================\n");
		execv(argls[0], argls);
		exit(0);
	}

	while (wait(NULL) > 0)
		;

	return 0;
}
