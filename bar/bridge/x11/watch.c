#include "x11.h"
#include "memory.h"
#include "state.h"
#include "wallpaper.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#define EVENT_BATCH 128
#define MEMORY_INTERVAL 5

static volatile sig_atomic_t stopping;

static void
stop_watch(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

static int
emit_state(Display *display, Window root, Atom state, Atom utf8)
{
    Atom actual_type = None;
    int actual_format = 0, status, ok;
    unsigned long count = 0, remaining = 0;
    unsigned char *data = NULL;

    status = XGetWindowProperty(display, root, state, 0,
                                (long)(BRIDGE_STATE_LIMIT / 4), False, utf8,
                                &actual_type, &actual_format, &count,
                                &remaining, &data);
    if (status != Success || actual_type != utf8 || actual_format != 8 ||
        remaining != 0 || count > BRIDGE_STATE_LIMIT)
        ok = bridge_write_state(stdout, NULL, 0);
    else
        ok = bridge_write_state(stdout, data, (size_t)count);
    if (data)
        XFree(data);
    return ok;
}

static int
emit_memory(void)
{
    struct bridge_memory memory;
    if (!bridge_read_memory(&memory)) {
        fprintf(stderr, "dwm-bar-bridge: cannot read valid /proc/meminfo\n");
        /* Keep watching state; try memory again at the next deadline. */
        return 1;
    }
    return printf("{\"type\":\"memory\",\"percent\":%.6f,\"usedGiB\":%.6f}\n",
                  memory.percent, memory.used_gib) >= 0 && fflush(stdout) == 0;
}

static int
monotonic_now(struct timespec *now)
{
    if (clock_gettime(CLOCK_MONOTONIC, now) == 0)
        return 1;
    perror("dwm-bar-bridge: clock_gettime");
    return 0;
}

/* Round up so poll cannot spin during the last fraction of a millisecond. */
static int
milliseconds_until(const struct timespec *deadline, const struct timespec *now)
{
    time_t seconds = deadline->tv_sec - now->tv_sec;
    long nanoseconds = deadline->tv_nsec - now->tv_nsec;
    if (nanoseconds < 0) {
        --seconds;
        nanoseconds += 1000000000L;
    }
    if (seconds < 0)
        return 0;
    return (int)(seconds * 1000 + (nanoseconds + 999999L) / 1000000L);
}

int
bridge_watch(Display *display, Window root)
{
    Atom state = XInternAtom(display, "_DWM_QUICKSHELL_STATE", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom wallpaper = XInternAtom(display, "_XROOTPMAP_ID", False);
    struct timespec now, deadline;
    struct pollfd connection = { ConnectionNumber(display), POLLIN, 0 };
    struct sigaction action = {0}, old_int, old_term;
    int result = 1;

    action.sa_handler = stop_watch;
    sigemptyset(&action.sa_mask);
    stopping = 0;
    if (sigaction(SIGINT, &action, &old_int) != 0) {
        perror("dwm-bar-bridge: sigaction");
        return 0;
    }
    if (sigaction(SIGTERM, &action, &old_term) != 0) {
        perror("dwm-bar-bridge: sigaction");
        sigaction(SIGINT, &old_int, NULL);
        return 0;
    }

    /* Subscribe before reading so updates between setup and read are queued. */
    XSelectInput(display, root, PropertyChangeMask | StructureNotifyMask);
    if (!emit_state(display, root, state, utf8) || !emit_memory() ||
        !bridge_wallpaper_emit(display, root))
        goto output_error;
    if (!monotonic_now(&deadline))
        goto failure;
    deadline.tv_sec += MEMORY_INTERVAL;

    while (!stopping) {
        int batch, dirty = 0, wallpaper_dirty = 0, timeout, status;
        XEvent event;

        /* Check the timer on every batch, even when X never becomes idle. */
        if (!monotonic_now(&now))
            goto failure;
        if (milliseconds_until(&deadline, &now) == 0) {
            if (!emit_memory())
                goto output_error;
            deadline = now;
            deadline.tv_sec += MEMORY_INTERVAL;
        }
        for (batch = 0; batch < EVENT_BATCH && !stopping && XPending(display);
             ++batch) {
            XNextEvent(display, &event);
            if (event.type == PropertyNotify &&
                event.xproperty.window == root && event.xproperty.atom == state)
                dirty = 1;
            if ((event.type == PropertyNotify && event.xproperty.window == root &&
                 event.xproperty.atom == wallpaper) ||
                (event.type == ConfigureNotify && event.xconfigure.window == root))
                wallpaper_dirty = 1;
        }
        /* Coalesce updates in this batch; always read the latest property. */
        if (dirty && !emit_state(display, root, state, utf8))
            goto output_error;
        if (wallpaper_dirty && !bridge_wallpaper_emit(display, root))
            goto output_error;
        if (stopping)
            break;
        if (XPending(display))
            continue;
        if (!monotonic_now(&now))
            goto failure;
        timeout = milliseconds_until(&deadline, &now);
        status = poll(&connection, 1, timeout);
        if (status < 0) {
            if (errno == EINTR)
                continue;
            perror("dwm-bar-bridge: poll");
            goto failure;
        }
        if (status > 0 && (connection.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            fprintf(stderr, "dwm-bar-bridge: X connection closed\n");
            goto failure;
        }
    }
    goto finish;

output_error:
    fprintf(stderr, "dwm-bar-bridge: cannot write stdout\n");
failure:
    result = 0;
finish:
    bridge_wallpaper_cleanup();
    sigaction(SIGINT, &old_int, NULL);
    sigaction(SIGTERM, &old_term, NULL);
    return result;
}
