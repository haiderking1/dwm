#define _POSIX_C_SOURCE 200809L
#include "../../reload/build.h"
#include "fixture.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int
await_result(void)
{
	struct pollfd pfd = { reload_build_fd(), POLLIN, 0 };
	int n;
	assert(pfd.fd >= 0);
	do {
		n = poll(&pfd, 1, 5000);
	} while (n < 0 && errno == EINTR);
	assert(n == 1);
	assert(pfd.revents & (POLLIN | POLLHUP));
	n = reload_build_result();
	assert(n != 0 && reload_build_fd() == -1);
	assert(reload_build_result() == n);
	return n;
}

static void
assert_closed_in_child(int fd)
{
	struct pollfd pfd = { fd, POLLIN, 0 };
	char byte;
	assert(poll(&pfd, 1, 5000) == 1);
	assert(read(fd, &byte, 1) == 0);
	close(fd);
}

static void
check_log(const char *path, const char *first, const char *second)
{
	char text[8192];
	struct stat st;
	FILE *file = fopen(path, "r");
	size_t n;
	assert(file);
	n = fread(text, 1, sizeof text - 1, file);
	text[n] = '\0';
	assert(!ferror(file) && fclose(file) == 0);
	assert(strstr(text, first));
	assert(!second || strstr(text, second));
	assert(!strstr(text, "old log sentinel"));
	assert(stat(path, &st) == 0);
	assert((st.st_mode & 0777) == 0600 && st.st_uid == geteuid());
}

int
main(int argc, char **argv)
{
	char dir[] = "/tmp/dwm-reload-test-XXXXXX";
	char makefile[1024], log[1024], ready[1024], release[1024];
	char complete[1024], prefix[1024], unsafe[1024], missing[1024];
	int display[2], other[2], fd;
	struct sigaction action;

	if (argc > 1 && strcmp(argv[1], "--probe") == 0)
		return fixture_probe(argc, argv);
	assert(argc == 2 && argv[1][0] == '/');
	memset(&action, 0, sizeof action);
	action.sa_handler = SIG_IGN;
	sigemptyset(&action.sa_mask);
	assert(sigaction(SIGCHLD, &action, NULL) == 0);
	assert(mkdtemp(dir));
	fixture_path(makefile, sizeof makefile, dir, "Makefile");
	fixture_path(log, sizeof log, dir, "build.log");
	fixture_path(ready, sizeof ready, dir, "ready");
	fixture_path(release, sizeof release, dir, "release");
	fixture_path(complete, sizeof complete, dir, "complete");
	fixture_path(prefix, sizeof prefix, dir, "prefix with spaces;not-a-command");
	fixture_path(unsafe, sizeof unsafe, dir, "unsafe.log");
	fixture_path(missing, sizeof missing, dir, "missing-directory");
	assert(setenv("RELOAD_TEST_BIN", argv[1], 1) == 0);
	assert(setenv("RELOAD_TEST_DIR", dir, 1) == 0);
	assert(setenv("RELOAD_TEST_PREFIX", prefix, 1) == 0);
	assert(unsetenv("MAKEFLAGS") == 0);
	assert(unsetenv("MFLAGS") == 0);
	assert(unsetenv("MAKEFILES") == 0);
	assert(reload_build_fd() == -1 && reload_build_result() == -1);
	assert(reload_build_start(NULL, prefix, log, -1) == -1);
	assert(errno == EINVAL);

	fixture_write(makefile,
	    ".PHONY: install\ninstall:\n\t@\"$(RELOAD_TEST_BIN)\" --probe \"$(PREFIX)\"\n");
	fixture_write(log, "old log sentinel\n");
	assert(chmod(log, 0644) == 0);
	assert(pipe(display) == 0 && pipe(other) == 0);
	assert(reload_build_start(dir, prefix, log, display[1]) == 1);
	fd = reload_build_fd();
	assert(fcntl(fd, F_GETFD) & FD_CLOEXEC);
	assert(fcntl(fd, F_GETFL) & O_NONBLOCK);
	assert(fcntl(display[1], F_GETFD) >= 0);
	close(display[1]);
	close(other[1]);
	fixture_wait(ready);
	assert(reload_build_result() == 0);
	assert(reload_build_start(dir, prefix, log, -1) == 0);
	assert(reload_build_fd() == fd);
	assert_closed_in_child(display[0]);
	assert_closed_in_child(other[0]);
	fixture_write(release, "go\n");
	assert(await_result() == 1);
	check_log(log, "probe stdout", "probe stderr");

	fixture_write(makefile, ".PHONY: install\ninstall:\n\t@echo expected-failure >&2\n\t@exit 7\n");
	assert(reload_build_start(dir, prefix, log, -1) == 1);
	assert(await_result() == -1);
	check_log(log, "expected-failure", "dwm reload: make exited with status");
	assert(reload_build_start(missing, prefix, log, -1) == 1);
	assert(await_result() == -1);

	/* execvp failure must also arrive asynchronously and reach the log. */
	{
		char *path = getenv("PATH") ? strdup(getenv("PATH")) : NULL;
		assert(!getenv("PATH") || path);
		assert(setenv("PATH", missing, 1) == 0);
		assert(reload_build_start(dir, prefix, log, -1) == 1);
		assert(await_result() == -1);
		check_log(log, "dwm reload: exec make", NULL);
		assert((path ? setenv("PATH", path, 1) : unsetenv("PATH")) == 0);
		free(path);
	}

	/* Refuse symlinks, hardlinks and FIFOs without truncating the target. */
	fixture_write(log, "old log sentinel\n");
	assert(symlink(log, unsafe) == 0);
	assert(reload_build_start(dir, prefix, unsafe, -1) == 1);
	assert(await_result() == -1);
	assert(unlink(unsafe) == 0);
	assert(link(log, unsafe) == 0);
	assert(reload_build_start(dir, prefix, unsafe, -1) == 1);
	assert(await_result() == -1);
	assert(unlink(unsafe) == 0);
	{
		char text[64];
		FILE *file = fopen(log, "r");
		assert(file && fgets(text, sizeof text, file));
		assert(strcmp(text, "old log sentinel\n") == 0);
		assert(fclose(file) == 0);
	}
	assert(mkfifo(unsafe, 0600) == 0);
	assert(reload_build_start(dir, prefix, unsafe, -1) == 1);
	assert(await_result() == -1);
	assert(unlink(unsafe) == 0);

	/* Reap this wrapper explicitly to verify it exits normally, not SIGPIPE. */
	action.sa_handler = SIG_DFL;
	assert(sigaction(SIGCHLD, &action, NULL) == 0);
	/* Cancel only drops the pipe. The make recipe must still finish. */
	assert(unlink(ready) == 0 && unlink(release) == 0 && unlink(complete) == 0);
	fixture_write(makefile,
	    ".PHONY: install\ninstall:\n\t@\"$(RELOAD_TEST_BIN)\" --probe \"$(PREFIX)\"\n");
	assert(reload_build_start(dir, prefix, log, -1) == 1);
	fixture_wait(ready);
	fd = reload_build_fd();
	reload_build_cancel();
	assert(reload_build_fd() == -1 && reload_build_result() == -1);
	assert(fcntl(fd, F_GETFD) == -1 && errno == EBADF);
	reload_build_cancel();
	fixture_write(release, "go\n");
	fixture_wait(complete);
	{
		long session;
		int status = 0;
		pid_t waited = 0;
		FILE *file = fopen(ready, "r");
		assert(file && fscanf(file, "%ld", &session) == 1);
		assert(fclose(file) == 0 && session > 1);
		for (int i = 0; i < 500 && waited == 0; ++i) {
			waited = waitpid((pid_t)session, &status, WNOHANG);
			assert(waited >= 0);
			if (waited == 0)
				assert(poll(NULL, 0, 10) == 0);
		}
		assert(waited == (pid_t)session);
		assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
	}
	action.sa_handler = SIG_IGN;
	assert(sigaction(SIGCHLD, &action, NULL) == 0);
	assert(unlink(ready) == 0 && unlink(release) == 0 && unlink(complete) == 0);
	assert(unlink(makefile) == 0 && unlink(log) == 0 && rmdir(dir) == 0);
	puts("reload build API tests passed");
	return 0;
}
