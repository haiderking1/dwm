#include "autostart.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void
autostart_run(const char *const *const commands[], int display_fd)
{
	const char *const *argv;
	struct sigaction action = {0};
	pid_t pid;
	unsigned int i;

	if (!commands)
		return;
	for (i = 0; (argv = commands[i]); ++i) {
		if (!argv[0] || !argv[0][0])
			continue;
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "dwm: autostart %s: fork: %s\n", argv[0], strerror(errno));
			continue;
		}
		if (pid != 0)
			continue;
		/* Apps must not retain dwm's connection to the X server. */
		if (display_fd >= 0)
			close(display_fd);
		if (setsid() < 0) {
			fprintf(stderr, "dwm: autostart %s: setsid: %s\n", argv[0], strerror(errno));
			_exit(127);
		}
		/* Restore child reaping for apps that launch their own processes. */
		action.sa_handler = SIG_DFL;
		sigemptyset(&action.sa_mask);
		if (sigaction(SIGCHLD, &action, NULL) < 0) {
			fprintf(stderr, "dwm: autostart %s: sigaction: %s\n", argv[0], strerror(errno));
			_exit(127);
		}
		/* execvp does not modify argv; its API predates const strings. */
		execvp(argv[0], (char *const *)argv);
		fprintf(stderr, "dwm: autostart %s: %s\n", argv[0], strerror(errno));
		_exit(127);
	}
}
