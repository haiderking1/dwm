#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "build.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int result_fd = -1;
static int last_result = -1;

static void
warning(const char *operation)
{
	int saved = errno;
	fprintf(stderr, "dwm reload: %s: %s\n", operation, strerror(saved));
	errno = saved;
}

static void
finish(int fd, int success)
{
	unsigned char result = success ? 1 : 0;
	ssize_t n;
	do {
		n = write(fd, &result, sizeof result);
	} while (n < 0 && errno == EINTR);
	close(fd);
	_exit(success ? 0 : 1);
}

static int
set_signal(int signal_number, void (*handler)(int))
{
	struct sigaction action;
	memset(&action, 0, sizeof action);
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	return sigaction(signal_number, &action, NULL);
}

static void
close_inherited(long max_fd)
{
#ifdef SYS_close_range
	if (syscall(SYS_close_range, 4U, ~0U, 0U) == 0)
		return;
#endif
	for (long fd = 4; fd < max_fd && fd <= INT_MAX; ++fd)
		close((int)fd);
}

static int
open_log(const char *path)
{
	struct stat st;
	int fd, saved;

	/* Validate before truncating, including pre-existing files. O_NONBLOCK
	 * prevents a malicious FIFO from hanging the wrapper during open. */
	fd = open(path, O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK,
	          0600);
	if (fd < 0)
		return -1;
	if (fstat(fd, &st) < 0)
		goto fail;
	if (!S_ISREG(st.st_mode) || st.st_uid != geteuid() || st.st_nlink != 1) {
		errno = EPERM;
		goto fail;
	}
	if (fchmod(fd, 0600) < 0 || ftruncate(fd, 0) < 0 ||
	    fcntl(fd, F_SETFL, 0) < 0)
		goto fail;
	return fd;
fail:
	saved = errno;
	close(fd);
	errno = saved;
	return -1;
}

static void
run_wrapper(int read_fd, int write_fd, int display_fd, long max_fd,
            const char *source, char *prefix_arg, const char *logpath)
{
	int log_fd, null_fd, status;
	pid_t child, waited;
	sigset_t unblock;
	char *args[] = { "make", "-C", (char *)source, "install", prefix_arg, NULL };

	close(read_fd);
	if (display_fd >= 0 && display_fd != read_fd && display_fd != write_fd)
		close(display_fd);
	if (set_signal(SIGPIPE, SIG_IGN) < 0 ||
	    set_signal(SIGCHLD, SIG_DFL) < 0) {
		warning("reset wrapper signals");
		_exit(1); /* EOF is failure even if SIGPIPE setup failed. */
	}
	sigemptyset(&unblock);
	sigaddset(&unblock, SIGCHLD);
	sigaddset(&unblock, SIGPIPE);
	if (sigprocmask(SIG_UNBLOCK, &unblock, NULL) < 0 || setsid() < 0) {
		warning("detach wrapper");
		finish(write_fd, 0);
	}
	/* Reserve descriptor 3 for the report; discard all other inherited
	 * descriptors, not just the X connection. Keep stderr for setup errors. */
	if (write_fd != 3) {
		if (dup2(write_fd, 3) < 0) {
			warning("move result pipe");
			finish(write_fd, 0);
		}
		close(write_fd);
	}
	write_fd = 3;
	if (fcntl(write_fd, F_SETFD, FD_CLOEXEC) < 0) {
		warning("protect result pipe");
		finish(write_fd, 0);
	}
	close_inherited(max_fd);
	log_fd = open_log(logpath);
	if (log_fd < 0) {
		warning("open rebuild log");
		finish(write_fd, 0);
	}
	/* Normalize the log descriptor even when a standard fd was closed. */
	if (log_fd < 4) {
		int moved = fcntl(log_fd, F_DUPFD_CLOEXEC, 4);
		close(log_fd);
		log_fd = moved;
	}
	if (log_fd < 0 || dup2(log_fd, STDOUT_FILENO) < 0 ||
	    dup2(log_fd, STDERR_FILENO) < 0) {
		warning("redirect rebuild log");
		finish(write_fd, 0);
	}
	close(log_fd);
	null_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
	if (null_fd < 0 || dup2(null_fd, STDIN_FILENO) < 0 ||
	    fcntl(STDIN_FILENO, F_SETFD, 0) < 0) {
		warning("redirect rebuild input");
		finish(write_fd, 0);
	}
	if (null_fd != STDIN_FILENO)
		close(null_fd);
	child = fork();
	if (child < 0) {
		warning("fork make");
		finish(write_fd, 0);
	}
	if (child == 0) {
		close(write_fd);
		if (set_signal(SIGPIPE, SIG_DFL) < 0) {
			warning("reset make SIGPIPE");
			_exit(127);
		}
		execvp(args[0], args);
		warning("exec make");
		_exit(127);
	}
	do {
		waited = waitpid(child, &status, 0);
	} while (waited < 0 && errno == EINTR);
	if (waited < 0) {
		warning("wait for make");
		finish(write_fd, 0);
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		if (WIFSIGNALED(status))
			fprintf(stderr, "dwm reload: make killed by signal %d\n", WTERMSIG(status));
		else
			fprintf(stderr, "dwm reload: make exited with status %d\n", WEXITSTATUS(status));
		finish(write_fd, 0);
	}
	finish(write_fd, 1);
}

int
reload_build_start(const char *source, const char *prefix,
                   const char *logpath, int display_fd)
{
	int pipe_fd[2] = { -1, -1 }, saved;
	long max_fd;
	char *prefix_arg;
	pid_t wrapper;

	if (result_fd >= 0)
		return 0;
	if (!source || !*source || !prefix || !logpath || !*logpath) {
		errno = EINVAL;
		warning("invalid rebuild arguments");
		return -1;
	}
	prefix_arg = malloc(strlen(prefix) + sizeof "PREFIX=");
	if (!prefix_arg) {
		warning("allocate make argument");
		return -1;
	}
	strcpy(prefix_arg, "PREFIX=");
	strcat(prefix_arg, prefix);
	max_fd = sysconf(_SC_OPEN_MAX);
	if (max_fd < 0 || pipe(pipe_fd) < 0)
		goto fail;
	for (int i = 0; i < 2; ++i) {
		/* Do not let closed stdin/stdout/stderr alias the report pipe. */
		int moved = fcntl(pipe_fd[i], F_DUPFD_CLOEXEC, 3);
		if (moved < 0)
			goto fail;
		close(pipe_fd[i]);
		pipe_fd[i] = moved;
	}
	if (fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK) < 0)
		goto fail;
	wrapper = fork();
	if (wrapper < 0)
		goto fail;
	if (wrapper == 0)
		run_wrapper(pipe_fd[0], pipe_fd[1], display_fd, max_fd,
		            source, prefix_arg, logpath);
	close(pipe_fd[1]);
	free(prefix_arg);
	result_fd = pipe_fd[0];
	last_result = 0;
	return 1;
fail:
	saved = errno;
	if (pipe_fd[0] >= 0)
		close(pipe_fd[0]);
	if (pipe_fd[1] >= 0)
		close(pipe_fd[1]);
	free(prefix_arg);
	errno = saved;
	warning("start rebuild");
	return -1;
}

int
reload_build_fd(void)
{
	return result_fd;
}

int
reload_build_result(void)
{
	unsigned char result;
	ssize_t n;

	if (result_fd < 0)
		return last_result;
	do {
		n = read(result_fd, &result, sizeof result);
	} while (n < 0 && errno == EINTR);
	if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		return 0;
	if (n < 0)
		warning("read rebuild result");
	else if (n == 0)
		fprintf(stderr, "dwm reload: wrapper closed pipe without a result\n");
	last_result = n == 1 && result == 1 ? 1 : -1;
	close(result_fd);
	result_fd = -1;
	return last_result;
}

void
reload_build_cancel(void)
{
	if (result_fd >= 0)
		close(result_fd);
	result_fd = -1;
	last_result = -1;
}
