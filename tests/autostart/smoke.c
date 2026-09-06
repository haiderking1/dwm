#include "../../startup/autostart.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void child_signal(int signal_number) { (void)signal_number; }

int main(int argc, char **argv)
{
	struct sigaction action = {0};
	int fd, status, successes = 0, failures = 0;
	pid_t pid;
	char descriptor[32];
	if (argc == 4 && !strcmp(argv[1], "child")) {
		assert(!strcmp(argv[3], "one argument with spaces;$HOME"));
		assert(fcntl(atoi(argv[2]), F_GETFD) == -1 && errno == EBADF);
		assert(getsid(0) == getpid());
		assert(sigaction(SIGCHLD, NULL, &action) == 0);
		assert(action.sa_handler == SIG_DFL);
		return 0;
	}
	alarm(5);
	fd = open("/dev/null", O_RDONLY);
	assert(fd >= 0);
	snprintf(descriptor, sizeof descriptor, "%d", fd);
	action.sa_handler = child_signal;
	sigemptyset(&action.sa_mask);
	assert(sigaction(SIGCHLD, &action, NULL) == 0);
	{
		const char *const empty[] = { NULL };
		const char *const valid[] = { argv[0], "child", descriptor, "one argument with spaces;$HOME", NULL };
		const char *const missing[] = { "/nonexistent/dwm-autostart-test", NULL };
		const char *const *const commands[] = { empty, valid, missing, valid, NULL };
		const char *const *const none[] = { NULL };
		autostart_run(NULL, fd);
		autostart_run(none, fd);
		autostart_run(commands, fd);
	}
	for (;;) {
		pid = wait(&status);
		if (pid < 0 && errno == EINTR) continue;
		if (pid < 0) { assert(errno == ECHILD); break; }
		assert(WIFEXITED(status));
		if (WEXITSTATUS(status) == 0) successes++;
		else { assert(WEXITSTATUS(status) == 127); failures++; }
	}
	assert(successes == 2 && failures == 1);
	assert(fcntl(fd, F_GETFD) >= 0);
	close(fd);
	puts("autostart tests passed");
	return 0;
}
