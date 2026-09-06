#ifndef RELOAD_BUILD_H
#define RELOAD_BUILD_H

/* Start returns 1 on launch, 0 while a result is unconsumed, -1 on error.
 * Log/make setup runs asynchronously; those errors arrive through result().
 * source, prefix and logpath need only remain valid until start returns.
 * The caller must ignore SIGCHLD or otherwise reap the wrapper process.
 * Single-threaded API, intended for dwm's event loop.
 */
int reload_build_start(const char *source, const char *prefix,
                     const char *logpath, int display_fd);
/* Poll this descriptor for POLLIN/POLLHUP. Never read or close it directly. */
int reload_build_fd(void);
/* 0 pending, 1 success, -1 failure/no job. Terminal results are retained. */
int reload_build_result(void);
/* Drop the result pipe without signalling any process; work may continue. */
void reload_build_cancel(void);

#endif
